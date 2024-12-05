#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <optional>//Used to represent values that may or may not be present
#include <unistd.h>
#include <thread>
#include <queue>
#include <chrono>
#include <arpa/inet.h>
#include <sstream>
#include <string>
#include <future>
#include <iomanip>  // Include for std::get_time
//#include<server.h>
//#include "socket_utils.h"

using namespace std;

struct Event {
    int priority;
    optional<string> data; // Optional data for the event it holds string for additional data
    chrono::time_point<chrono::system_clock> time; // Server timestamp

    bool operator<(const Event& other) const {
        return priority > other.priority; // Higher priority comes first
    }
};

priority_queue<Event> eventQueue;

optional<int> createSocket1() {
    int serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSock < 0) {
        cerr << "Failed to create socket" << endl;
        return nullopt;
    }
    return serverSock;
}

optional<bool> bindSocket(int serverSocket, const sockaddr_in &serverAddress) {
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        cerr << "Binding socket failed" << endl;//The address structure is cast to a sockaddr*, which is a generic pointer type required by the bind function.
        return nullopt;
    }
    return true;//bind associates a socket with a specific local IP address and port number.
}

void receiveAndPushToQueue(int socketfd) {
    char buffer[1024] = {0};
    ssize_t received;

    while ((received = recv(socketfd, buffer, sizeof(buffer), 0)) > 0) {//The buffer where the received data will be stored.
        auto now = std::chrono::system_clock::now();//The now variable holds a time point that represents the exact current moment as understood by the system clock.now() fun of s/m clk
        std::time_t serverTime = std::chrono::system_clock::to_time_t(now);//time_point now converts to time_t
        char timeBuffer[100];
        std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", std::localtime(&serverTime));
//std::localtime(&serverTime): Converts the serverTime from time_t to tm struct, which represents local time.
        istringstream iss(string(buffer, received));//isstringstream Create a Readable Stream from Raw Data and string is Convert Raw Buffer to String:
        string line;//line of type std::string that will be used to store each line read from the istringstream.

        while (getline(iss, line)) {//read each line
            if (line.empty()) continue;

            istringstream lineStream(line);//string as a stream data can be read sequenntially
            int priority;
            string data, clientTimestampStr;

            if (lineStream >> priority) {//for ex:"2:burn" the >> reads only priority 2 from linestream
                lineStream.ignore(1); // Ignore the space or delimiter
                getline(lineStream, data, '-'); // Read until the dash This line reads from the lineStream into the data variable until it encounters a dash (-), which separates the event data from the timestamp.
                getline(lineStream, clientTimestampStr);

                // Parse the client timestamp
                std::tm clientTime{};
                std::istringstream(clientTimestampStr) >> std::get_time(&clientTime, "%Y-%m-%d %H:%M:%S");//this creates input stream clientTimestampStr and read the values into variables for get_time ex:clientTime.tm_year
                std::time_t clientTimeT = std::mktime(&clientTime);//std::mktime takes a std::tm structure and converts it to std::time_t, means it converts readable format
                // Calculate time difference
                double timeDiff = std::difftime(serverTime, clientTimeT);
                
                eventQueue.push(Event{priority, data, now});
                cout << "Data received and pushed to the event queue at " 
                     << timeBuffer << ": " << priority << " - " << data << endl;
                cout << "Client Timestamp: " << clientTimestampStr << endl;
                cout << "Server Timestamp: " << timeBuffer << endl;
                cout << "Time Difference: " << timeDiff << " seconds" << endl;
                
                //print nano, mili,micro sec
             //auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            //cout << "Current time since epoch: " << now_ms << " milliseconds" << endl;
            }
        }
    }

    if (received < 0) {
        cerr << "Receiving data failed" << endl;
    }
}

void processEvents() {
    while (true) {
        if (!eventQueue.empty()) {
         auto start = std::chrono::high_resolution_clock::now(); // Start timing
            Event event = eventQueue.top();
            eventQueue.pop();

            cout << "Processing event data: " << event.priority << " - " 
                 << (event.data ? *event.data : "No Data") << endl;
           auto end = std::chrono::high_resolution_clock::now(); // End timing high resolution clock is the clock with the tick period  available on the system
            std::chrono::duration<double, std::milli> duration = end - start; // Duration in milliseconds This /////calculates the duration of time taken to process the event by subtracting the start time from the end time. The result is stored in a std::chrono::duration object that is specified in milliseconds.
            cout << "Time taken to process event: " << duration.count() << " milliseconds" << endl;//its give total number of millisec

        }
        
    }
}

int main() {
    auto serverSocket = createSocket1();
    if (!serverSocket) return -1;

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);

    if (!bindSocket(*serverSocket, serverAddress)) return -1;

    listen(*serverSocket, 5);//5 is max num of pending connections
    cout << "Waiting for a client to connect..." << endl;

    int clientSocket = accept(*serverSocket, nullptr, nullptr);
    if (clientSocket < 0) {//nullptr is used because the server doesn’t need to retrieve the client’s address for this example.
        cerr << "Accepting connection failed" << endl;
        return -1;
    }

    receiveAndPushToQueue(clientSocket);

    std::future<void> eventProcessingFuture = std::async(std::launch::async, processEvents);
    //future is hold the value of asynchronous and std::async is This function is used to run a task asynchronously
    //std::launch::async this will be run in new thread and processEvents is executed asynchronously
    close(clientSocket);
    close(*serverSocket);
    return 0;
}







