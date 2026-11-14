#pragma once
// Marks functions that compile for both host (CPU) and device (CUDA) code.
#if defined(__CUDACC__)
#define E47_HD __host__ __device__ __forceinline__
#else
#define E47_HD inline
#endif
