#include <regex>
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

#include "dfslib-shared-p1.h"
#include "dfslib-clientnode-p1.h"
#include "proto-src/dfs-service.grpc.pb.h"

using grpc::Status;
using grpc::Channel;
using grpc::StatusCode;
using grpc::ClientWriter;
using grpc::ClientReader;
using grpc::ClientContext;

using namespace std::chrono;
using namespace google::protobuf::util;

//
// STUDENT INSTRUCTION:
//
// You may want to add aliases to your namespaced service methods here.
// All of the methods will be under the `dfs_service` namespace.
//
// For example, if you have a method named MyMethod, add
// the following:
//
//      using dfs_service::MyMethod
//


DFSClientNodeP1::DFSClientNodeP1() : DFSClientNode() {}

DFSClientNodeP1::~DFSClientNodeP1() noexcept {}

StatusCode DFSClientNodeP1::Store(const std::string &filename) {

    //
    // STUDENT INSTRUCTION:
    //
    // Add your request to store a file here. This method should
    // connect to your gRPC service implementation method
    // that can accept and store a file.
    //
    // When working with files in gRPC you'll need to stream
    // the file contents, so consider the use of gRPC's ClientWriter.
    //
    // The StatusCode response should be:
    //
    // StatusCode::OK - if all went well
    // StatusCode::DEADLINE_EXCEEDED - if the deadline timeout occurs
    // StatusCode::NOT_FOUND - if the file cannot be found on the client
    // StatusCode::CANCELLED otherwise
    //
    //
    ClientContext ctx;
    ctx.AddMetadata("filename", filename);
    time_point<system_clock> expirationTime = system_clock::now() + milliseconds(deadline_timeout); 
    ctx.set_deadline(expirationTime);

    const std::string& filePath = WrapPath(filename);
    //https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c
    struct stat buffer;
    int fileSize;
    //https://stackoverflow.com/questions/16810485/cannot-convert-stdstring-to-const-char
    if (stat(filePath.c_str(), &buffer) != 0)
    {
        return StatusCode::NOT_FOUND;
    }
    else{
        fileSize = buffer.st_size;
    }
    
    //https://grpc.io/docs/languages/cpp/basics/
    dfs_service::StoreResponse response;
    std::unique_ptr<ClientWriter<dfs_service::FilePacket>> packetSender(service_stub -> Store(&ctx, &response));
    //https://www.cplusplus.com/reference/fstream/ifstream/ifstream/
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
    grpc::Status status = packetSender->Finish();

    if (!status.ok())
    {
        return StatusCode::CANCELLED;
    }
    
    return StatusCode::OK;
}


StatusCode DFSClientNodeP1::Fetch(const std::string &filename) {

    //
    // STUDENT INSTRUCTION:
    //
    // Add your request to fetch a file here. This method should
    // connect to your gRPC service implementation method
    // that can accept a file request and return the contents
    // of a file from the service.
    //
    // As with the store function, you'll need to stream the
    // contents, so consider the use of gRPC's ClientReader.
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
    //ctx.AddMetadata("FileName", filename);
    time_point<system_clock> expirationTime = system_clock::now() + milliseconds(deadline_timeout); 
    ctx.set_deadline(expirationTime);

    const std::string& filePath = WrapPath(filename);
    dfs_service::FileName fileName;
    fileName.set_name(filename);
    std::unique_ptr<ClientReader<dfs_service::FilePacket>> packetReciver(service_stub -> Fetch(&ctx, fileName));

    //read from the receiver
    std::ofstream fileStream;
    dfs_service::FilePacket packetHolder;
    //https://grpc.io/docs/languages/cpp/basics/
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

StatusCode DFSClientNodeP1::Delete(const std::string& filename) {

    //
    // STUDENT INSTRUCTION:
    //
    // Add your request to delete a file here. Refer to the Part 1
    // student instruction for details on the basics.
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
    //ctx.AddMetadata("FileName", filename);
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

StatusCode DFSClientNodeP1::List(std::map<std::string,int>* file_map, bool display) {

    //
    // STUDENT INSTRUCTION:
    //
    // Add your request to list all files here. This method
    // should connect to your service's list method and return
    // a list of files using the message type you created.
    //
    // The file_map parameter is a simple map of files. You should fill
    // the file_map with the list of files you receive with keys as the
    // file name and values as the modified time (mtime) of the file
    // received from the server.
    //
    // The StatusCode response should be:
    //
    // StatusCode::OK - if all went well
    // StatusCode::DEADLINE_EXCEEDED - if the deadline timeout occurs
    // StatusCode::CANCELLED otherwise
    //
    //
    ClientContext ctx;
    //ctx.AddMetadata("FileName", filename);
    time_point<system_clock> expirationTime = system_clock::now() + milliseconds(deadline_timeout); 
    ctx.set_deadline(expirationTime);

    dfs_service::FileInfos fileInfos;
    google::protobuf::Empty nullRequest;

    grpc::Status status = service_stub->ListAll(&ctx, nullRequest, &fileInfos);

    if (!status.ok())
    {
        return status.error_code();
    }

    for (const dfs_service::FileInfo &fileInfo : fileInfos.infos())
    {
        //http://www.cplusplus.com/reference/map/map/insert/
        //https://developers.google.com/protocol-buffers/docs/reference/cpp/google.protobuf.util.time_util
        file_map->insert(std::pair<std::string, int> (fileInfo.name(), TimeUtil::TimestampToSeconds(fileInfo.lastmodified())));
    }
    
    return StatusCode::OK;
}

StatusCode DFSClientNodeP1::Stat(const std::string &filename, void* file_status) {

    //
    // STUDENT INSTRUCTION:
    //
    // Add your request to get the status of a file here. This method should
    // retrieve the status of a file on the server. Note that you won't be
    // tested on this method, but you will most likely find that you need
    // a way to get the status of a file in order to synchronize later.
    //
    // The status might include data such as name, size, mtime, crc, etc.
    //
    // The file_status is left as a void* so that you can use it to pass
    // a message type that you defined. For instance, may want to use that message
    // type after calling Stat from another method.
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
    //ctx.AddMetadata("FileName", filename);
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

//
// STUDENT INSTRUCTION:
//
// Add your additional code here, including
// implementations of your client methods
//

