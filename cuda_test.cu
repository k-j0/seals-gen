
#include <cstdio>
#include <cstdlib>
#include <string>
#include <cuda.h>
#include "cuda_test.h"


// Wrappers around cuda calls to stop upon runtime errors
inline void __cuda_check(cudaError_t code, const char* fn, const char* file, int line) {
    if (code != cudaSuccess) {
        std::printf("\nCUDA error %d running %s\n%s\nIn %s line %d\n\n", code, fn, cudaGetErrorString(code), file, line);
        std::exit(1);
    }
}
#define CUDA_CHECK(op) __cuda_check((op), #op, __FILE__, __LINE__)
#define CUDA_CHECK_K __cuda_check(cudaPeekAtLastError(), "kernel", __FILE__, __LINE__)



__global__ void vadd_k(float* a, float* b, int n) {
    int idx = threadIdx.x + blockIdx.x * blockDim.x;
    if (idx < n) {
        a[idx] += b[idx];
    }
}

void testCuda (int n, int blockSize) {
    std::printf("Starting CUDA test, N = %d, block size = %d.\n", n, blockSize);
    
    // Alloc resources
    float* x = (float*)std::malloc(n * sizeof(float));
    float* y = (float*)std::malloc(n * sizeof(float));
    float* x_d;
    float* y_d;
    CUDA_CHECK(cudaMalloc(&x_d, n*sizeof(float)));
    CUDA_CHECK(cudaMalloc(&y_d, n*sizeof(float)));
    for (int i = 0; i < n; ++i){
        x[i] = 3;
        y[i] = 7;
    }
    
    // Copy to device
    CUDA_CHECK(cudaMemcpy(x_d, x, n*sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(y_d, y, n*sizeof(float), cudaMemcpyHostToDevice));
    
    // Run kernel
    vadd_k<<<n/blockSize+(n%blockSize!=0), blockSize>>>(x_d, y_d, n);
    CUDA_CHECK_K;
    CUDA_CHECK(cudaDeviceSynchronize());
    
    // Copy results back to host
    CUDA_CHECK(cudaMemcpy(x, x_d, n*sizeof(float), cudaMemcpyDeviceToHost));
    
    // Check results
    std::printf("Expecting 10s:\n");
    for (int i = 0; i < n && i < 100; ++i) {
        std::printf("%f ", x[i]);
    }
    std::printf("\n\n");
    bool valid = true;
    for (int i = 0; i < n; ++i) {
        if (x[i] != 10) {
            valid = false;
            break;
        }
    }
    if (valid) {
        std::printf("Valid across all %d elements.\n", n);
    } else {
        std::printf("Test was invalid on at least one element.\n");
    }
    
    // Clean up
    CUDA_CHECK(cudaFree(x_d));
    CUDA_CHECK(cudaFree(y_d));
    std::free(x);
    std::free(y);
    std::printf("Finished CUDA test.\n");
}
