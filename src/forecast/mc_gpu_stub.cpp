// Used when the project is built without CUDA.
#include <stdexcept>

#include "mc.hpp"

namespace e47 {

bool cuda_available() { return false; }

McResult run_mc_gpu(const McInputs&, const std::vector<float>&, uint64_t, uint64_t, int) {
  throw std::runtime_error("built without CUDA (configure with -DENABLE_CUDA=ON)");
}

}  // namespace e47
