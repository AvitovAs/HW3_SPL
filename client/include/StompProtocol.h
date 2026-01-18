#pragma once

#include "../include/ConnectionHandler.h"
#include "../include/event.h"
#include <string>
#include <vector>
#include <map>
#include <mutex>

// מחלקה זו מנהלת את כל הלוגיקה של הפרוטוקול
class StompProtocol {
private:
    std::string username;
    int subscriptionIdCounter;
    int receiptIdCounter;
    bool isConnectedFlag;
    bool shouldTerminateFlag;

    // מיפויים לשמירת המצב
    std::map<std::string, int> gameToSubId; // Game Name -> Subscription ID
    std::map<int, std::string> subIdToGame; // Subscription ID -> Game Name
    
    // ניהול Receipts: ReceiptID -> Command/Action Description
    std::map<int, std::string> receiptActions;

    // מבנה נתונים לסיכום משחק: Game Name -> (User Name -> List of Events)
    std::map<std::string, std::map<std::string, std::vector<Event>>> gameStorage;

    std::mutex mtx; // לסנכרון בין הת'רד הראשי לת'רד של הסוקט (אם נדרש)

public:
    StompProtocol();

    // ניהול חיבור
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