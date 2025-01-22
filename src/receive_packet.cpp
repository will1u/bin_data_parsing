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
#include <poll.h>
#include <atomic>
#include "data_util.h"
#include "packet.h"
#include "timing_util.h"
#include "directory_util.h"

using namespace std;

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

    if (listen(server_socket, 1024) < 0) {
        cerr << "Error listening on socket!" << endl;
        close(server_socket);
        return -1;
    }

    std::cout << "Server is listening on port 8001..." << std::endl;

    // Accept connections in a loop
    int connections = 0;
    std::atomic<int> activeThreads(0);

    atomic<bool> closeServer = false;
    thread serverCloser([&closeServer]() {
        cout << "Press Enter to stop the server..." << endl;
        cin.get();
        closeServer.store(true);
    });

    while (!closeServer.load()) {
        

        struct pollfd pfd;
        pfd.fd = server_socket;
        pfd.events = POLLIN; // Wait for incoming connections
        int poll_result = poll(&pfd, 1, 100); // Timeout after 100 milliseconds

        if (poll_result == -1) {
            std::cerr << "Error with poll()." << std::endl;
            break;
        } else if (poll_result == 0) {
            // Timeout, check `closeServer`
            continue;
        }
        
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

        activeThreads += 1;

        // Spawn a new thread for this client
        cout << "debug pt 3" << endl;
        std::thread t([client_socket, connections, &activeThreads]() {
            DataUtil::handleClient(client_socket, connections + 1);

            // Decrement the active thread count when done
            activeThreads--;
        });

        t.detach();
        connections += 1;

        cout << "----------------------------------" << endl;
        cout << connections << " connections accepted." << endl;
        cout << "----------------------------------" << endl;
        
    }
    
    while (activeThreads > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    close(server_socket);
    serverCloser.join();
    return 0;
}
