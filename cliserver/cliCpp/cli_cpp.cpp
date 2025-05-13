#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 7000
#define BUFFER_SIZE 1024

void sendMessage(int sock, const std::string& msg) {
    int msgLength = msg.size();
    send(sock, &msgLength, sizeof(msgLength), 0);
    send(sock, msg.c_str(), msg.size(), 0);
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error\n";
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported\n";
        return -1;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed\n";
        return -1;
    }

    fd_set readfds;
    int max_fd = std::max(sock, STDIN_FILENO) + 1;

    std::cout << "Connected. Type 'run ./my_program' to start a program, or type commands directly.\n";

    while (true) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);  // Monitor keyboard
        FD_SET(sock, &readfds);          // Monitor socket

        int activity = select(max_fd, &readfds, NULL, NULL, NULL);

        if (activity < 0) {
            std::cerr << "Select error\n";
            break;
        }

        // Check if user typed something
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            std::string input;
            std::getline(std::cin, input);
            if (input.empty()) continue;

            sendMessage(sock, input);

            if (input == "exit") {
                std::cout << "Exiting.\n";
                break;
            }
        }

        // Check if server sent something
        if (FD_ISSET(sock, &readfds)) {
            memset(buffer, 0, BUFFER_SIZE);
            ssize_t n = read(sock, buffer, BUFFER_SIZE - 1);
            if (n > 0) {
                buffer[n] = '\0';
                std::cout << buffer << std::flush;
            } else if (n == 0) {
                std::cout << "\nServer closed the connection.\n";
                break;
            } else {
                std::cerr << "Read error\n";
                break;
            }
        }
    }

    close(sock);
    return 0;
}
