#include <map>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <getopt.h>
#include <dirent.h>
#include <sys/stat.h>
#include <grpcpp/grpcpp.h>
#include <google/protobuf/util/time_util.h>

#include "proto-src/dfs-service.grpc.pb.h"
#include "src/dfslibx-call-data.h"
#include "src/dfslibx-service-runner.h"
#include "dfslib-shared-p2.h"
#include "dfslib-servernode-p2.h"

using grpc::Status;
using grpc::Server;
using grpc::StatusCode;
using grpc::ServerReader;
using grpc::ServerWriter;
using grpc::ServerContext;
using grpc::ServerBuilder;

using dfs_service::DFSService;

using namespace std::chrono;
using namespace google::protobuf::util;


//
// STUDENT INSTRUCTION:
//
// Change these "using" aliases to the specific
// message types you are using in your `dfs-service.proto` file
// to indicate a file request and a listing of files from the server
//
using FileRequestType = dfs_service::FileName;
using FileListResponseType = dfs_service::FileInfos;

extern dfs_log_level_e DFS_LOG_LEVEL;

//
// STUDENT INSTRUCTION:
//
// As with Part 1, the DFSServiceImpl is the implementation service for the rpc methods
// and message types you defined in your `dfs-service.proto` file.
//
// You may start with your Part 1 implementations of each service method.
//
// Elements to consider for Part 2:
//
// - How will you implement the write lock at the server level?
// - How will you keep track of which client has a write lock for a file?
//      - Note that we've provided a preset client_id in DFSClientNode that generates
//        a client id for you. You can pass that to the server to identify the current client.
// - How will you release the write lock?
// - How will you handle a store request for a client that doesn't have a write lock?
// - When matching files to determine similarity, you should use the `file_checksum` method we've provided.
//      - Both the client and server have a pre-made `crc_table` variable to speed things up.
//      - Use the `file_checksum` method to compare two files, similar to the following:
//
//          std::uint32_t server_crc = dfs_file_checksum(filepath, &this->crc_table);
//
//      - Hint: as the crc checksum is a simple integer, you can pass it around inside your message types.
//
class DFSServiceImpl final :
    public DFSService::WithAsyncMethod_CallbackList<DFSService::Service>,
        public DFSCallDataManager<FileRequestType , FileListResponseType> {

private:

    /** The runner service used to start the service and manage asynchronicity **/
    DFSServiceRunner<FileRequestType, FileListResponseType> runner;

    /** The mount path for the server **/
    std::string mount_path;

    /** Mutex for managing the queue requests **/
    std::mutex queue_mutex;

    /** The vector of queued tags used to manage asynchronous requests **/
    std::vector<QueueRequest<FileRequestType, FileListResponseType>> queued_tags;


    /**
     * Prepend the mount path to the filename.
     *
     * @param filepath
     * @return
     */
    const std::string WrapPath(const std::string &filepath) {
        return this->mount_path + filepath;
    }

    //std::map<std::string, std::string>* Dummy;
    std::map<std::string, std::string> fileWriteLockHolders;
    std::mutex fileWriteLocksMutex;
    std::mutex listLock;

    /** CRC Table kept in memory for faster calculations **/
    CRC::Table<std::uint32_t, 32> crc_table;

    bool lockFileForClient(std::string fileName, std::string clientID){
        //https://thispointer.com/how-check-if-a-given-key-exists-in-a-map-c/
        fileWriteLocksMutex.lock();
        std::string client = std::string(fileWriteLockHolders.find(fileName)->second.begin(), fileWriteLockHolders.find(fileName)->second.end());
        if (fileWriteLockHolders.find(fileName) == fileWriteLockHolders.end() || client.compare(clientID) == 0)
        {
            fileWriteLockHolders.insert({fileName, clientID});
        }
        else{
            fileWriteLocksMutex.unlock();
            return false;
        }
        
        fileWriteLocksMutex.unlock();
        return true;
    }

    void releaseFileLock(std::string fileName){
        fileWriteLocksMutex.lock();
        fileWriteLockHolders.erase(fileName);
        fileWriteLocksMutex.unlock();
    }

public:

    DFSServiceImpl(const std::string& mount_path, const std::string& server_address, int num_async_threads):
        mount_path(mount_path), crc_table(CRC::CRC_32()) {

        this->runner.SetService(this);
        this->runner.SetAddress(server_address);
        this->runner.SetNumThreads(num_async_threads);
        this->runner.SetQueuedRequestsCallback([&]{ this->ProcessQueuedRequests(); });

    }

    ~DFSServiceImpl() {
        this->runner.Shutdown();
    }

    void Run() {
        this->runner.Run();
    }

    /**
     * Request callback for asynchronous requests
     *
     * This method is called by the DFSCallData class during
     * an asynchronous request call from the client.
     *
     * Students should not need to adjust this.
     *
     * @param context
     * @param request
     * @param response
     * @param cq
     * @param tag
     */
    void RequestCallback(grpc::ServerContext* context,
                         FileRequestType* request,
                         grpc::ServerAsyncResponseWriter<FileListResponseType>* response,
                         grpc::ServerCompletionQueue* cq,
                         void* tag) {

        std::lock_guard<std::mutex> lock(queue_mutex);
        this->queued_tags.emplace_back(context, request, response, cq, tag);

    }

    /**
     * Process a callback request
     *
     * This method is called by the DFSCallData class when
     * a requested callback can be processed. You should use this method
     * to manage the CallbackList RPC call and respond as needed.
     *
     * See the STUDENT INSTRUCTION for more details.
     *
     * @param context
     * @param request
     * @param response
     */
    void ProcessCallback(ServerContext* context, FileRequestType* request, FileListResponseType* response) {

        //
        // STUDENT INSTRUCTION:
        //
        // You should add your code here to respond to any CallbackList requests from a client.
        // This function is called each time an asynchronous request is made from the client.
        //
        // The client should receive a list of files or modifications that represent the changes this service
        // is aware of. The client will then need to make the appropriate calls based on those changes.
        //
        grpc::Status status = this->CallbackList(context, request, response);

        if (!status.ok())
        {
            dfs_log(LL_DEBUG2) << "ProcessCallback fails";
        }        
    }

    /**
     * Processes the queued requests in the queue thread
     */
    void ProcessQueuedRequests() {
        while(true) {

            //
            // STUDENT INSTRUCTION:
            //
            // You should add any synchronization mechanisms you may need here in
            // addition to the queue management. For example, modified files checks.
            //
            // Note: you will need to leave the basic queue structure as-is, but you
            // may add any additional code you feel is necessary.
            //


            // Guarded section for queue
            {
                dfs_log(LL_DEBUG2) << "Waiting for queue guard";
                std::lock_guard<std::mutex> lock(queue_mutex);


                for(QueueRequest<FileRequestType, FileListResponseType>& queue_request : this->queued_tags) {
                    this->RequestCallbackList(queue_request.context, queue_request.request,
                        queue_request.response, queue_request.cq, queue_request.cq, queue_request.tag);
                    queue_request.finished = true;
                }

                // any finished tags first
                this->queued_tags.erase(std::remove_if(
                    this->queued_tags.begin(),
                    this->queued_tags.end(),
                    [](QueueRequest<FileRequestType, FileListResponseType>& queue_request) { return queue_request.finished; }
                ), this->queued_tags.end());

            }
        }
    }

    //
    // STUDENT INSTRUCTION:
    //
    // Add your additional code here, including
    // the implementations of your rpc protocol methods.
    //

    // 1. REQUIRED (Parts 1 & 2): A method to store files on the server
    ::grpc::Status Store(::grpc::ServerContext* context, ::grpc::ServerReader< ::dfs_service::FilePacket>* reader, ::dfs_service::StoreResponse* response) override{
        const std::multimap<grpc::string_ref, grpc::string_ref> metadata = context->client_metadata();
        //https://stackoverflow.com/questions/41583218/grpc-client-id-or-connection-information
        auto iter = metadata.find("filename");
        auto fileName = std::string(iter->second.begin(), iter->second.end());
        const std::string& filePath = WrapPath(fileName);

        //if deadline exceeded
        //https://grpc.io/blog/deadlines/
        if (context->IsCancelled()) {
            return grpc::Status(StatusCode::DEADLINE_EXCEEDED, "Deadline exceeded.");
        }

        dfs_service::FilePacket packetHolder;
        std::ofstream fileStream;

        while (reader->Read(&packetHolder))
        {
            if (!fileStream.is_open())
            {
                fileStream.open(filePath, std::ofstream::binary | std::ios::trunc);
            }

            fileStream << packetHolder.packet();      
        }
        
        fileStream.close();
        response->set_issuccessful(true);
        releaseFileLock(fileName);
        return grpc::Status(StatusCode::OK, "Successfully stored.");
    }
    // 2. REQUIRED (Parts 1 & 2): A method to fetch files from the server
    ::grpc::Status Fetch(::grpc::ServerContext* context, const ::dfs_service::FileName* request, ::grpc::ServerWriter< ::dfs_service::FilePacket>* writer) override{
        const std::string fileName = request->name();
        const std::string& filePath = WrapPath(fileName);

        if (context->IsCancelled()) {
            return grpc::Status(StatusCode::DEADLINE_EXCEEDED, "Deadline exceeded.");
        }

        struct stat buffer;
        int fileSize;
        if (stat(filePath.c_str(), &buffer) != 0)
        {
            return grpc::Status(StatusCode::NOT_FOUND, "File not found on server.");
        }
        else{
            fileSize = buffer.st_size;
        }

        std::ifstream fileStream(filePath, std::ifstream::binary);
        dfs_service::FilePacket packetHolder;
        int bytesTransferred = 0;

        while (bytesTransferred < fileSize && !fileStream.eof())
        {
            std::vector<char> senderBuffer(PacketSize);
            int bytesToTransfer = std::min(fileSize - bytesTransferred, PacketSize);
            fileStream.read(senderBuffer.data(), bytesToTransfer);
            packetHolder.set_packet(senderBuffer.data(), bytesToTransfer);
            writer->Write(packetHolder);
            bytesTransferred += bytesToTransfer;
        }
        
        fileStream.close();
        return grpc::Status(StatusCode::OK, "Successfully fetched.");
    }
    // 3. REQUIRED (Parts 1 & 2): A method to list all files on the server
    ::grpc::Status ListAll(::grpc::ServerContext* context, const ::google::protobuf::Empty* request, ::dfs_service::FileInfos* response) override{
        // if (context->IsCancelled()) {
        //     return grpc::Status(StatusCode::DEADLINE_EXCEEDED, "Deadline exceeded.");
        // }

        DIR *currentDir;
        struct dirent *entry;

        listLock.lock();
        if ((currentDir = opendir(mount_path.c_str())) == NULL)
        {
            listLock.unlock();
            return grpc::Status(StatusCode::CANCELLED, "Directory invalid.");
        }

        while ((entry = readdir(currentDir)) != NULL)
        {
            std::string fileName = entry->d_name;
            std::string filePath = WrapPath(fileName);
            struct stat buffer;
            int fileSize;
            google::protobuf::Timestamp*  mod_time;
            google::protobuf::Timestamp*  create_time;
            if (stat(filePath.c_str(), &buffer) != 0)
            {
                listLock.unlock();
                return grpc::Status(StatusCode::NOT_FOUND, "File not found on server.");
            }
            else{
                fileSize = buffer.st_size;
                //https://stackoverflow.com/questions/40504281/c-how-to-check-the-last-modified-time-of-a-file
                mod_time = new google::protobuf::Timestamp(TimeUtil::TimeTToTimestamp(buffer.st_mtime));
                create_time = new google::protobuf::Timestamp(TimeUtil::TimeTToTimestamp(buffer.st_ctime));
            }

            dfs_service::FileInfo* fileInfo = response->add_infos();
            fileInfo->set_name(fileName);
            fileInfo->set_size(fileSize);
            fileInfo->set_allocated_lastmodified(mod_time);
            fileInfo->set_allocated_created(create_time);  
            fileInfo->set_checksum(dfs_file_checksum(filePath, &crc_table));  
        }
        
        closedir(currentDir);
        listLock.unlock();
        
        return grpc::Status(StatusCode::OK, "Success.");
    }
    // 4. REQUIRED (Parts 1 & 2): A method to get the status of a file on the server
    ::grpc::Status GetFileInfo(::grpc::ServerContext* context, const ::dfs_service::FileName* request, ::dfs_service::FileInfo* response) override{
        const std::string fileName = request->name();
        const std::string& filePath = WrapPath(fileName);

        if (context->IsCancelled()) {
            return grpc::Status(StatusCode::DEADLINE_EXCEEDED, "Deadline exceeded.");
        }

        struct stat buffer;
        int fileSize;
        google::protobuf::Timestamp*  mod_time;
        google::protobuf::Timestamp*  create_time;
        if (stat(filePath.c_str(), &buffer) != 0)
        {
            return grpc::Status(StatusCode::NOT_FOUND, "File not found on server.");
        }
        else{
            fileSize = buffer.st_size;
            //https://stackoverflow.com/questions/40504281/c-how-to-check-the-last-modified-time-of-a-file
            mod_time = new google::protobuf::Timestamp(TimeUtil::TimeTToTimestamp(buffer.st_mtime));
            create_time = new google::protobuf::Timestamp(TimeUtil::TimeTToTimestamp(buffer.st_ctime));
        }

        response->set_name(request->name());
        response->set_size(fileSize);
        response->set_allocated_lastmodified(mod_time);
        response->set_allocated_created(create_time);
        response->set_checksum(dfs_file_checksum(filePath, &crc_table));

        return grpc::Status(StatusCode::OK, "Success.");
    }
    // 5. REQUIRED (Part 2 only): A method to request a write lock from the server
    ::grpc::Status RequestWriteLock(::grpc::ServerContext* context, const ::dfs_service::WriteLockRequest* request, ::dfs_service::WriteLockResponse* response) override{
        if (context->IsCancelled()) {
            return grpc::Status(StatusCode::DEADLINE_EXCEEDED, "Deadline exceeded.");
        }

        std::string fileName = request->name();
        std::string clientID = request->clientid();

        if (lockFileForClient(fileName, clientID))
        {
            return grpc::Status(StatusCode::OK, "Lock Success.");
        }

        return grpc::Status(StatusCode::RESOURCE_EXHAUSTED, "Lock Fails.");
    }
    // 6. REQUIRED (Part 2 only): A method named CallbackList to handle asynchronous file listing requests
    //                            from a client. This method should return a listing of files along with their
    //                            attribute information. The expected attribute information should include name,
    //                            size, modified time, and creation time.
    ::grpc::Status CallbackList(::grpc::ServerContext* context, const ::dfs_service::FileName* request, ::dfs_service::FileInfos* response) override{
        google::protobuf::Empty nullRequest;
        return this->ListAll(context, &nullRequest, response);
    }
    // 7. REQUIRED (Part 2 only): A method to delete a file from the server
    ::grpc::Status Delete(::grpc::ServerContext* context, const ::dfs_service::FileName* request, ::dfs_service::DeleteReponse* response) override{
        const std::string fileName = request->name();
        const std::string& filePath = WrapPath(fileName);

        if (context->IsCancelled()) {
            return grpc::Status(StatusCode::DEADLINE_EXCEEDED, "Deadline exceeded.");
        }

        struct stat buffer;
        int fileSize;
        if (stat(filePath.c_str(), &buffer) != 0)
        {
            return grpc::Status(StatusCode::NOT_FOUND, "File not found on server.");
        }
        else{
            fileSize = buffer.st_size;
        }

        //https://www.cplusplus.com/reference/cstdio/remove/
        if (remove(filePath.c_str()) != 0)
        {
            return grpc::Status(StatusCode::CANCELLED, "Error deleting.");
        }
        
        response->set_issuccessful(true);
        // if (releaseFileLock(fileName)) {
        //     return grpc::Status(StatusCode::CANCELLED, "Fail to release lock.");
        // }
        releaseFileLock(fileName);
        return grpc::Status(StatusCode::OK, "Successfully deleted.");
    }
};

//
// STUDENT INSTRUCTION:
//
// The following three methods are part of the basic DFSServerNode
// structure. You may add additional methods or change these slightly
// to add additional startup/shutdown routines inside, but be aware that
// the basic structure should stay the same as the testing environment
// will be expected this structure.
//
/**
 * The main server node constructor
 *
 * @param mount_path
 */
DFSServerNode::DFSServerNode(const std::string &server_address,
        const std::string &mount_path,
        int num_async_threads,
        std::function<void()> callback) :
        server_address(server_address),
        mount_path(mount_path),
        num_async_threads(num_async_threads),
        grader_callback(callback) {}
/**
 * Server shutdown
 */
DFSServerNode::~DFSServerNode() noexcept {
    dfs_log(LL_SYSINFO) << "DFSServerNode shutting down";
}

/**
 * Start the DFSServerNode server
 */
void DFSServerNode::Start() {
    DFSServiceImpl service(this->mount_path, this->server_address, this->num_async_threads);


    dfs_log(LL_SYSINFO) << "DFSServerNode server listening on " << this->server_address;
    service.Run();
}

//
// STUDENT INSTRUCTION:
//
// Add your additional definitions here
//
