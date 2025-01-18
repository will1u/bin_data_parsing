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

int main() {

    int numFiles = 3;

    vector<thread> threads; // Vector to hold threads

    for (int i = 1; i <= numFiles; i++) {

        string directoryPath = "~/projects/data_parsing/tests/";
        directoryPath = DirectoryUtil::expandTilde(directoryPath);
        string filePath = directoryPath + "file-" + std::to_string(i) + ".dat";


        threads.emplace_back(DataUtil::sendFile, filePath);
    }

    for (auto &t : threads) {
        t.join();
    }

    cout << "All files have been sent." << endl;

    return 0;
}