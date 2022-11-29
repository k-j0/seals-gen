#pragma once

#ifdef CUDA

#include <cstdio>
#include <cstdlib>
#include <string>
#include <cuda.h>

// Wrappers around cuda calls to stop upon runtime errors
inline void __cuda_check(cudaError_t code, const char* fn, const char* file, int line) {
    if (code != cudaSuccess) {
        std::printf("\nCUDA error %d running %s\n%s\nIn %s line %d\n\n", code, fn, cudaGetErrorString(code), file, line);
        std::exit(1);
    }
}
#define CUDA_CHECK(op) __cuda_check((op), #op, __FILE__, __LINE__)
#define CUDA_CHECK_K __cuda_check(cudaPeekAtLastError(), "kernel", __FILE__, __LINE__)

#else
#define CUDA_CHECK(op) (op)
#define CUDA_CHECK_K
#endif
