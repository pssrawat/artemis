#include "cuda.h"
#include "stdio.h"

// extern __host__ __device__ int MAX(int a, int b) { return a > b ? a : b; }
// extern __host__ __device__ int MIN(int a, int b) { return a < b ? a : b; }
// extern __host__ __device__ int CEIL(int a, int b) { return ( (a) % (b) == 0 ? (a) / (b) :  ( (a) / (b) + 1 ) ); }

void Check_CUDA_Error(const char* message){
  cudaError_t error = cudaGetLastError();
  if( error != cudaSuccess ){
    printf("CUDA-ERROR:%s, %s\n",message,cudaGetErrorString(error) ); 
    exit(-1);
  }
}
