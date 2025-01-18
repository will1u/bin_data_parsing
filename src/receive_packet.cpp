#include <iostream>
#include <fstream>
#include <bitset>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <vector>
#include <sstream>
#include <thread>
#include <cstdlib>

#include "data_util.h"
#include "packet.h"
#include "timing_util.h"
#include "directory_util.h"

using namespace std;

void handleClient(int client_socket, int connection_num) {

    // Receive file size
    uint32_t fileSizeN;
    ssize_t size_received = recv(client_socket, &fileSizeN, sizeof(fileSizeN), 0);
    if (size_received <= 0) {
        std::cerr << "Error receiving file size!\n";
        close(client_socket);
        return;
    }

    // Receive file data
    uint32_t fileSize = ntohl(fileSizeN);
    std::vector<char> buffer(fileSize);
    ssize_t total_received = 0;
    std::string timestamp;
    while (total_received < (ssize_t)fileSize) {
        ssize_t bytes = recv(client_socket, buffer.data() + total_received, fileSize - total_received, 0);
        timestamp = TimingUtil::getTime();
        
        if (bytes <= 0) {
            std::cerr << "Error receiving file data!\n";
            close(client_socket);
            return;
        }
        total_received += bytes;
    }

    // Send a response to the client
    std::string response = "File received successfully!";
    send(client_socket, response.c_str(), response.size(), 0);


    // Close client socket 
    close(client_socket);
    std::cout << "Client socket closed.\n"; 


    // Write file data to disk
    std::cout << "Writing client data to disk.\n"; 
    string pathToDirectory = "~/projects/data_parsing/partitioned_data/";
    pathToDirectory = DirectoryUtil::expandTilde(pathToDirectory);

    string receivedFileName = pathToDirectory + "received_file_" + std::to_string(connection_num) + ".dat";
    
    if (!std::filesystem::exists(pathToDirectory)) {
        std::filesystem::create_directories(pathToDirectory);
    }
    
    std::ofstream outFile(receivedFileName, std::ios::binary);
    if (!outFile) {
        std::cerr << "Could not create output file.\n";
        return;
    }

    else {
        outFile.write(buffer.data(), buffer.size());
        std::cout << "Wrote " << buffer.size() << " bytes to " + receivedFileName + ".\n";
    }

}

int main() {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        cerr << "Error creating socket!" << endl;
        return -1;
    }

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    
    
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8001);
    server_address.sin_addr.s_addr = INADDR_ANY;

    
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        cerr << "Error binding to port!" << endl;
        close(server_socket);
        return -1;
    }

    if (listen(server_socket, 5) < 0) {
        cerr << "Error listening on socket!" << endl;
        close(server_socket);
        return -1;
    }

    std::cout << "Server is listening on port 8001..." << std::endl;

    // Accept connections in a loop
    int connections = 0;
    
    while (connections < 3) {
        
        cout << "debug pt 1" << endl;
        struct sockaddr_in client_address;
        socklen_t client_len = sizeof(client_address);

        // This call will block until a new client connects
        int client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_len);
        
        cout << "debug pt 2" << endl;
        if (client_socket == -1) {
            std::cerr << "Error accepting connection!" << std::endl;
            break;
        }

        std::cout << "Client connected!" << std::endl;

        // Spawn a new thread for this client
        cout << "debug pt 3" << endl;
        std::thread t(handleClient, client_socket, connections + 1); 
        t.detach();
        connections += 1;

        cout << "----------------------------------" << endl;
        cout << connections << " connections accepted." << endl;
        cout << "----------------------------------" << endl;
        
    }
    
    // Close server socket
    close(server_socket);

    return 0;
}
