#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <chrono>
#include <thread>

std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return {};
    }
    std::vector<char> data((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
    return data;
}

bool sendFrame(const std::vector<char>& data) {
    int32_t size = data.size();
    int32_t size_le = htole32(size);  // little-endian

    // Write 4-byte size header
    if (write(STDOUT_FILENO, &size_le, sizeof(size_le)) != sizeof(size_le)) {
        std::cerr << "Failed to write frame size." << std::endl;
        return false;
    }

    // Write the frame data in chunks
    size_t totalSent = 0;
    while (totalSent < data.size()) {
        ssize_t sent = write(STDOUT_FILENO, data.data() + totalSent, data.size() - totalSent);
        if (sent <= 0) {
            std::cerr << "Failed to write frame data." << std::endl;
            return false;
        }
        totalSent += sent;
    }

    return true;
}

int main() {
    std::vector<std::string> frameFiles = {
        // "../CUDA/Probabilistic_Fluid_Simulation/inputs/images/baboon.png",
        // "../CUDA/Probabilistic_Fluid_Simulation/inputs/images/peppers.png",
        // "../CUDA/Probabilistic_Fluid_Simulation/inputs/images/tulips.png"
        "../CUDA/Probabilistic_Fluid_Simulation/inputs/images/Checker16_1024.png",
        "../CUDA/Probabilistic_Fluid_Simulation/inputs/velocity_fields/perlin/T0/PerlinRG_256.png"
    };
    std::vector<std::vector<char>> frames;

    // Load frames into memory
    for (const auto& file : frameFiles) {
        auto data = readFile(file);
        if (data.empty()) {
            std::cerr << "Error loading " << file << std::endl;
            return 1;
        }
        frames.push_back(data);
    }

    // Send frames in a loop forever
    while (true) {
        for (const auto& frame : frames) {
            if (!sendFrame(frame)) {
                std::cerr << "Error sending frame. Exiting simulator." << std::endl;
                return 1;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

        }
    }
    return 0;
}
