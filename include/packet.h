#pragma once
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <vector>
#include <sstream>
#include <fcntl.h>
#include "data_util.h"

namespace PacketUtil {
    struct Packet {
        uint32_t packetID;
        std::vector<DataUtil::DataPoint> data_list;

        std::vector<DataUtil::DataPoint> getDataList() {
            return data_list;
        }

        void serialize(std::ostream& os) {
            os.write(reinterpret_cast<const char*>(&packetID), sizeof(packetID));
            uint32_t data_list_count = static_cast<uint32_t>(data_list.size());
            os.write(reinterpret_cast<const char*>(&data_list_count), sizeof(data_list_count));

            for (const auto& data_point : data_list) {
                data_point.serialize(os);
            }
        }

        void deserialize(std::istream& is) {
            is.read(reinterpret_cast<char*>(&packetID), sizeof(packetID));
            uint32_t data_list_count;
            is.read(reinterpret_cast<char*>(&data_list_count), sizeof(data_list_count));

            data_list.resize(data_list_count);
            for (auto& data_point : data_list) {
                data_point.deserialize(is);
            }
        }

        void print() const {
            std::cout << "Packet ID: " << packetID << "\n";
            std::cout << "Data List Count: " << data_list.size() << "\n";
            for (const auto& data_point : data_list) {
                data_point.print();
            }
        }
    };

    void sendPacket(int network_socket, Packet& packet) {
        // Serialize the packet into binary format using a stringstream
        std::stringstream ss;
        packet.serialize(ss);
        
        std::string packet_data = ss.str();

        // Set up the file descriptor sets for select
        fd_set write_fds;
        FD_ZERO(&write_fds);
        FD_SET(network_socket, &write_fds);

        // Set a timeout to prevent blocking indefinitely (e.g., 5 seconds)
        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        // Check if the socket is ready for writing using select
        int ready = select(network_socket + 1, NULL, &write_fds, NULL, &timeout);

        if (ready < 0) {
            std::cerr << "Error in select!" << std::endl;
            close(network_socket);
            return;
        } else if (ready == 0) {
            std::cerr << "Timeout: Socket is not ready for writing!" << std::endl;
            return;
        } else {
            if (FD_ISSET(network_socket, &write_fds)) {

                // Send the packet size
                uint32_t packet_size = htonl(packet_data.size()); // Convert to network byte order
                if (send(network_socket, &packet_size, sizeof(packet_size), 0) == -1) {
                    std::cerr << "Error sending packet size!" << std::endl;
                    return;
                }

                // Send the serialized packet data over the network socket
                ssize_t bytes_sent = send(network_socket, packet_data.c_str(), packet_data.size(), 0);
                if (bytes_sent == -1) {
                    std::cerr << "Error sending packet data!" << std::endl;
                    close(network_socket);
                } else {
                    std::cout << "Sent " << bytes_sent << " bytes to the server." << std::endl;
                }
            }
        }
    }

    uint32_t ipToUint32(const std::string& ipAddress) {
        uint32_t ip;
        if (inet_pton(AF_INET, ipAddress.c_str(), &ip) != 1) {
            std::cerr << "Invalid IP address format." << std::endl;
            return 0;
        }
        return ip;
    }


    uint32_t getClientIP(int clientSocket) {
        struct sockaddr_in clientAddr;
        std::string ip_str;
        socklen_t addrLen = sizeof(clientAddr);
        
        // Get the client address
        if (getpeername(clientSocket, (struct sockaddr*)&clientAddr, &addrLen) == 0) {
            // Convert IP address to string format
            ip_str = inet_ntoa(clientAddr.sin_addr);
        }
        else{
            std::cerr << "error converting IP to string format" << std::endl;
            return 0;}

        return ipToUint32(ip_str);
    }



    PacketUtil::Packet receivePacket(int client_socket) {
        //1.16.25: no longer receiving packet, receiving raw binary instead

        std::stringstream ss;  // Stringstream to accumulate data
        ss.clear();  // Clear any previous contents

        ssize_t bytes_received;
        uint32_t packet_size;
        ssize_t size_received = recv(client_socket, &packet_size, sizeof(packet_size), 0);  // First, receive the size of the incoming packet
        PacketUtil::Packet emptyPacket;
        
        if (size_received <= 0 ){
            std::cerr << "error receiving packet size" << std::endl;
            return emptyPacket;
        }

        packet_size = ntohl(packet_size);

        std::vector<char> buffer(packet_size);
        ssize_t total_received = 0;

        std::cout << "packet size: " << packet_size << std::endl;
        while (total_received < packet_size) {
            bytes_received = recv(client_socket, buffer.data() + total_received, packet_size - total_received, 0);
            std::cout << "bytes_received: " << bytes_received << std::endl;
            
            if (bytes_received <= 0) {
                std::cerr << "Error receiving data!" << std::endl;
                    
                return emptyPacket;
            }
            total_received += bytes_received;
        }

        // Use stringstream to deserialize the packet
        ss.write(buffer.data(), buffer.size());
        PacketUtil::Packet packet;
        ss.seekg(0);
        packet.deserialize(ss);

        return packet;
    }

    

}