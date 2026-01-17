#include "../include/StompProtocol.h"
#include "../include/ConnectionHandler.h"
#include "../include/event.h"
#include <stdlib.h>
#include <thread>
#include <atomic>
#include <iostream>
#include <sstream>

std::atomic<bool> shouldTerminate(false);
std::atomic<bool> isLoggedIn(false);
std::string currentUser;

// Thread 1: Read from keyboard
void keyboardThread(StompProtocol& protocol, ConnectionHandler& connectionHandler) {
    while (!shouldTerminate) {
        std::string line;
        std::getline(std::cin, line);
        
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        
        if (command == "join") {
            if (!isLoggedIn) {
                std::cout << "Please login first" << std::endl;
                continue;
            }
            
            std::string gameName;
            iss >> gameName;
            
            std::string topic = "/" + gameName;
            std::string frame = protocol.buildSubscribeFrame(topic);
            if (!frame.empty() && connectionHandler.sendFrameAscii(frame, '\0')) {
                std::cout << "Joined channel " << gameName << std::endl;
            }
            
        } else if (command == "exit") {
            if (!isLoggedIn) {
                std::cout << "Please login first" << std::endl;
                continue;
            }
            
            std::string gameName;
            iss >> gameName;
            
            std::string topic = "/" + gameName;
            std::string frame = protocol.buildUnsubscribeFrame(topic);
            if (!frame.empty() && connectionHandler.sendFrameAscii(frame, '\0')) {
                std::cout << "Exited channel " << gameName << std::endl;
            } else if (frame.empty()) {
                std::cout << "Not subscribed to " << gameName << std::endl;
            }
            
        } else if (command == "report") {
            if (!isLoggedIn) {
                std::cout << "Please login first" << std::endl;
                continue;
            }
            
            std::string fileName;
            iss >> fileName;
            
            // TODO: Implement report functionality
            // Load events from JSON file
            // Send each event as SEND frame
            std::cout << "Report command - to be implemented" << std::endl;
            
        } else if (command == "summary") {
            if (!isLoggedIn) {
                std::cout << "Please login first" << std::endl;
                continue;
            }
            
            std::string gameName, user, outputFile;
            iss >> gameName >> user >> outputFile;
            
            // TODO: Implement summary functionality
            std::cout << "Summary command - to be implemented" << std::endl;
            
        } else if (command == "logout") {
            if (!isLoggedIn) {
                std::cout << "Not logged in" << std::endl;
                continue;
            }
            
            std::string frame = protocol.buildDisconnectFrame();
            connectionHandler.sendFrameAscii(frame, '\0');
            isLoggedIn = false;
            shouldTerminate = true;
            
        } else {
            std::cout << "Unknown command: " << command << std::endl;
        }
    }
}

// Thread 2: Read from socket
void socketThread(StompProtocol& protocol, ConnectionHandler& connectionHandler) {
    while (!shouldTerminate) {
        std::string frame;
        if (!connectionHandler.getFrameAscii(frame, '\0')) {
            std::cout << "Disconnected from server" << std::endl;
            shouldTerminate = true;
            break;
        }
        
        protocol.handleFrame(frame);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] <<" <host:port> or <host> <port>" << std::endl;
        return -1;
    }
    
    std::string host;
    short port;
    
    // Parse host:port format (e.g., "localhost:7777")
    std::string hostPort = argv[1];
    size_t colonPos = hostPort.find(':');
    
    if (colonPos != std::string::npos) {
        // Format: host:port
        host = hostPort.substr(0, colonPos);
        port = atoi(hostPort.substr(colonPos + 1).c_str());
    } else if (argc >= 3) {
        // Format: host port
        host = argv[1];
        port = atoi(argv[2]);
    } else {
        std::cerr << "Invalid arguments - must provide port" << std::endl;
        return -1;
    }
    
    ConnectionHandler connectionHandler(host, port);
    
    // Wait for login command
    std::cout << "Waiting for login command..." << std::endl;
    std::string loginCommand;
    std::getline(std::cin, loginCommand);
    
    std::istringstream iss(loginCommand);
    std::string cmd, hostPortArg, username, password;
    iss >> cmd;
    
    if (cmd != "login") {
        std::cerr << "First command must be login" << std::endl;
        return -1;
    }
    
    iss >> hostPortArg >> username >> password;
    currentUser = username;
    
    // Connect to server
    if (!connectionHandler.connect()) {
        std::cout << "Could not connect to server" << std::endl;
        return 1;
    }
    
    // Create protocol handler
    StompProtocol protocol;
    
    // Send CONNECT frame
    std::string connectFrame = protocol.buildConnectFrame(host, username, password);
    if (!connectionHandler.sendFrameAscii(connectFrame, '\0')) {
        std::cerr << "Failed to send CONNECT frame" << std::endl;
        return 1;
    }
    
    // Wait briefly for CONNECTED response
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    isLoggedIn = true;
    
    // Start threads
    std::thread keyThread(keyboardThread, std::ref(protocol), std::ref(connectionHandler));
    std::thread sockThread(socketThread, std::ref(protocol), std::ref(connectionHandler));
    
    // Wait for threads to finish
    keyThread.join();
    sockThread.join();
    
    connectionHandler.close();
    std::cout << "Client terminated" << std::endl;
    
    return 0;
}