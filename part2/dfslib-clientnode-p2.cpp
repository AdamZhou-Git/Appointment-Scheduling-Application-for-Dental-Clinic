#include <regex>
#include <mutex>
#include <vector>
#include <string>
#include <thread>
#include <cstdio>
#include <chrono>
#include <errno.h>
#include <csignal>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <getopt.h>
#include <unistd.h>
#include <limits.h>
#include <sys/inotify.h>
#include <grpcpp/grpcpp.h>
#include <google/protobuf/util/time_util.h>
#include <utime.h>

#include "src/dfs-utils.h"
#include "src/dfslibx-clientnode-p2.h"
#include "dfslib-shared-p2.h"
#include "dfslib-clientnode-p2.h"
#include "proto-src/dfs-service.grpc.pb.h"

using grpc::Status;
using grpc::Channel;
using grpc::StatusCode;
using grpc::ClientWriter;
using grpc::ClientReader;
using grpc::ClientContext;

using namespace std::chrono;
using namespace google::protobuf::util;


extern dfs_log_level_e DFS_LOG_LEVEL;
//http://www.cplusplus.com/reference/mutex/mutex/lock/
std::mutex glb_mtx;

//
// STUDENT INSTRUCTION:
//
// Change these "using" aliases to the specific
// message types you are using to indicate
// a file request and a listing of files from the server.
//
using FileRequestType = dfs_service::FileName;
using FileListResponseType = dfs_service::FileInfos;


DFSClientNodeP2::DFSClientNodeP2() : DFSClientNode() {}
DFSClientNodeP2::~DFSClientNodeP2() {}

grpc::StatusCode DFSClientNodeP2::RequestWriteAccess(const std::string &filename) {

    //
    // STUDENT INSTRUCTION:
    //
    // Add your request to obtain a write lock here when trying to store a file.
    // This method should request a write lock for the given file at the server,
    // so that the current client becomes the sole creator/writer. If the server
    // responds with a RESOURCE_EXHAUSTED response, the client should cancel
    // the current file storage
    //
    // The StatusCode response should be:
    //
    // OK - if all went well
    // StatusCode::DEADLINE_EXCEEDED - if the deadline timeout occurs
    // StatusCode::RESOURCE_EXHAUSTED - if a write lock cannot be obtained
    // StatusCode::CANCELLED otherwise
    //
    // Pass client ID and filename to request a lock

    ClientContext ctx;
    time_point<system_clock> expirationTime = system_clock::now() + milliseconds(deadline_timeout); 
    ctx.set_deadline(expirationTime);

    dfs_service::WriteLockRequest writeLockRequst;
    writeLockRequst.set_clientid(ClientId());
    writeLockRequst.set_name(filename);
    dfs_service::WriteLockResponse writeLockResponse;

    grpc::Status status = service_stub->RequestWriteLock(&ctx, writeLockRequst, &writeLockResponse);

    if (!status.ok())
    {
        return status.error_code();
    }
    
    return StatusCode::OK;
}

grpc::StatusCode DFSClientNodeP2::Store(const std::string &filename) {

    //
    // STUDENT INSTRUCTION:
    //
    // Add your request to store a file here. Refer to the Part 1
    // student instruction for details on the basics.
    //
    // You can start with your Part 1 implementation. However, you will
    // need to adjust this method to recognize when a file trying to be
    // stored is the same on the server (i.e. the ALREADY_EXISTS gRPC response).
    //
    // You will also need to add a request for a write lock before attempting to store.
    //
    // If the write lock request fails, you should return a status of RESOURCE_EXHAUSTED
    // and cancel the current operation.
    //
    // The StatusCode response should be:
    //
    // StatusCode::OK - if all went well
    // StatusCode::DEADLINE_EXCEEDED - if the deadline timeout occurs
    // StatusCode::ALREADY_EXISTS - if the local cached file has not changed from the server version
    // StatusCode::RESOURCE_EXHAUSTED - if a write lock cannot be obtained
    // StatusCode::CANCELLED otherwise
    //
    //
    // 1. check if file exists and identical to cache. (Handle ALREADY_EXISTS)
    // 2. Request write lock (handle RESOURCE_EXHAUSTED)
    // 3. Proceed to write (handle OK)
    ClientContext ctx;
    ctx.AddMetadata("filename", filename);
    time_point<system_clock> expirationTime = system_clock::now() + milliseconds(deadline_timeout); 
    ctx.set_deadline(expirationTime);

    //check exists locally
    const std::string& filePath = WrapPath(filename);
    struct stat buffer;
    int fileSize;
    if (stat(filePath.c_str(), &buffer) != 0)
    {
        return StatusCode::NOT_FOUND;
    }
    else{
        fileSize = buffer.st_size;
    }

    //request lock
    grpc::StatusCode lockStatus;
    if ((lockStatus = this->RequestWriteAccess(filename)) != grpc::StatusCode::OK)
    {
        return lockStatus;
    }

    //check exists remotely and verify checksum
    ClientContext ctx2;
    time_point<system_clock> expirationTime2 = system_clock::now() + milliseconds(deadline_timeout); 
    ctx2.set_deadline(expirationTime2);

    dfs_service::FileName fileName;
    fileName.set_name(filename);
    dfs_service::FileInfo fileInfo;

    grpc::Status status = service_stub->GetFileInfo(&ctx2, fileName, &fileInfo);

    uint32_t localCheckSum = dfs_file_checksum(filePath, &crc_table);
    if (localCheckSum == fileInfo.checksum())
    {
        return grpc::StatusCode::ALREADY_EXISTS;
    }
    
    //write   
    dfs_service::StoreResponse response;
    std::unique_ptr<ClientWriter<dfs_service::FilePacket>> packetSender(service_stub -> Store(&ctx, &response));
    std::ifstream fileStream(filePath, std::ifstream::binary);
    dfs_service::FilePacket packetHolder;
    int bytesTransferred = 0;

    while (bytesTransferred < fileSize && !fileStream.eof())
    {
        std::vector<char> senderBuffer(PacketSize);
        int bytesToTransfer = std::min(fileSize - bytesTransferred, PacketSize);
        fileStream.read(senderBuffer.data(), bytesToTransfer);
        packetHolder.set_packet(senderBuffer.data(), bytesToTransfer);
        packetSender->Write(packetHolder);
        bytesTransferred += bytesToTransfer;
    }

    fileStream.close();
    packetSender->WritesDone();
    grpc::Status storeStatus = packetSender->Finish();

    if (!storeStatus.ok())
    {
        return StatusCode::CANCELLED;
    }
    
    return StatusCode::OK;
}


grpc::StatusCode DFSClientNodeP2::Fetch(const std::string &filename) {

    //
    // STUDENT INSTRUCTION:
    //
    // Add your request to fetch a file here. Refer to the Part 1
    // student instruction for details on the basics.
    //
    // You can start with your Part 1 implementation. However, you will
    // need to adjust this method to recognize when a file trying to be
    // fetched is the same on the client (i.e. the files do not differ
    // between the client and server and a fetch would be unnecessary.
    //
    // The StatusCode response should be:
    //
    // OK - if all went well
    // DEADLINE_EXCEEDED - if the deadline timeout occurs
    // NOT_FOUND - if the file cannot be found on the server
    // ALREADY_EXISTS - if the local cached file has not changed from the server version
    // CANCELLED otherwise
    //
    // Hint: You may want to match the mtime on local files to the server's mtime
    //
    // ClientContext ctx;
    // time_point<system_clock> expirationTime = system_clock::now() + milliseconds(deadline_timeout); 
    // ctx.set_deadline(expirationTime);

    //check exists locally
    const std::string& filePath = WrapPath(filename);
    struct stat buffer;
    int fileSize;

    if (stat(filePath.c_str(), &buffer) != 0)
    {
        //do noting
    }
    else{
        fileSize = buffer.st_size;
        //check exists remotely and verify checksum
        ClientContext ctx2;
        time_point<system_clock> expirationTime2 = system_clock::now() + milliseconds(deadline_timeout); 
        ctx2.set_deadline(expirationTime2);

        dfs_service::FileName fileName;
        fileName.set_name(filename);
        dfs_service::FileInfo fileInfo;

        //request lock
        // grpc::StatusCode lockStatus;
        // if ((lockStatus = this->RequestWriteAccess(filename)) != grpc::StatusCode::OK)
        // {
        //     return lockStatus;
        // }

        grpc::Status status = service_stub->GetFileInfo(&ctx2, fileName, &fileInfo);

        uint32_t localCheckSum = dfs_file_checksum(filePath, &crc_table);
        if (localCheckSum == fileInfo.checksum())
        {
            return grpc::StatusCode::ALREADY_EXISTS;
        }
    }

    ClientContext ctx;
    time_point<system_clock> expirationTime = system_clock::now() + milliseconds(deadline_timeout); 
    ctx.set_deadline(expirationTime);

    dfs_service::FileName fileName;
    fileName.set_name(filename);
    std::unique_ptr<ClientReader<dfs_service::FilePacket>> packetReciver(service_stub -> Fetch(&ctx, fileName));

    std::ofstream fileStream;
    dfs_service::FilePacket packetHolder;
    while (packetReciver->Read(&packetHolder))
    {
        if (!fileStream.is_open())
        {
            fileStream.open(filePath, std::ofstream::binary | std::ios::trunc);
        }

        fileStream << packetHolder.packet();      
    }
    
    fileStream.close();
    grpc::Status status = packetReciver->Finish();
    if (!status.ok())
    {
        return status.error_code();
    }
    
    return StatusCode::OK;
}

// grpc::StatusCode DFSClientNodeP2::NecessaryFetch(const std::string &filename){
//     ClientContext ctx;
//     time_point<system_clock> expirationTime = system_clock::now() + milliseconds(deadline_timeout); 
//     ctx.set_deadline(expirationTime);

//     const std::string& filePath = WrapPath(filename);
//     dfs_service::FileName fileName;
//     fileName.set_name(filename);
//     std::unique_ptr<ClientReader<dfs_service::FilePacket>> packetReciver(service_stub -> Fetch(&ctx, fileName));

//     std::ofstream fileStream;
//     dfs_service::FilePacket packetHolder;
//     while (packetReciver->Read(&packetHolder))
//     {
//         if (!fileStream.is_open())
//         {
//             fileStream.open(filePath, std::ofstream::binary | std::ios::trunc);
//         }

//         fileStream << packetHolder.packet();      
//     }
    
//     fileStream.close();
//     grpc::Status status = packetReciver->Finish();
//     if (!status.ok())
//     {
//         return status.error_code();
//     }
    
//     return StatusCode::OK;
// }

grpc::StatusCode DFSClientNodeP2::Delete(const std::string &filename) {

    //
    // STUDENT INSTRUCTION:
    //
    // Add your request to delete a file here. Refer to the Part 1
    // student instruction for details on the basics.
    //
    // You will also need to add a request for a write lock before attempting to delete.
    //
    // If the write lock request fails, you should return a status of RESOURCE_EXHAUSTED
    // and cancel the current operation.
    //
    // The StatusCode response should be:
    //
    // StatusCode::OK - if all went well
    // StatusCode::DEADLINE_EXCEEDED - if the deadline timeout occurs
    // StatusCode::RESOURCE_EXHAUSTED - if a write lock cannot be obtained
    // StatusCode::CANCELLED otherwise
    //
    //
    //request lock
    grpc::StatusCode lockStatus;
    if ((lockStatus = this->RequestWriteAccess(filename)) != grpc::StatusCode::OK)
    {
        return lockStatus;
    }

    ClientContext ctx;
    time_point<system_clock> expirationTime = system_clock::now() + milliseconds(deadline_timeout); 
    ctx.set_deadline(expirationTime);

    dfs_service::FileName fileName;
    fileName.set_name(filename);
    dfs_service::DeleteReponse deleteReponse;

    grpc::Status status = service_stub->Delete(&ctx, fileName, &deleteReponse);

    if (!status.ok())
    {
        return status.error_code();
    }

    return StatusCode::OK;
}

grpc::StatusCode DFSClientNodeP2::List(std::map<std::string,int>* file_map, bool display) {

    //
    // STUDENT INSTRUCTION:
    //
    // Add your request to list files here. Refer to the Part 1
    // student instruction for details on the basics.
    //
    // You can start with your Part 1 implementation and add any additional
    // listing details that would be useful to your solution to the list response.
    //
    // The StatusCode response should be:
    //
    // StatusCode::OK - if all went well
    // StatusCode::DEADLINE_EXCEEDED - if the deadline timeout occurs
    // StatusCode::CANCELLED otherwise
    //
    //
    ClientContext ctx;
    time_point<system_clock> expirationTime = system_clock::now() + milliseconds(deadline_timeout); 
    ctx.set_deadline(expirationTime);

    dfs_service::FileInfos fileInfos;
    google::protobuf::Empty nullRequest;

    dfs_log(LL_ERROR) << "List is okay here >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>";

    grpc::Status status = service_stub->ListAll(&ctx, nullRequest, &fileInfos);

    if (!status.ok())
    {
        return status.error_code();
    }

    for (const dfs_service::FileInfo &fileInfo : fileInfos.infos())
    {
        file_map->insert(std::pair<std::string, int> (fileInfo.name(), TimeUtil::TimestampToSeconds(fileInfo.lastmodified())));
    }
    
    return StatusCode::OK;
}

grpc::StatusCode DFSClientNodeP2::Stat(const std::string &filename, void* file_status) {

    //
    // STUDENT INSTRUCTION:
    //
    // Add your request to get the status of a file here. Refer to the Part 1
    // student instruction for details on the basics.
    //
    // You can start with your Part 1 implementation and add any additional
    // status details that would be useful to your solution.
    //
    // The StatusCode response should be:
    //
    // StatusCode::OK - if all went well
    // StatusCode::DEADLINE_EXCEEDED - if the deadline timeout occurs
    // StatusCode::NOT_FOUND - if the file cannot be found on the server
    // StatusCode::CANCELLED otherwise
    //
    //
    ClientContext ctx;
    time_point<system_clock> expirationTime = system_clock::now() + milliseconds(deadline_timeout); 
    ctx.set_deadline(expirationTime);

    dfs_service::FileName fileName;
    fileName.set_name(filename);
    dfs_service::FileInfo fileInfo;

    grpc::Status status = service_stub->GetFileInfo(&ctx, fileName, &fileInfo);

    if (!status.ok())
    {
        return status.error_code();
    }

    file_status = &fileInfo;

    return StatusCode::OK;
}

void DFSClientNodeP2::InotifyWatcherCallback(std::function<void()> callback) {

    //
    // STUDENT INSTRUCTION:
    //
    // This method gets called each time inotify signals a change
    // to a file on the file system. That is every time a file is
    // modified or created.
    //
    // You may want to consider how this section will affect
    // concurrent actions between the inotify watcher and the
    // asynchronous callbacks associated with the server.
    //
    // The callback method shown must be called here, but you may surround it with
    // whatever structures you feel are necessary to ensure proper coordination
    // between the async and watcher threads.
    //
    // Hint: how can you prevent race conditions between this thread and
    // the async thread when a file event has been signaled?
    //
    //http://www.cplusplus.com/reference/mutex/mutex/lock/

    glb_mtx.lock();
    callback();
    glb_mtx.unlock();
}

//
// STUDENT INSTRUCTION:
//
// This method handles the gRPC asynchronous callbacks from the server.
// We've provided the base structure for you, but you should review
// the hints provided in the STUDENT INSTRUCTION sections below
// in order to complete this method.
//
void DFSClientNodeP2::HandleCallbackList() {

    void* tag;

    bool ok = false;

    //
    // STUDENT INSTRUCTION:
    //
    // Add your file list synchronization code here.
    //
    // When the server responds to an asynchronous request for the CallbackList,
    // this method is called. You should then synchronize the
    // files between the server and the client based on the goals
    // described in the readme.
    //
    // In addition to synchronizing the files, you'll also need to ensure
    // that the async thread and the file watcher thread are cooperating. These
    // two threads could easily get into a race condition where both are trying
    // to write or fetch over top of each other. So, you'll need to determine
    // what type of locking/guarding is necessary to ensure the threads are
    // properly coordinated.
    //

    // Block until the next result is available in the completion queue.
    while (completion_queue.Next(&tag, &ok)) {
        {
            //
            // STUDENT INSTRUCTION:
            //
            // Consider adding a critical section or RAII style lock here
            //
            glb_mtx.lock();
            // The tag is the memory location of the call_data object
            AsyncClientData<FileListResponseType> *call_data = static_cast<AsyncClientData<FileListResponseType> *>(tag);

            dfs_log(LL_DEBUG2) << "Received completion queue callback";

            // Verify that the request was completed successfully. Note that "ok"
            // corresponds solely to the request for updates introduced by Finish().
            // GPR_ASSERT(ok);
            if (!ok) {
                dfs_log(LL_ERROR) << "Completion queue callback not ok.";
            }

            if (ok && call_data->status.ok()) {

                dfs_log(LL_DEBUG3) << "Handling async callback ";

                //
                // STUDENT INSTRUCTION:
                //
                // Add your handling of the asynchronous event calls here.
                // For example, based on the file listing returned from the server,
                // how should the client respond to this updated information?
                // Should it retrieve an updated version of the file?
                // Send an update to the server?
                // Do nothing?
                //
                for (const dfs_service::FileInfo& remoteFileInfo : call_data->reply.infos())
                {
                    const std::string& clientPath = WrapPath(remoteFileInfo.name());
                    struct stat buffer;
                    int fileSize;
                    if (stat(clientPath.c_str(), &buffer) != 0)
                    {
                        this->Fetch(remoteFileInfo.name());
                        continue;
                    }
                    
                    google::protobuf::Timestamp* localModTime = new google::protobuf::Timestamp(TimeUtil::TimeTToTimestamp(buffer.st_mtime));
                    if (*localModTime > remoteFileInfo.lastmodified())
                    {
                        this->Store(remoteFileInfo.name());
                        continue;
                    }

                    if (*localModTime < remoteFileInfo.lastmodified())
                    {
                        this->Fetch(remoteFileInfo.name());
                        continue;
                    }
                    
                }

            } else {
                dfs_log(LL_ERROR) << "Status was not ok. Will try again in " << DFS_RESET_TIMEOUT << " milliseconds.";
                dfs_log(LL_ERROR) << "Status was " << call_data->status.error_code();
                dfs_log(LL_ERROR) << call_data->status.error_message();
                std::this_thread::sleep_for(std::chrono::milliseconds(DFS_RESET_TIMEOUT));
            }

            // Once we're complete, deallocate the call_data object.
            glb_mtx.unlock();
            delete call_data;

            //
            // STUDENT INSTRUCTION:
            //
            // Add any additional syncing/locking mechanisms you may need here

        }


        // Start the process over and wait for the next callback response
        dfs_log(LL_DEBUG3) << "Calling InitCallbackList";
        InitCallbackList();

    }
}

/**
 * This method will start the callback request to the server, requesting
 * an update whenever the server sees that files have been modified.
 *
 * We're making use of a template function here, so that we can keep some
 * of the more intricate workings of the async process out of the way, and
 * give you a chance to focus more on the project's requirements.
 */
void DFSClientNodeP2::InitCallbackList() {
    CallbackList<FileRequestType, FileListResponseType>();
}

//
// STUDENT INSTRUCTION:
//
// Add any additional code you need to here
//


