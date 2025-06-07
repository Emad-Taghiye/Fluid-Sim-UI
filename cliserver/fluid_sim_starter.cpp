#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>

#define BUFFER_SIZE 2048

int main(int argc, char *argv[]) {
    std::cout << "Launching fluidsim_cuda...\n";

    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <client_socket_fd> <imaage_name>\n";
        return 1;
    }

    std::cout << "text is " << "../CUDA/Probabilistic_Fluid_Simulation/inputs/images/" + std::string(argv[2]) + ".png" << "\n";

    execl("../CUDA/Probabilistic_Fluid_Simulation/build/fluidsim_cuda",
        "fluidsim_cuda",
        "400", "1000.0", "0",
        ("../CUDA/Probabilistic_Fluid_Simulation/inputs/images/" + std::string(argv[2]) + ".png").c_str(),
        "../CUDA/Probabilistic_Fluid_Simulation/inputs/velocity_fields/voronoi/T0/VoronoiRG_256.png",
        argv[1], nullptr);
}
