#pragma once
// Philox 4x32-10 counter-based random number generator
// (Salmon et al., "Parallel random numbers: as easy as 1, 2, 3", SC 2011).
//
// A counter-based generator has no sequential state. The output for a given
// (key, counter) pair is a pure function, so the CPU and GPU paths produce the
// same random inputs for the same scenario index regardless of thread layout.
#include <cstdint>
#include <cmath>
#include "hd.hpp"

namespace e47 {

struct U32x4 {
  uint32_t x, y, z, w;
};

E47_HD void mulhilo32(uint32_t a, uint32_t b, uint32_t& hi, uint32_t& lo) {
  const uint64_t p = static_cast<uint64_t>(a) * static_cast<uint64_t>(b);
  hi = static_cast<uint32_t>(p >> 32);
  lo = static_cast<uint32_t>(p);
}

E47_HD U32x4 philox4x32_10(U32x4 c, uint32_t k0, uint32_t k1) {
  constexpr uint32_t M0 = 0xD2511F53u, M1 = 0xCD9E8D57u;
  constexpr uint32_t W0 = 0x9E3779B9u, W1 = 0xBB67AE85u;
#if defined(__CUDACC__)
#pragma unroll
#endif
  for (int r = 0; r < 10; ++r) {
    uint32_t hi0, lo0, hi1, lo1;
    mulhilo32(M0, c.x, hi0, lo0);
    mulhilo32(M1, c.z, hi1, lo1);
    c = U32x4{hi1 ^ c.y ^ k0, lo1, hi0 ^ c.w ^ k1, lo0};
    k0 += W0;
    k1 += W1;
  }
  return c;
}

// Uniform in (0, 1], 24-bit resolution (float) or 32-bit resolution (double).
E47_HD float u01f(uint32_t v) { return (static_cast<float>(v >> 8) + 1.0f) * (1.0f / 16777216.0f); }
E47_HD double u01d(uint32_t v) { return (static_cast<double>(v) + 1.0) * (1.0 / 4294967296.0); }

// Stream of random numbers for one scenario draw.
// counter = (draw_lo, draw_hi, step, purpose); key = seed.
struct DrawRng {
  uint32_t k0, k1, draw_lo, draw_hi;

  E47_HD DrawRng(uint64_t seed, uint64_t draw)
      : k0(static_cast<uint32_t>(seed)), k1(static_cast<uint32_t>(seed >> 32)),
        draw_lo(static_cast<uint32_t>(draw)), draw_hi(static_cast<uint32_t>(draw >> 32)) {}

  E47_HD U32x4 bits(uint32_t step, uint32_t purpose) const {
    return philox4x32_10(U32x4{draw_lo, draw_hi, step, purpose}, k0, k1);
  }

  // Four uniforms in (0, 1].
  template <class Real>
  E47_HD void uniform4(uint32_t step, uint32_t purpose, Real out[4]) const {
    const U32x4 b = bits(step, purpose);
    if constexpr (sizeof(Real) == 4) {
      out[0] = u01f(b.x); out[1] = u01f(b.y); out[2] = u01f(b.z); out[3] = u01f(b.w);
    } else {
      out[0] = u01d(b.x); out[1] = u01d(b.y); out[2] = u01d(b.z); out[3] = u01d(b.w);
    }
  }

  // Four standard normals via the Box-Muller transform.
  template <class Real>
  E47_HD void normal4(uint32_t step, uint32_t purpose, Real out[4]) const {
    using std::cos;
    using std::log;
    using std::sin;
    using std::sqrt;
    Real u[4];
    uniform4<Real>(step, purpose, u);
    const Real two_pi = Real(6.283185307179586);
    const Real r0 = sqrt(Real(-2) * log(u[0]));
    const Real r1 = sqrt(Real(-2) * log(u[2]));
    out[0] = r0 * cos(two_pi * u[1]);
    out[1] = r0 * sin(two_pi * u[1]);
    out[2] = r1 * cos(two_pi * u[3]);
    out[3] = r1 * sin(two_pi * u[3]);
  }
};

}  // namespace e47
