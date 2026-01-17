#include "../include/StompProtocol.h"
#include <iostream>
#include <sstream>

StompProtocol::StompProtocol() 
    : subsIdCounter(0), receiptIdCounter(0) {}

StompProtocol::~StompProtocol() {}

int StompProtocol::getNextSubId() {
    return subsIdCounter++;
}

int StompProtocol::getNextReceiptId() {
    return receiptIdCounter++;
}

std::string StompProtocol::buildConnectFrame(const std::string& host, const std::string& username, const std::string& passcode) {
    std::string frame = "CONNECT\n";
    frame += "accept-version:1.2\n";
    frame += "host:" + host + "\n";
    frame += "login:" + username + "\n";
    frame += "passcode:" + passcode + "\n";
    frame += "\n";
    
    return frame;
}

std::string StompProtocol::buildSubscribeFrame(const std::string& topic) {
    int subId = getNextSubId();
    topicToSubId[topic] = subId;
    
    std::string frame = "SUBSCRIBE\n";
    frame += "destination:" + topic + "\n";
    frame += "id:" + std::to_string(subId) + "\n";
    frame += "receipt:" + std::to_string(getNextReceiptId()) + "\n";
    frame += "\n";
    
    return frame;
}

std::string StompProtocol::buildUnsubscribeFrame(const std::string& topic) {
    if (topicToSubId.find(topic) == topicToSubId.end()) {
        return ""; // Not subscribed
    }
    
    int subId = topicToSubId[topic];
    topicToSubId.erase(topic);
    
    std::string frame = "UNSUBSCRIBE\n";
    frame += "id:" + std::to_string(subId) + "\n";
    frame += "receipt:" + std::to_string(getNextReceiptId()) + "\n";
    frame += "\n";
    
    return frame;
}

std::string StompProtocol::buildSendFrame(const std::string& topic, const std::string& body) {
    std::string frame = "SEND\n";
    frame += "destination:" + topic + "\n";
    frame += "\n";
    frame += body;
    
    return frame;
}

std::string StompProtocol::buildDisconnectFrame() {
    std::string frame = "DISCONNECT\n";
    frame += "receipt:" + std::to_string(getNextReceiptId()) + "\n";
    frame += "\n";
    
    return frame;
}

bool StompProtocol::isSubscribed(const std::string& topic) {
    return topicToSubId.find(topic) != topicToSubId.end();
}

std::string StompProtocol::extractCommand(const std::string& frame) {
    size_t pos = frame.find('\n');
    if (pos == std::string::npos) return "";
    return frame.substr(0, pos);
}

std::map<std::string, std::string> StompProtocol::parseHeaders(const std::string& frame) {
    std::map<std::string, std::string> headers;
    std::istringstream stream(frame);
    std::string line;
    
    // Skip command line
    std::getline(stream, line);
    
    // Parse headers
    while (std::getline(stream, line) && !line.empty()) {
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            headers[key] = value;
        }
    }
    
    return headers;
}

std::string StompProtocol::extractBody(const std::string& frame) {
    size_t bodyStart = frame.find("\n\n");
    if (bodyStart == std::string::npos) return "";
    return frame.substr(bodyStart + 2);
}

void StompProtocol::handleFrame(const std::string& frame) {
    std::string command = extractCommand(frame);
    std::map<std::string, std::string> headers = parseHeaders(frame);
    std::string body = extractBody(frame);
    
    if (command == "CONNECTED") {
        std::cout << "Login successful" << std::endl;
    } 
    else if (command == "RECEIPT") {
        // Receipt acknowledgment - can add more specific handling if needed
    }
    else if (command == "MESSAGE") {
        // Handle incoming game event messages
        std::cout << "Received message from " << headers["destination"] << std::endl;
        // TODO: Parse and save game events
    }
    else if (command == "ERROR") {
        std::string message = headers.count("message") ? headers["message"] : body;
        std::cerr << "Error: " << message << std::endl;
    }
}

