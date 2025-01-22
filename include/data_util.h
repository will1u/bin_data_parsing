#pragma once
#include <string>
#include <vector>
#include <bitset>
#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <algorithm>
#include <atomic>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <cstdint>


#include "timing_util.h"
#include "directory_util.h"

namespace DataUtil {

    struct DataPoint {
        uint32_t timestamp; // 28-bit field
        uint8_t hitcount;   // 5-bit field
        std::vector<uint16_t> hits; // 10-bit fields

        void serialize(std::ostream& os) const {
            // Write 28 bits of timestamp by splitting across bytes
            uint32_t timestampMask = timestamp & ((1 << 28) - 1);
            os.write(reinterpret_cast<const char*>(&timestampMask), 4); // Write 4 bytes for timestamp (upper bits will be zeroed)

            // Write 5 bits of hitcount
            uint8_t hitcountMask = hitcount & 0x1F; // Mask to 5 bits
            os.write(reinterpret_cast<const char*>(&hitcountMask), 1); // Write 1 byte for hitcount

            // Write each hit as 10 bits, we’ll need to split bits into bytes for each hit
            for (uint16_t hit : hits) {
                uint16_t hitMask = hit & 0x3FF; // Mask to 10 bits
                os.write(reinterpret_cast<const char*>(&hitMask), 2); // Write 2 bytes for each 10-bit hit
            }
        }

        void deserialize(std::istream& is) {
            // Read 28 bits for timestamp
            uint32_t timestampMask;
            is.read(reinterpret_cast<char*>(&timestampMask), 4); // Read 4 bytes
            timestamp = timestampMask & ((1 << 28) - 1); // Mask to 28 bits

            // Read 5 bits for hitcount
            uint8_t hitcountMask;
            is.read(reinterpret_cast<char*>(&hitcountMask), 1); // Read 1 byte
            hitcount = hitcountMask & 0x1F; // Mask to 5 bits

            // Read each hit as 10 bits
            hits.resize(hitcount);
            for (uint16_t &hit : hits) {
                uint16_t hitMask;
                is.read(reinterpret_cast<char*>(&hitMask), 2); // Read 2 bytes
                hit = hitMask & 0x3FF; // Mask to 10 bits
            }
        }

        void print() const {
            std::cout << "Timestamp (Decimal): " << timestamp << "\n";
            std::cout << "Hit Count (Decimal): " << static_cast<int>(hitcount) << "\n";

            // Print binary representations
            std::cout << "Timestamp (Binary): " << std::bitset<28>(timestamp) << "\n"; // 28 bits
            std::cout << "Hit Count (Binary): " << std::bitset<5>(hitcount) << "\n";   // 5 bits

            std::cout << "Hits (Binary): ";
            for (const auto& hit : hits) {
                std::cout << std::bitset<10>(hit) << " "; // Each hit as 10 bits
            }
            std::cout << "\n----------------------------------\n";
        }

        bool operator<(const DataPoint& other) const {
            return timestamp < other.timestamp;
        }
    };

    std::vector<uint8_t> readFileIntoBuffer(const std::string& filePath) {
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (!file) {
            std::cerr << "Could not open file: " << filePath << std::endl;
            return {};
        }
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> buffer(size);
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            std::cerr << "Error reading file into buffer.\n";
            return {};
        }
        return buffer;
    }

    uint32_t readBuffer(const std::vector<uint8_t>& buffer, size_t& bit_pos, int count) {
        // Make sure we don't go out of range
        // (buffer.size() * 8) gives total number of bits in the buffer
        if (bit_pos + count > buffer.size() * 8) {
            throw std::runtime_error("Not enough bits left in buffer to read!");
    }

        uint32_t value = 0;
        for (int i = 0; i < count; i++) {
            // Which byte are we in?
            size_t byteIndex = bit_pos / 8;
            // Which bit within that byte? (0=most significant, 7=least significant for an 8-bit byte)
            size_t bitIndex = bit_pos % 8;

            // Extract the bit. Let's treat bit 0 as the *leftmost* or most significant bit of the byte:
            //    buffer[byteIndex] >> (7 - bitIndex) & 1
            // Then we shift 'value' left and add the bit in.
            uint8_t theBit = (buffer[byteIndex] >> (7 - bitIndex)) & 0x1;
            value = (value << 1) | theBit;

            bit_pos++;
        }
        return value;
}   

    std::vector<DataPoint> readBits(const std::string& filePath) {

        std::vector<uint8_t> buffer = readFileIntoBuffer(filePath);
        if (buffer.empty()) {
            return {};
        }

        std::vector<DataPoint> data_list;
        size_t bit_pos = 0; // tracks which bit we're on in the buffer

        // 2. Loop until we don't have enough bits for at least the 28-bit timestamp + 5-bit count
        while (bit_pos + 28 + 5 <= buffer.size() * 8) {
            DataPoint data_point;
            // read 28 bits for timestamp
            data_point.timestamp = readBuffer(buffer, bit_pos, 28);

            // read 5 bits for hitcount
            data_point.hitcount = static_cast<uint8_t>(readBuffer(buffer, bit_pos, 5));

            // 3. read 'hitcount' hits (10 bits each)
            data_point.hits.resize(data_point.hitcount);
            for (int i = 0; i < data_point.hitcount; i++) {
                // Make sure we have enough bits left for 10 bits
                if (bit_pos + 10 > buffer.size() * 8) {
                    // We can't fully read all hits, so break or throw
                    break;
                }
                data_point.hits[i] = static_cast<uint16_t>(readBuffer(buffer, bit_pos, 10));
            }

            data_list.push_back(data_point);
        }

        return data_list;
    }

    void printDataPoints(const std::vector<DataPoint>& dataSets) {
        for (const auto& dataSet : dataSets) {
            dataSet.print();
        }
    }
    
    std::string combineDataBinary(const DataPoint& dp) {
        // We'll gather everything into a single string.
        // The timestamp is 28 bits, hitcount is 5 bits, and each hit is 10 bits.
        // We'll represent each as a bitset and concatenate them.
        
        std::ostringstream oss;

        // Timestamp (28 bits)
        oss << std::bitset<28>(dp.timestamp).to_string() << " ";

        // Hitcount (5 bits)
        oss << std::bitset<5>(dp.hitcount).to_string() << " ";

        // Each hit (10 bits)
        for (auto hit : dp.hits) {
            oss << std::bitset<10>(hit).to_string() << " ";
        }

        return oss.str();
    }
    
    int partitionToOutput(std::string outputFile, std::vector<DataPoint> dataList, bool append = false) {
        

        std::ofstream outFile;
        
        if (append == true){
            outFile.open(outputFile, std::ios::app);
        }
        else{
            outFile.open(outputFile);
        }

        if (!outFile){
            std::cerr << "could not open output file." << std::endl;
            return -1;
        }

        std::cout << " \n" << std::endl;
        
        for (auto i: dataList){
            std::string line = combineDataBinary(i);
            outFile << line << std::endl;

            outFile.flush();

        }
        std::cout << "successfully written to " << outputFile << std::endl;

        return 0;
    }

    std::string concatenateReadBits(std::vector<std::string> data_list) {
        std::string result;
        for (auto i : data_list){
            result += i;
        }

        return result;
    }

    std::vector<DataUtil::DataPoint> watchDirectory(const std::string &directoryPath, std::atomic<bool> &stop) {
        
        // scans for changes in raw data directory and puts them into a vector<DataUtil::DataPoint>
        
        std::unordered_map<std::string, std::filesystem::file_time_type> files;
        std::vector<DataUtil::DataPoint> result_data_list;

        for (const auto &entry : std::filesystem::directory_iterator(directoryPath)) {
            files[entry.path().string()] = std::filesystem::last_write_time(entry);
        }
        
        int newFileCounter = 0;
        while (!stop.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); 

            for (const auto &entry : std::filesystem::directory_iterator(directoryPath)) {
                auto currentFileTime = std::filesystem::last_write_time(entry);
                const std::string filePath = entry.path().string();

                if (files.find(filePath) == files.end()) {
                    std::cout << std::to_string(newFileCounter + 1) << ": New file detected: " << filePath << std::endl;
                    newFileCounter += 1;
                    
                    files[filePath] = currentFileTime;
                    
                    std::vector<DataPoint> data_list = readBits(filePath);
                    result_data_list.insert(result_data_list.end(), data_list.begin(), data_list.end());


                } else if (files[filePath] != currentFileTime) {
                    std::cout << std::to_string(newFileCounter + 1) << ": File modified: " << filePath << std::endl;
                    newFileCounter += 1;
                    
                    files[filePath] = currentFileTime;
                    
                    std::vector<DataPoint> data_list = readBits(filePath);
                    result_data_list.insert(result_data_list.end(), data_list.begin(), data_list.end());
                }
            }
        }

        return result_data_list;
    }

    std::vector<DataUtil::DataPoint> sortDataList(std::vector<DataUtil::DataPoint> dataList){
        std::vector<DataUtil::DataPoint> sortedDataList = dataList;
        std::sort(sortedDataList.begin(), sortedDataList.end());   
        return sortedDataList;
    }

    int sendFile(const std::string &filePath) {
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);

        if (!file) {
            std::cerr << "Could not open file: " << filePath << std::endl;
            return -1;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> fileData(size);
        if (!file.read(fileData.data(), size)) {
            std::cerr << "Error: Failed to read file" << std::endl;
            return -1;
        }


        int network_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (network_socket == -1) {
            std::cerr << "Error creating socket!" << std::endl;
            return -1;
        }

        struct sockaddr_in server_address;
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(8001);  // Port number
        if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
            std::cerr << "Invalid address!" << std::endl;
            close(network_socket);
            return -1;
        }

        int connection_status = connect(network_socket, (struct sockaddr *)&server_address, sizeof(server_address));
        if (connection_status == -1) {
            std::cerr << "Error connecting to server!" << std::endl;
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
        std::cout << "Server response: " << server_response << std::endl;

        // Close the socket
        close(network_socket);
        return 0;
    }

    int sendFromMemory(void* mem_addr, size_t size) {
        if (mem_addr == nullptr) {
            std::cerr << "Error: Null memory address provided." << std::endl;
            return -1;
        }

        int network_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (network_socket == -1) {
            std::cerr << "Error creating socket!" << std::endl;
            return -1;
        }

        struct sockaddr_in server_address;
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(8001);  
        if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
            std::cerr << "Invalid address!" << std::endl;
            close(network_socket);
            return -1;
        }

        int connection_status = connect(network_socket, (struct sockaddr*)&server_address, sizeof(server_address));
        if (connection_status == -1) {
            std::cerr << "Error connecting to server! errno: " << errno 
                    << " (" << strerror(errno) << ")" << std::endl;
            close(network_socket);
            return -1;
        }

        // Send file size
        uint32_t fileSizeN = htonl(static_cast<uint32_t>(size)); // Convert to network byte order
        ssize_t sentBytes = send(network_socket, &fileSizeN, sizeof(fileSizeN), 0);
        if (sentBytes < 0) {
            std::cerr << "Error sending file size!" << std::endl;
            close(network_socket);
            return -1;
        }

        // Send file data
        sentBytes = send(network_socket, mem_addr, size, 0); // Send actual file data
        if (sentBytes < 0) {
            std::cerr << "Error sending file data!" << std::endl;
            close(network_socket);
            return -1;
        }   

        std::cout << "Successfully sent " << sentBytes << " bytes from memory." << std::endl;
        close(network_socket);
        return 0;   
    }

    std::pair<char*, size_t> mapToMemory(std::string filePath) {
        int fd = open(filePath.c_str(), O_RDONLY);

        struct stat sb;
        if (fstat(fd, &sb) == -1) {
            std::cerr << "Error with file size " << std::endl;
            close(fd);
            return {nullptr, 0};
        }

        std::cout << "File size: " << sb.st_size << std::endl;

        char* file_in_memory = static_cast<char*>(mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
        close(fd);
        
        if (file_in_memory == MAP_FAILED) {
            std::cerr << "Error mapping file: " << filePath << std::endl;
            return {nullptr, 0};
        }

        return {file_in_memory, static_cast<size_t>(sb.st_size)};
    }


    void handleClient(int client_socket, int connection_num) {
        
        // Receive file size (known file)
        uint32_t fileSizeN;
        std::cout << "debug pt 4" << std::endl;

        ssize_t size_received = recv(client_socket, &fileSizeN, sizeof(fileSizeN), 0);
        if (size_received < 0) {
            std::cerr << "Error receiving file size! recv() failed with error: " << strerror(errno) << std::endl;
            close(client_socket);
            return;
        } else if (size_received != sizeof(fileSizeN)) {
            std::cerr << "Partial file size received. Expected " << sizeof(fileSizeN)
                    << " bytes, but received " << size_received << " bytes." << std::endl;
            close(client_socket);
            return;
        } else {
            std::cout << "Successfully received file size: " << ntohl(fileSizeN) << " bytes." << std::endl;
        }

        // Receive file data
    
        // Convert file size back to host byte order
        uint32_t fileSize = ntohl(fileSizeN);
        std::cout << "File size to receive: " << fileSize << " bytes." << std::endl;

        std::vector<char> buffer(fileSize);
        ssize_t total_received = 0;
        
        while (total_received < (ssize_t)fileSize) {
            ssize_t bytes = recv(client_socket, buffer.data() + total_received, fileSize - total_received, 0);
            if (bytes <= 0) {
                std::cerr << "Error receiving file data!" << std::endl;
                close(client_socket);
                return;
            }
            total_received += bytes;
        }
        std::cout << "debug pt 5" << std::endl;
        std::cout << "Successfully received " << total_received << " bytes." << std::endl;

        // Close client socket 
        close(client_socket);
        std::cout << "Client socket closed.\n"; 

        std::cout << "debug pt 6" << std::endl;
        // Write file data to disk
        std::cout << "Writing client data to disk.\n"; 
        std::string pathToDirectory = "~/projects/data_parsing/partitioned_data/";
        pathToDirectory = DirectoryUtil::expandTilde(pathToDirectory);
        std::cout << "Expanded path to directory: " << pathToDirectory << std::endl;

        if (!std::filesystem::exists(pathToDirectory)) {
            
            std::cout << "debug pt 7" << std::endl;
            
            if (!std::filesystem::create_directories(pathToDirectory)) {
                std::cerr << "Failed to create directory: " << pathToDirectory << std::endl;
                return;
            }
        }
        
        std::string receivedFileName = pathToDirectory + "received_file_" + std::to_string(connection_num) + ".dat";
        
        std::cout << "Attempting to open file: " << receivedFileName << std::endl;
        std::ofstream outFile(receivedFileName, std::ios::binary);

        std::cout << "debug pt 8" << std::endl;
        if (!outFile) {
            std::cerr << "Could not create output file: " << receivedFileName << std::endl;
            return;
        } else {
            std::cout << "Successfully opened file for writing: " << receivedFileName << std::endl;
        }

        std::cout << "debug pt 9" << std::endl;
        if (!buffer.empty()) {
            // Write the buffer data to the file
            std::cout << "Buffer size: " << buffer.size() << " bytes." << std::endl;
            outFile.write(buffer.data(), buffer.size());
            if (outFile.fail()) {
                std::cerr << "Error writing data to file: " << receivedFileName << std::endl;
            } else {
                std::cout << "Successfully wrote " << buffer.size() << " bytes to " << receivedFileName << std::endl;
            }
        } else {
            std::cerr << "Buffer is empty, nothing to write to file." << std::endl;
        }

    }
}