#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define PORT 5000
#define BUFFER_SIZE 1024

bool readFull(int socket, char* buffer, size_t length) {
    size_t totalRead = 0;
    while (totalRead < length) {
        ssize_t bytesRead = read(socket, buffer + totalRead, length - totalRead);
        if (bytesRead <= 0) return false;  // Error or connection closed
        totalRead += bytesRead;
    }
    return true;
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 1) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    std::cout << "Server listening on port " << PORT << "...\n";

    if ((client_socket = accept(server_fd, (struct sockaddr*)&address,
                                (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    std::cout << "Client connected.\n";

    int child_stdin[2], child_stdout[2];
    pid_t child_pid = -1;
    bool running = false;

    while (true) {
        // First read the message length
        int msgLength = 0;
        if (!readFull(client_socket, (char*)&msgLength, sizeof(msgLength))) {
            std::cout << "Client disconnected (length read).\n";
            break;
        }

        if (msgLength <= 0 || msgLength > 10000) {
            std::cerr << "Invalid message length: " << msgLength << "\n";
            break;
        }

        // Read the actual message
        char* buffer = new char[msgLength + 1];
        memset(buffer, 0, msgLength + 1);
        if (!readFull(client_socket, buffer, msgLength)) {
            std::cout << "Client disconnected (msg read).\n";
            delete[] buffer;
            break;
        }

        std::string message(buffer, msgLength);
        delete[] buffer;
        std::cout << "Received: " << message << "\n";

        if (message == "exit") {
            std::string msg = "Closing connection.\n";
            send(client_socket, msg.c_str(), msg.size(), 0);
            break;
        }

        if (message.rfind("run ", 0) == 0) {  // Start new interactive program
            if (running) {
                std::string err = "A program is already running.\n";
                send(client_socket, err.c_str(), err.size(), 0);
                continue;
            }

            std::string cmd = message.substr(4);

            // Set up pipes
            pipe(child_stdin);
            pipe(child_stdout);

            child_pid = fork();
            if (child_pid == 0) {
                // Child process
                dup2(child_stdin[0], STDIN_FILENO);
                dup2(child_stdout[1], STDOUT_FILENO);
                dup2(child_stdout[1], STDERR_FILENO);

                close(child_stdin[1]);
                close(child_stdout[0]);

                execlp(cmd.c_str(), cmd.c_str(), NULL);
                perror("execlp failed");
                exit(1);
            } else if (child_pid > 0) {
                // Parent process
                close(child_stdin[0]);
                close(child_stdout[1]);

                fcntl(child_stdout[0], F_SETFL, O_NONBLOCK);
                running = true;
                std::string msg = "Program started: " + cmd + "\n";
                send(client_socket, msg.c_str(), msg.size(), 0);
            } else {
                std::string err = "Failed to fork process.\n";
                send(client_socket, err.c_str(), err.size(), 0);
            }
        }
        else if (running) {
            // Input to the running interactive program
            write(child_stdin[1], message.c_str(), message.size());
            std::cout << "[Sent to child stdin]: " << message << std::endl;
        }
        else {
            // 🚨 New: Handle normal one-shot commands (like ls, whoami)
            std::cout << "Executing one-shot command: " << message << std::endl;

            FILE* pipe = popen(message.c_str(), "r");
            if (!pipe) {
                std::string error = "Failed to run command\n";
                send(client_socket, error.c_str(), error.size(), 0);
            } else {
                char outBuf[BUFFER_SIZE];
                while (fgets(outBuf, sizeof(outBuf), pipe) != nullptr) {
                    send(client_socket, outBuf, strlen(outBuf), 0);
                }
                pclose(pipe);
            }
        }

        // Check if child has output
        if (running) {
            char outBuf[BUFFER_SIZE];
            ssize_t n = read(child_stdout[0], outBuf, BUFFER_SIZE - 1);
            while (n > 0) {
                outBuf[n] = '\0';
                send(client_socket, outBuf, n, 0);
                n = read(child_stdout[0], outBuf, BUFFER_SIZE - 1);
            }

            // Check if child has exited
            int status;
            if (waitpid(child_pid, &status, WNOHANG) > 0) {
                std::string msg = "\n[Program exited]\n";
                send(client_socket, msg.c_str(), msg.size(), 0);
                close(child_stdin[1]);
                close(child_stdout[0]);
                running = false;
            }
        }
    }

    close(client_socket);
    close(server_fd);
    return 0;
}
