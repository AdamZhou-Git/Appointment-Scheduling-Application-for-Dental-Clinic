#include <map>
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
#include <experimental/filesystem>

#include "src/dfs-utils.h"
#include "dfslib-shared-p1.h"
#include "dfslib-servernode-p1.h"
#include "proto-src/dfs-service.grpc.pb.h"

using grpc::Status;
using grpc::Server;
using grpc::StatusCode;
using grpc::ServerReader;
using grpc::ServerWriter;
using grpc::ServerContext;
using grpc::ServerBuilder;

using dfs_service::DFSService;

using namespace google::protobuf::util;

#ifdef __APPLE__
#ifndef st_mtime
#define st_mtime st_mtimespec.tv_sec
#endif
#endif

#ifdef __APPLE__
#ifndef st_ctime
#define st_ctime st_ctimespec.tv_sec
#endif
#endif


//
// STUDENT INSTRUCTION:
//
// DFSServiceImpl is the implementation service for the rpc methods
// and message types you defined in the `dfs-service.proto` file.
//
// You should add your definition overrides here for the specific
// methods that you created for your GRPC service protocol. The
// gRPC tutorial described in the readme is a good place to get started
// when trying to understand how to implement this class.
//
// The method signatures generated can be found in `proto-src/dfs-service.grpc.pb.h` file.
//
// Look for the following section:
//
//      class Service : public ::grpc::Service {
//
// The methods returning grpc::Status are the methods you'll want to override.
//
// In C++, you'll want to use the `override` directive as well. For example,
// if you have a service method named MyMethod that takes a MyMessageType
// and a ServerWriter, you'll want to override it similar to the following:
//
//      Status MyMethod(ServerContext* context,
//                      const MyMessageType* request,
//                      ServerWriter<MySegmentType> *writer) override {
//
//          /** code implementation here **/
//      }
//
class DFSServiceImpl final : public DFSService::Service {

private:

    /** The mount path for the server **/
    std::string mount_path;

    /**
     * Prepend the mount path to the filename.
     *
     * @param filepath
     * @return
     */
    const std::string WrapPath(const std::string &filepath) {
        return this->mount_path + filepath;
    }


public:

    DFSServiceImpl(const std::string &mount_path): mount_path(mount_path) {
    }

    ~DFSServiceImpl() {}

    //
    // STUDENT INSTRUCTION:
    //
    // Add your additional code here, including
    // implementations of your protocol service methods
    //
    grpc::Status Store(::grpc::ServerContext* context, ::grpc::ServerReader< ::dfs_service::FilePacket>* reader, ::dfs_service::StoreResponse* response) override{
        //https://grpc.io/docs/languages/cpp/basics/
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
        return grpc::Status(StatusCode::OK, "Successfully stored.");
    }
    // 2. REQUIRED (Parts 1 & 2): A method to fetch files from the server
    grpc::Status Fetch(::grpc::ServerContext* context, const ::dfs_service::FileName* request, ::grpc::ServerWriter< ::dfs_service::FilePacket>* writer) override{
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
    // 3. REQUIRED (Parts 1 & 2): A method to delete files from the server
    grpc::Status Delete(::grpc::ServerContext* context, const ::dfs_service::FileName* request, ::dfs_service::DeleteReponse* response) override{
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
        return grpc::Status(StatusCode::OK, "Successfully deleted.");
    }
    // 4. REQUIRED (Parts 1 & 2): A method to list all files on the server
    grpc::Status ListAll(::grpc::ServerContext* context, const ::google::protobuf::Empty* request, ::dfs_service::FileInfos* response) override{
        if (context->IsCancelled()) {
            return grpc::Status(StatusCode::DEADLINE_EXCEEDED, "Deadline exceeded.");
        }

        // if (!std::experimental::filesystem::exists(mount_path))
        // {
        //     return grpc::Status(StatusCode::CANCELLED, "Directory invalid.");
        // }
        
        //https://stackoverflow.com/questions/612097/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c
        //experimental::filesystem not working
        // for (const auto & entry : std::experimental::filesystem::directory_iterator(mount_path))
        // {
        //     std::string filePath = entry.path();
        //     std::string fileName = entry.path().filename();

            // struct stat buffer;
            // int fileSize;
            // google::protobuf::Timestamp*  mod_time;
            // google::protobuf::Timestamp*  create_time;
            // if (stat(filePath.c_str(), &buffer) != 0)
            // {
            //     return grpc::Status(StatusCode::NOT_FOUND, "File not found on server.");
            // }
            // else{
            //     fileSize = buffer.st_size;
            //     //https://stackoverflow.com/questions/40504281/c-how-to-check-the-last-modified-time-of-a-file
            //     mod_time = new google::protobuf::Timestamp(TimeUtil::TimeTToTimestamp(buffer.st_mtime));
            //     create_time = new google::protobuf::Timestamp(TimeUtil::TimeTToTimestamp(buffer.st_ctime));
            // }

            // dfs_service::FileInfo* fileInfo = response->add_infos();
            // fileInfo->set_name(fileName);
            // fileInfo->set_size(fileSize);
            // fileInfo->set_allocated_lastmodified(mod_time);
            // fileInfo->set_allocated_created(create_time);    
        // }
        DIR *currentDir;
        struct dirent *entry;

        if ((currentDir = opendir(mount_path.c_str())) == NULL)
        {
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
        }
        
        closedir(currentDir);
        
        return grpc::Status(StatusCode::OK, "Success.");
    }
    // 5. REQUIRED (Parts 1 & 2): A method to get the status of a file on the server
    grpc::Status GetFileInfo(::grpc::ServerContext* context, const ::dfs_service::FileName* request, ::dfs_service::FileInfo* response)override{
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

        return grpc::Status(StatusCode::OK, "Success.");
    }
};

//
// STUDENT INSTRUCTION:
//
// The following three methods are part of the basic DFSServerNode
// structure. You may add additional methods or change these slightly,
// but be aware that the testing environment is expecting these three
// methods as-is.
//
/**
 * The main server node constructor
 *
 * @param server_address
 * @param mount_path
 */
DFSServerNode::DFSServerNode(const std::string &server_address,
        const std::string &mount_path,
        std::function<void()> callback) :
    server_address(server_address), mount_path(mount_path), grader_callback(callback) {}

/**
 * Server shutdown
 */
DFSServerNode::~DFSServerNode() noexcept {
    dfs_log(LL_SYSINFO) << "DFSServerNode shutting down";
    this->server->Shutdown();
}

/** Server start **/
void DFSServerNode::Start() {
    DFSServiceImpl service(this->mount_path);
    ServerBuilder builder;
    builder.AddListeningPort(this->server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    this->server = builder.BuildAndStart();
    dfs_log(LL_SYSINFO) << "DFSServerNode server listening on " << this->server_address;
    this->server->Wait();
}

//
// STUDENT INSTRUCTION:
//
// Add your additional DFSServerNode definitions here
//

