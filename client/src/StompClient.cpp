#include <iostream>
#include <thread>
#include <vector>
#include <sstream>
#include "../include/ConnectionHandler.h"
#include "../include/StompProtocol.h"
#include "../include/event.h"

using namespace std;

// פונקציית הת'רד שמאזינה לסוקט
void socketReader(ConnectionHandler* handler, StompProtocol* protocol) {
    while (!protocol->shouldTerminate()) {
        std::string frame;
        // קריאה עד תו ה-Null לפי פרוטוקול STOMP
        if (!handler->getFrameAscii(frame, '\0')) {
            std::cout << "Disconnected from server." << std::endl;
            protocol->setShouldTerminate(true);
            protocol->setConnected(false);
            break;
        }
        // עיבוד ההודעה
        protocol->processFrame(frame, *handler);
    }
}

int main(int argc, char *argv[]) {
    StompProtocol protocol;
    ConnectionHandler* connectionHandler = nullptr;
    std::thread* listenerThread = nullptr;

    while (1) {
        const short bufsize = 1024;
        char buf[bufsize];
        std::cin.getline(buf, bufsize);
        std::string line(buf);
        
        std::stringstream ss(line);
        std::string command;
        ss >> command;

        if (command == "login") {
            if (protocol.isConnected()) {
                std::cout << "The client is already logged in, log out before trying again" << std::endl;
                continue;
            }

            std::string hostPort, username, password;
            ss >> hostPort >> username >> password;

            size_t colonPos = hostPort.find(':');
            if (colonPos == std::string::npos) {
                std::cout << "Invalid host:port format" << std::endl;
                continue;
            }
            std::string host = hostPort.substr(0, colonPos);
            short port = (short)stoi(hostPort.substr(colonPos + 1));

            connectionHandler = new ConnectionHandler(host, port);
            if (!connectionHandler->connect()) {
                std::cout << "Could not connect to server" << std::endl;
                delete connectionHandler;
                connectionHandler = nullptr;
                continue;
            }

            protocol.setUsername(username);
            
            // שליחת מסגרת CONNECT
            std::string frame = protocol.buildConnectFrame(host, username, password);
            if (!connectionHandler->sendFrameAscii(frame, '\0')) {
                std::cout << "Failed to send CONNECT frame" << std::endl;
                connectionHandler->close();
                delete connectionHandler;
                connectionHandler = nullptr;
                continue;
            }

            // הפעלת ת'רד ההאזנה
            listenerThread = new std::thread(socketReader, connectionHandler, &protocol);
        } 
        else if (command == "join") {
            if (!protocol.isConnected()) {
                std::cout << "Not connected. Please login first." << std::endl;
                continue;
            }
            std::string gameName;
            ss >> gameName;
            std::string frame = protocol.buildSubscribeFrame(gameName);
            connectionHandler->sendFrameAscii(frame, '\0');
            // ההדפסה "Joined channel" תתבצע בת'רד השני כשיגיע ה-RECEIPT
        }
        else if (command == "exit") {
             if (!protocol.isConnected()) {
                std::cout << "Not connected." << std::endl;
                continue;
            }
            std::string gameName;
            ss >> gameName;
            std::string frame = protocol.buildUnsubscribeFrame(gameName);
            if (frame != "")
                connectionHandler->sendFrameAscii(frame, '\0');
            else 
                std::cout << "Not subscribed to channel " << gameName << std::endl;
        }
        else if (command == "report") {
             if (!protocol.isConnected()) {
                std::cout << "Not connected." << std::endl;
                continue;
            }
            std::string file;
            ss >> file;
            // שימוש בפונקציית העזר מ-event.cpp
            names_and_events data = parseEventsFile(file);
            std::string gameName = data.team_a_name + "_" + data.team_b_name;

            for (const Event& event : data.events) {
                std::string frame = protocol.buildSendFrame(gameName, event);
                connectionHandler->sendFrameAscii(frame, '\0');
                
                // שמירה מקומית של האירועים שאני דיווחתי
                protocol.saveEvent(event, gameName, protocol.getUsername());
            }
            // סנכרון פשוט להדפסה
            std::cout << "Reported " << data.events.size() << " events for game " << gameName << std::endl;
        }
        else if (command == "summary") {
            std::string gameName, user, file;
            ss >> gameName >> user >> file;
            std::string result = protocol.getSummary(gameName, user, file);
            std::cout << result << std::endl;
        }
        else if (command == "logout") {
             if (!protocol.isConnected()) {
                std::cout << "Not connected." << std::endl;
                continue;
            }
            std::string frame = protocol.buildDisconnectFrame();
            connectionHandler->sendFrameAscii(frame, '\0');
            
            // המתנה לסיום הת'רד (שמזהה את ה-RECEIPT של הניתוק)
            if (listenerThread && listenerThread->joinable()) {
                listenerThread->join();
                delete listenerThread;
                listenerThread = nullptr;
            }
            
            connectionHandler->close();
            delete connectionHandler;
            connectionHandler = nullptr;
            protocol.setConnected(false); // ליתר ביטחון
            
            // במטלה כתוב שאחרי logout ה-Client מחכה לפקודות נוספות (כמו login חדש)
            // אז לא עושים break, אלא ממשיכים בלולאה
            std::cout << "Logged out." << std::endl;
        }
    }
    return 0;
}