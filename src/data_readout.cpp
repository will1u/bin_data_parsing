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

int main() {
    int numFiles = 256;
    std::atomic<bool> startSending(false); 
    std::atomic<bool> stopSending(false);  

    std::vector<std::thread> threads; 

    for (int i = 1; i <= numFiles; i++) {
        std::string directoryPath = "~/projects/data_parsing/tests/";
        directoryPath = DirectoryUtil::expandTilde(directoryPath);
        std::string filePath = directoryPath + "file-" + std::to_string(i) + ".dat";

        // Launch a thread for each file
        threads.emplace_back([&, filePath]() {
            while (!startSending.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
                if (stopSending.load()) return; 
            }
            DataUtil::sendFile(filePath);
            if (stopSending.load()) return; 
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