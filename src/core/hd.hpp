#pragma once
// Marks functions that compile for both host (CPU) and device (CUDA) code.
#if defined(__CUDACC__)
#define E47_HD __host__ __device__ __forceinline__
// Host-device templates may call device-only callables (e.g. a lambda that does
// atomicAdd) when instantiated from a kernel; silence the host-side check.
#define E47_HD_TEMPLATE _Pragma("nv_exec_check_disable")
#else
#define E47_HD inline
#define E47_HD_TEMPLATE
#endif
