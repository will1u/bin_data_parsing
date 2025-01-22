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
#include <mutex>

#include "data_util.h"
#include "packet.h"
#include "directory_util.h"

int main() {
    int numFiles = 256;
    std::atomic<bool> startSending(false); 
    std::atomic<bool> stopSending(false);  
    std::mutex logMutex;

    std::vector<std::thread> threads;

    std::string directoryPath = "~/projects/data_parsing/tests/";
    directoryPath = DirectoryUtil::expandTilde(directoryPath);

    std::vector<std::pair<char*, size_t>> memoryRegions;

    for (int i = 0; i < numFiles; i++) {
        std::string filePath = directoryPath + "file-" + std::to_string(i + 1) + ".dat";

        // Map file to memory
        auto memoryRegion = DataUtil::mapToMemory(filePath);
        char* memoryAddress = memoryRegion.first;
        size_t fileSize = memoryRegion.second;

        // Check if mapping was successful
        if (memoryAddress) {
            memoryRegions.emplace_back(memoryAddress, fileSize);

            // Debugging output
            std::cout << "File: " << filePath
                    << ", Mapped to address: " << static_cast<void*>(memoryAddress)
                    << ", Size: " << fileSize << " bytes." << std::endl;
        } else {
            std::cerr << "Failed to map file: " << filePath << std::endl;
        }
    }

    for (const auto& region : memoryRegions) {
        char* memAddr = region.first;
        size_t size = region.second;

        threads.emplace_back([&, memAddr, size]() {
            while (!startSending.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (stopSending.load()) return;
            }
            {
                std::lock_guard<std::mutex> lock(logMutex);
                std::cout << "Thread ID: " << std::this_thread::get_id()
                        << ", Memory address: " << static_cast<void*>(memAddr)
                        << std::endl;
            }
            DataUtil::sendFromMemory(memAddr, size);
        });
    }

    // dummy start and stop control for now
    std::cout << "Press Enter to start sending files..." << std::endl;
    std::cin.get(); 
    startSending.store(true); 

    std::cout << "Press Enter to stop sending files..." << std::endl;
    std::cin.get(); 
    stopSending.store(true); 


    for (auto &t : threads) {
        t.join();
    }

    std::cout << "All files have been sent or stopped." << std::endl;

    return 0;
}