
#include "cuda_utils.h"
#include "cuda_info.h"

std::string getCudaInfo () {
    
    std::string out = "CUDA enabled, ";
    
    int numDevices;
    CUDA_CHECK(cudaGetDeviceCount(&numDevices));
    out += std::to_string(numDevices) + " device" + (numDevices != 1 ? "s" : "") + ":";
    
    for (int i = 0; i < numDevices; ++i) {
        cudaDeviceProp props;
        CUDA_CHECK(cudaGetDeviceProperties(&props, i));
        out += "\n\t- " + std::string(props.name);
        out += ", SM " + std::to_string(props.major) + "." + std::to_string(props.minor);
        if (props.isMultiGpuBoard) {
            out += " (Multi-GPU board)";
        }
        if (props.integrated) {
            out += " (integrated)";
        }
        out += "\n\t\t" + std::to_string(props.clockRate / 1000) + "MHz (memory: " + std::to_string(props.memoryClockRate / 1000) + "MHz)";
        out += "\n\t\t" + std::to_string(props.asyncEngineCount) + " async engines, " + std::to_string(props.multiProcessorCount) + " multiprocessors";
    }
    
    return out;
}
