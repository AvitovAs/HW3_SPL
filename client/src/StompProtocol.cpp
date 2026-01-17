#include "../include/StompProtocol.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <fstream>

StompProtocol::StompProtocol() 
    : username(""), subscriptionIdCounter(0), receiptIdCounter(0), 
      isConnectedFlag(false), shouldTerminateFlag(false), 
      gameToSubId(), subIdToGame(), receiptActions(), gameStorage(), mtx() {}

void StompProtocol::setUsername(std::string name) {
    this->username = name;
}

std::string StompProtocol::getUsername() {
    return username;
}

bool StompProtocol::isConnected() {
    return isConnectedFlag;
}

void StompProtocol::setConnected(bool status) {
    isConnectedFlag = status;
}

bool StompProtocol::shouldTerminate() {
    return shouldTerminateFlag;
}

void StompProtocol::setShouldTerminate(bool status) {
    shouldTerminateFlag = status;
}

// === Frame Generation Methods ===

std::string StompProtocol::buildConnectFrame(std::string host, std::string login, std::string passcode) {
    std::stringstream ss;
    ss << "CONNECT\n"
       << "accept-version:1.2\n"
       << "host:" << host << "\n"
       << "login:" << login << "\n"
       << "passcode:" << passcode << "\n"
       << "\n" << '\0';
    return ss.str();
}

std::string StompProtocol::buildSubscribeFrame(std::string destination) {
    subscriptionIdCounter++;
    int id = subscriptionIdCounter;
    receiptIdCounter++;
    int receipt = receiptIdCounter;

    // רישום מקומי
    gameToSubId[destination] = id;
    subIdToGame[id] = destination;
    receiptActions[receipt] = "joined " + destination;

    std::stringstream ss;
    ss << "SUBSCRIBE\n"
       << "destination:/" << destination << "\n"
       << "id:" << id << "\n"
       << "receipt:" << receipt << "\n"
       << "\n" << '\0';
    return ss.str();
}

std::string StompProtocol::buildUnsubscribeFrame(std::string destination) {
    if (gameToSubId.find(destination) == gameToSubId.end()) {
        return ""; // לא רשום
    }
    int id = gameToSubId[destination];
    receiptIdCounter++;
    int receipt = receiptIdCounter;

    receiptActions[receipt] = "exited " + destination;

    std::stringstream ss;
    ss << "UNSUBSCRIBE\n"
       << "id:" << id << "\n"
       << "receipt:" << receipt << "\n"
       << "\n" << '\0';
    
    // נמחק מהמיפוי רק אחרי קבלת RECEIPT, אבל למען הפשטות נניח שהצליח
    // בפועל עדיף לחכות ל-Receipt, אבל המימוש פה אופטימי
    // כאן לא מוחקים עדיין כדי שנדע למי שייך ה-Receipt
    return ss.str();
}

std::string StompProtocol::buildSendFrame(std::string destination, const Event& event) {
    // בניית גוף ההודעה לפי הפורמט הנדרש
    std::stringstream body;
    body << "user: " << username << "\n";
    body << "team a: " << event.get_team_a_name() << "\n";
    body << "team b: " << event.get_team_b_name() << "\n";
    body << "event name: " << event.get_name() << "\n";
    body << "time: " << event.get_time() << "\n";
    body << "general game updates:\n";
    for (auto const& [key, val] : event.get_game_updates()) {
        body << "    " << key << ": " << val << "\n";
    }
    body << "team a updates:\n";
    for (auto const& [key, val] : event.get_team_a_updates()) {
        body << "    " << key << ": " << val << "\n";
    }
    body << "team b updates:\n";
    for (auto const& [key, val] : event.get_team_b_updates()) {
        body << "    " << key << ": " << val << "\n";
    }
    body << "description:\n" << event.get_discription();

    std::stringstream ss;
    ss << "SEND\n"
       << "destination:/" << destination << "\n"
       << "\n" 
       << body.str() << "\n" << '\0';
    return ss.str();
}

std::string StompProtocol::buildDisconnectFrame() {
    receiptIdCounter++;
    int receipt = receiptIdCounter;
    receiptActions[receipt] = "disconnect";

    std::stringstream ss;
    ss << "DISCONNECT\n"
       << "receipt:" << receipt << "\n"
       << "\n" << '\0';
    return ss.str();
}

// === Processing Incoming Frames ===

void StompProtocol::processFrame(std::string frame, ConnectionHandler& connectionHandler) {
    std::stringstream ss(frame);
    std::string command;
    std::getline(ss, command);

    // ניקוי תווים לא רצויים מהפקודה (כמו \r)
    if (!command.empty() && command.back() == '\r') command.pop_back();

    if (command == "CONNECTED") {
        std::cout << "Login successful" << std::endl;
        isConnectedFlag = true;
    } 
    else if (command == "MESSAGE") {
        handleMessage(frame);
    } 
    else if (command == "RECEIPT") {
        handleReceipt(frame);
    } 
    else if (command == "ERROR") {
        handleError(frame, connectionHandler);
    }
}

void StompProtocol::handleMessage(std::string frame) {
    // דילוג על ה-Headers והגעה לגוף ההודעה
    size_t bodyPos = frame.find("\n\n");
    if (bodyPos == std::string::npos) return;
    std::string body = frame.substr(bodyPos + 2);

    // חילוץ שם המשחק (destination) מה-Headers
    std::string destination = "";
    std::stringstream ss(frame);
    std::string line;
    while (std::getline(ss, line) && line != "") {
        if (line.find("destination:") == 0) {
            destination = line.substr(12); // אורך "destination:" הוא 12
            // אם יש / בהתחלה, נוריד אותו
            if (destination.length() > 0 && destination[0] == '/') destination = destination.substr(1);
            if (!destination.empty() && destination.back() == '\r') destination.pop_back();
        }
    }

    Event event = parseEventBody(body);
    
    // חילוץ המשתמש ששלח את ההודעה מתוך הגוף שפירסרנו (נמצא בשורה הראשונה לרוב)
    // אבל אנחנו נשתמש במידע שכבר יש לנו מהפירסור הידני למטה
    
    // כדי למנוע כפילויות: אם אנחנו שלחנו את ההודעה, לא נשמור אותה שוב (כי שמרנו ב-report)
    // ה-Event לא שומר את שם המשתמש כשדה, אז נחלץ אותו ידנית מהסטרינג
    std::string userInMessage = "";
    std::stringstream bodyStream(body);
    if (std::getline(bodyStream, line) && line.find("user:") != std::string::npos) {
         userInMessage = line.substr(line.find(":") + 2); // הנחה שיש רווח אחרי הנקודותיים
         if (!userInMessage.empty() && userInMessage.back() == '\r') userInMessage.pop_back();
    }

    if (userInMessage != username) {
        // שמירת האירוע רק אם זה לא אנחנו
        saveEvent(event, destination, userInMessage);
        std::cout << "Received message from " << userInMessage << " in topic " << destination << std::endl;
    }
}

void StompProtocol::handleReceipt(std::string frame) {
    std::stringstream ss(frame);
    std::string line;
    int receiptId = -1;
    while(std::getline(ss, line) && line != "") {
        if (line.find("receipt-id:") == 0) {
            receiptId = std::stoi(line.substr(11));
        }
    }

    if (receiptId != -1 && receiptActions.count(receiptId)) {
        std::string action = receiptActions[receiptId];
        if (action.find("joined") == 0) {
            std::cout << "Joined channel " << action.substr(7) << std::endl;
        } else if (action.find("exited") == 0) {
            std::string game = action.substr(7);
            std::cout << "Exited channel " << game << std::endl;
            // ניקוי המיפויים
            if (gameToSubId.count(game)) {
                int id = gameToSubId[game];
                subIdToGame.erase(id);
                gameToSubId.erase(game);
            }
        } else if (action == "disconnect") {
            setShouldTerminate(true);
            setConnected(false);
            // כאן אפשר לסגור את הסוקט ב-Client הראשי
        }
        receiptActions.erase(receiptId);
    }
}

void StompProtocol::handleError(std::string frame, ConnectionHandler& handler) {
    std::cout << frame << std::endl; // הדפסת השגיאה במלואה או חלקים ממנה
    setShouldTerminate(true);
    setConnected(false);
    handler.close();
}

void StompProtocol::saveEvent(const Event& event, std::string gameName, std::string user) {
    std::lock_guard<std::mutex> lock(mtx);
    gameStorage[gameName][user].push_back(event);
}

// פונקציית עזר לפירסור גוף ההודעה והמרתו לאובייקט Event
Event StompProtocol::parseEventBody(std::string body) {
    std::stringstream ss(body);
    std::string line;
    std::string team_a = "", team_b = "", event_name = "", description = "";
    int time = 0;
    std::map<std::string, std::string> general_updates, team_a_updates, team_b_updates;
    
    std::string current_section = "";

    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        
        // זיהוי שדות ראשיים
        if (line.find("team a:") == 0) team_a = line.substr(8);
        else if (line.find("team b:") == 0) team_b = line.substr(8);
        else if (line.find("event name:") == 0) event_name = line.substr(12);
        else if (line.find("time:") == 0) time = std::stoi(line.substr(6));
        else if (line == "general game updates:") current_section = "general";
        else if (line == "team a updates:") current_section = "team_a";
        else if (line == "team b updates:") current_section = "team_b";
        else if (line == "description:") current_section = "description";
        else if (current_section == "description") {
            description += line + "\n";
        }
        else if (line.find(":") != std::string::npos && current_section != "") {
            // טיפול בעדכונים (מפתח: ערך)
            size_t delim = line.find(":");
            std::string key = line.substr(0, delim);
            // הסרת רווחים בהתחלה (אינדנטציה)
            key.erase(0, key.find_first_not_of(" \t"));
            std::string val = line.substr(delim + 1);
            if (!val.empty() && val[0] == ' ') val = val.substr(1); // הסרת רווח אחרי נקודותיים

            if (current_section == "general") general_updates[key] = val;
            else if (current_section == "team_a") team_a_updates[key] = val;
            else if (current_section == "team_b") team_b_updates[key] = val;
        }
    }

    return Event(team_a, team_b, event_name, time, general_updates, team_a_updates, team_b_updates, description);
}

std::string StompProtocol::getSummary(std::string gameName, std::string user, std::string file) {
    // לוגיקת הסיכום - איסוף הנתונים וכתיבה לקובץ מתבצעת ב-Client עצמו או כאן
    // נבצע את איסוף הנתונים כאן ונחזיר סטרינג (או שנכתוב לקובץ ישר)
    // המטלה מבקשת לכתוב לקובץ. נעשה זאת כאן.
    
    std::string output = "";
    if (gameStorage[gameName].find(user) == gameStorage[gameName].end()) {
        return "No events found for user " + user + " in game " + gameName;
    }

    std::vector<Event>& events = gameStorage[gameName][user];
    if (events.empty()) return "No events to summarize";

    // חישוב סטטיסטיקות מצטברות
    std::map<std::string, std::string> general_stats;
    std::map<std::string, std::string> stats_a;
    std::map<std::string, std::string> stats_b;

    // שם הקבוצות נלקח מהאירוע הראשון
    std::string teamA = events[0].get_team_a_name();
    std::string teamB = events[0].get_team_b_name();

    for (const auto& ev : events) {
        for (auto const& [k, v] : ev.get_game_updates()) general_stats[k] = v;
        for (auto const& [k, v] : ev.get_team_a_updates()) stats_a[k] = v;
        for (auto const& [k, v] : ev.get_team_b_updates()) stats_b[k] = v;
    }

    // פורמט הפלט לפי המטלה
    std::stringstream report;
    report << teamA << " vs " << teamB << "\n";
    report << "Game stats:\n";
    report << "General stats:\n";
    for (auto const& [k, v] : general_stats) report << k << ": " << v << "\n";
    
    report << teamA << " stats:\n";
    for (auto const& [k, v] : stats_a) report << k << ": " << v << "\n";
    
    report << teamB << " stats:\n";
    for (auto const& [k, v] : stats_b) report << k << ": " << v << "\n";

    report << "Game event reports:\n";
    for (const auto& ev : events) {
        report << ev.get_time() << " - " << ev.get_name() << ":\n\n";
        report << ev.get_discription() << "\n\n";
    }

    // כתיבה לקובץ
    std::ofstream outfile(file);
    if (outfile.is_open()) {
        outfile << report.str();
        outfile.close();
        return "Summary wrote to " + file;
    } else {
        return "Error opening file " + file;
    }
}