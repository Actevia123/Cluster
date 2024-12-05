#include <iostream>
#include <sys/socket.h> //It provides declarations for the socket functions such as socket(), bind(), listen(), accept(), and connect(). 
#include <netinet/in.h>//its particularly specifies IPv4 version and structure address also
#include <optional>
#include <unistd.h>
#include <cstring>//It provides functions like strerror() to get human-readable error messages
#include <arpa/inet.h>// functions inet_pton(), which converts a human-readable IP address (like "127.0.0.1") into a  struct in_addr format used in sockets.
#include <cerrno>//error purpose
#include <string>
#include<chrono>
//#include<client.h>
//#include "socket_utils.h"
//#include <thread>

using namespace std;

optional<int> createSocket() {
 auto handleError = [](const char* msg) {
        cout << msg << ": " << strerror(errno) << endl;
    };
    int clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//specifies the address family IPv4 ,TCP,protocol
    if (clientSocket < 0) {
       // cout << "Failed to create socket: " << strerror(errno) << endl;
        handleError("Failed to create socket");
        return nullopt;
    }
    return clientSocket;
}

optional<bool> connectToServer(int socketfd, const sockaddr_in &serverAddress) {
    auto connectionStatus = connect(socketfd, (struct sockaddr*)&serverAddress, sizeof(serverAddress));//The address of the server to connect to, cast to a generic pointer type (sockaddr*). This is necessary because the connect function expects a pointer to a sockaddr structure.
    if (connectionStatus < 0) {
        cout << "Connection to server failed: " << strerror(errno) << endl;//this function provides human readable str
        return nullopt;
    }
    return true;
}

void eventSendToServer(int socketfd, int priority, const char* message) {
    auto now = std::chrono::system_clock::now();//now captures the current time using the system clock.
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);//currentTime converts the now time point into a time_t format.

    // Prepare message with timestamp
    char timeBuffer[100];
    std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", std::localtime(&currentTime));//std::localtime(&currentTime): This function converts a std::time_t value (in this case, currentTime) to a std::tm structure that represents local time. The & operator is used to pass the address of currentTime. The result is a pointer to a std::tm structure that contains the local time representation. and strftime is formates date and time
    
    // Format: priority:message - timestamp
    string fullMessage = to_string(priority) + ":" + message + " - " + timeBuffer + "\n"; 
   
    size_t sent = send(socketfd, fullMessage.data(), fullMessage.size(), 0);//ssize_t send(int sockfd, const void *buf, size_t len, int flags);2nd parameter returns pointer to a array

  
    if (sent < 0) {
        cout << "Error sending data: " << strerror(errno) << endl;
    } else {
        cout << "Sent data: " << fullMessage;
        
    
    }
}


int main() {
    auto sockfd = createSocket();
    if (!sockfd) return -1;

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;  
    serverAddress.sin_port = htons(8080);//converts the port number to network byte order.
    inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);//converts the IP address from text to binary form.

    if (!connectToServer(*sockfd, serverAddress)) {// deferencing pass actual value bcz optional If we tried to pass sockfd directly, we would be passing the     optional wrapper instead of the actual socket descriptor. 
        close(*sockfd);
        return -1;
    }

    // Send all messages
    eventSendToServer(*sockfd, 1, "basamma");
    eventSendToServer(*sockfd, 3, "actevia");
    eventSendToServer(*sockfd, 2, "burn");
    eventSendToServer(*sockfd, 4, "people");

    close(*sockfd); // Close socket after sending all messages
    return 0;
}



