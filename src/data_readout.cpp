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
#include <fcntl.h>
#include <thread>

#include "data_util.h"
#include "packet.h"
#include "directory_util.h"


using namespace std;

int sendFile(const string &filePath) {
    ifstream file(filePath, ios::binary | ios::ate);

    if (!file) {
        cerr << "Could not open file: " << filePath << endl;
        return -1;
    }

    streamsize size = file.tellg();
    file.seekg(0, ios::beg);

    vector<char> fileData(size);
    if (!file.read(fileData.data(), size)) {
        std::cerr << "Error: Failed to read file" << std::endl;
        return -1;
    }


    int network_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (network_socket == -1) {
        cerr << "Error creating socket!" << endl;
        return -1;
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8001);  // Port number
    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
        cerr << "Invalid address!" << endl;
        close(network_socket);
        return -1;
    }

    int connection_status = connect(network_socket, (struct sockaddr *)&server_address, sizeof(server_address));
    if (connection_status == -1) {
        cerr << "Error connecting to server!" << endl;
        close(network_socket);
        return -1;
    }


    // Send file size
    uint32_t fileSizeN = htonl(static_cast<uint32_t>(size));
    ssize_t sentBytes = send(network_socket, &fileSizeN, sizeof(fileSizeN), 0);
    if (sentBytes < 0) {
        std::cerr << "Error sending file size!" << std::endl;
        close(network_socket);
        return -1;
    }

    // Send file data
    sentBytes = send(network_socket, fileData.data(), fileData.size(), 0);
    if (sentBytes < 0) {
        std::cerr << "Error sending file data!" << std::endl;
        close(network_socket);
        return -1;
    }

    std::cout << "Sent " << sentBytes << " bytes of raw file data." << std::endl;

    // Optionally, receive a response from the server
    char server_response[256];
    recv(network_socket, &server_response, sizeof(server_response), 0);
    cout << "Server response: " << server_response << endl;

    // Close the socket
    close(network_socket);
    return 0;
}


int main() {

    
    int numFiles = 3;

    vector<thread> threads; // Vector to hold threads

    for (int i = 1; i <= numFiles; i++) {
        // Generate file path
        string directoryPath = "~/projects/data_parsing/tests/";
        directoryPath = DirectoryUtil::expandTilde(directoryPath);
        string filePath = directoryPath + "file-" + std::to_string(i) + ".dat";

        // Launch a thread for each file
        threads.emplace_back(sendFile, filePath);
    }

    // Wait for all threads to complete
    for (auto &t : threads) {
        t.join();
    }

    cout << "All files have been sent." << endl;

    return 0;
}