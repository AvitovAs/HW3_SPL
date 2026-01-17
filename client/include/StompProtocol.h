#pragma once

#include <string>
#include <map>

class StompProtocol
{
private:
    int subsIdCounter;
    int receiptIdCounter;
    std::map<std::string, int> topicToSubId;
    
    // Parsing helpers
    std::string extractCommand(const std::string& frame);
    std::map<std::string, std::string> parseHeaders(const std::string& frame);
    std::string extractBody(const std::string& frame);

public:
    StompProtocol();
    ~StompProtocol();

    // Build frame operations (return frame strings)
    std::string buildConnectFrame(const std::string& host, const std::string& username, const std::string& passcode);
    std::string buildSubscribeFrame(const std::string& topic);
    std::string buildUnsubscribeFrame(const std::string& topic);
    std::string buildSendFrame(const std::string& topic, const std::string& body);
    std::string buildDisconnectFrame();
    
    // Handle incoming frames
    void handleFrame(const std::string& frame);
    
    // Check if subscribed to topic
    bool isSubscribed(const std::string& topic);
    
private:
    int getNextSubId();
    int getNextReceiptId();
};