#include <stdlib.h>
#include "../include/ConnectionHandler.h"
#include <thread>
#include <atomic>

bool shouldTerminate = false;

void socketThread(ConnectionHandler& connectionHandler) {
    
    std::string line;

    while (!shouldTerminate) {

    if (!connectionHandler.getLine(line)) {
            std::cout << "Disconnected. Exiting...\n" << std::endl;
            shouldTerminate = true;
            break;
        }
    

    line.resize(line.length()-1);

    std::cout << "Reply: " << line << " " << line.length() << " bytes " << std::endl << std::endl;
    if (line == "bye") {
        std::cout << "Exiting...\n" << std::endl;
        shouldTerminate = true;
        }
    }

    return;
    }

/**
* This code assumes that the server replies the exact text the client sent it (as opposed to the practical session example)
*/
int main (int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " host port" << std::endl << std::endl;
        return -1;
    }

    std::string host = argv[1];
    short port = atoi(argv[2]);

    ConnectionHandler connectionHandler(host, port);
    if (!connectionHandler.connect()) {
        std::cerr << "Cannot connect to " << host << ":" << port << std::endl;
        return 1;
    }
	
    std::thread t1 = std::thread(socketThread, std::ref(connectionHandler));

	//From here we will see the rest of the ehco client implementation:
    while (!shouldTerminate) {
        const short bufsize = 1024;
        char buf[bufsize];

        std::cin.getline(buf, bufsize);
		std::string line(buf);

		int len=line.length();
        if (!connectionHandler.sendLine(line)) {
            std::cout << "Disconnected. Exiting...\n" << std::endl;
            shouldTerminate = true;
            break;
        }
		// connectionHandler.sendLine(line) appends '\n' to the message. Therefor we send len+1 bytes.
        std::cout << "Sent " << len+1 << " bytes to server" << std::endl;
    }

    t1.join();

    return 0;
}
