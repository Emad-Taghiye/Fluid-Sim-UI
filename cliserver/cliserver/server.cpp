#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <vector>
#include <netinet/tcp.h>
#include <sstream>


#define PORT 5000
#define BUFFER_SIZE 1024

bool readCommand(int clientSocket, std::string &command) {
    char buffer[BUFFER_SIZE] = {0};
    ssize_t valread = read(clientSocket, buffer, BUFFER_SIZE - 1);
    if (valread <= 0) {
        return false;
    }
    command = std::string(buffer, valread);
    std::cout << "Received command: " << command << std::endl;
    return true;
}

std::vector<std::string> split_by_space(const std::string& str) {
    std::istringstream iss(str);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token)
        tokens.push_back(token);
    return tokens;
}

int main() {
    int server_fd, clientSocket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                   &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    std::cout << "Server listening on port " << PORT << "...\n";

    if (listen(server_fd, 1) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    if ((clientSocket = accept(server_fd, (struct sockaddr*)&address,
                               (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    std::cout << "Client connected.\n";
    int flag = 1;
    setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));


    // Wait for command
    std::string command;
    while (true) {
        if (!readCommand(clientSocket, command)) {
            std::cout << "Client disconnected or error.\n";
            break;
        }

        if (command.find("run ./simulator") != std::string::npos) {
            std::cout << "Starting simulator...\n";

            pid_t pid = fork();
            if (pid == 0) {
                // In child: Redirect stdout to client socket
                
            } else if (pid > 0) {
                // In parent: wait for child to finish
                int status;
                waitpid(pid, &status, 0);
                std::cout << "Simulator finished.\n";
                break;  // End after simulator runs
            } else {
                perror("fork failed");
            }
        } else if (command.find("run salloc") != std::string::npos) {
            std::vector<std::string> tokens = split_by_space(command);
            std::string last = "";

            if (!tokens.empty()) {
                last = tokens.back();  // "baboon"
            } else {
                std::cout << "No tokens found in command." << std::endl;
        }
            pid_t pid = fork();
            if (pid == 0) {
                std::string fd_str = std::to_string(clientSocket);
                execl("./fluid_sim_starter", "./fluid_sim_starter", fd_str.c_str(), last.c_str(), nullptr);
                perror("Failed to exec fluid_sim_starter");
                exit(EXIT_FAILURE);
            }
        } else {
            std::cout << "Unknown command received: " << command << std::endl;
        }
    }

    close(clientSocket);
    close(server_fd);
    return 0;
}
