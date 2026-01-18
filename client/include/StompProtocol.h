#pragma once

#include "../include/ConnectionHandler.h"
#include "../include/event.h"
#include <string>
#include <vector>
#include <map>
#include <mutex>

class StompProtocol {
private:
    std::string username;
    int subscriptionIdCounter;
    int receiptIdCounter;
    bool isConnectedFlag;
    bool shouldTerminateFlag;

    std::map<std::string, int> gameToSubId;
    std::map<int, std::string> subIdToGame;
    
    std::map<int, std::string> receiptActions;

    std::map<std::string, std::map<std::string, std::vector<Event>>> gameStorage;

    std::mutex mtx; 

public:
    StompProtocol();

    void setUsername(std::string name);
    std::string getUsername();
    bool isConnected();
    void setConnected(bool status);
    bool shouldTerminate();
    void setShouldTerminate(bool status);

    void processFrame(std::string frame, ConnectionHandler& connectionHandler);

    std::string buildConnectFrame(std::string host, std::string login, std::string passcode);
    std::string buildSubscribeFrame(std::string destination);
    std::string buildUnsubscribeFrame(std::string destination);
    std::string buildSendFrame(std::string destination, const Event& event, std::string filename);
    std::string buildDisconnectFrame();

    void saveEvent(const Event& event, std::string gameName, std::string user);
    
    std::string getSummary(std::string gameName, std::string user, std::string file);

private:
    Event parseEventBody(std::string body);
    void handleMessage(std::string frame);
    void handleReceipt(std::string frame);
    void handleError(std::string frame, ConnectionHandler& handler);
};