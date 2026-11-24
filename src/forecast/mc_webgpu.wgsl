// Monte Carlo ensemble on the browser's GPU (WebGPU compute shader; web build only).
//
// A direct port of the float instantiation of simulate_draw() and hist_bin() in
// src/forecast/mc.hpp and of the Philox generator in src/core/philox.hpp. Keep this
// file in step with those two: a change to the per-draw physics there must be made
// here as well. The browser self-test (/app/?selftest=1) runs this kernel and the
// CPU float path on the same inputs and seed and compares them draw for draw.
//
// The host side (src/forecast/mc_webgpu.cpp) embeds this file at build time.
// One invocation simulates one draw; histograms are updated with atomicAdd, as in
// the CUDA driver (src/forecast/mc_gpu.cu). WGSL has no 64-bit integers, so the seed
// and the draw index are pairs of u32 words, and Philox's 32 x 32 -> 64-bit product
// is rebuilt exactly from 16-bit halves.

const kMaxYears: u32 = 32u;
const kBins: u32 = 1024u;
const kBootStride: u32 = 8u;
const kKeepFeatures: u32 = 8u;

const kFeQuad: i32 = 0;
const kFeGompertz: i32 = 1;
const kDamped: i32 = 2;
const kFeatureUniform: i32 = 1;

// Real(6.283185307179586) in normal4(): the same f32 value.
const kTwoPi: f32 = 6.283185307179586;

// Mirror of struct McInputs (mc.hpp), member for member. Every member is a 4-byte
// int or float, so this storage layout equals the C++ layout byte for byte;
// mc_webgpu.cpp checks the size and the offsets with static_assert.
struct McInputs {
  base_year: i32,
  n_years: i32,
  ln_e0: f32,
  y0: f32,
  u0: f32,
  gen0_twh: f32,
  pop_med: array<f32, 32>,
  pop_sd: array<f32, 32>,
  urb_med: array<f32, 32>,
  urb_sd: array<f32, 32>,
  g_mean: array<f32, 32>,
  g_level_sd: f32,
  g_shock_sd: f32,
  g_rho: f32,
  w_model: array<f32, 3>,
  n_boot: i32,
  quad_only: i32,
  resid0: f32,
  resid_rho: f32,
  resid_innov_sd: f32,
  dt_level: f32,
  dt_trend: f32,
  dt_phi: f32,
  dt_alpha: f32,
  dt_beta: f32,
  dt_sigma: f32,
  addon_lo: f32,
  addon_hi: f32,
  n_features: i32,
  feat_kind: array<i32, 16>,
  feat_sign: array<f32, 16>,
  feat_lo: array<f32, 16>,
  feat_mode: array<f32, 16>,
  feat_hi: array<f32, 16>,
  n_states: i32,
  state_region: array<i32, 40>,
  state_pop_share: array<array<f32, 32>, 40>,
  state_ln_rel_income: array<f32, 40>,
  conv_mean: f32,
  conv_sd: f32,
  rel_income_lag: f32,
  hist_lo: array<array<f32, 32>, 6>,
  hist_hi: array<array<f32, 32>, 6>,
}

// One dispatch covers draws [first, first + count) of the run.
struct Params {
  seed_lo: u32,
  seed_hi: u32,
  first: u32,
  count: u32,
  n_keep: u32,
  pad0: u32,
  pad1: u32,
  pad2: u32,
}

@group(0) @binding(0) var<storage, read> inp: McInputs;
@group(0) @binding(1) var<storage, read> boot: array<f32>;                 // [n_boot][kBootStride]
@group(0) @binding(2) var<storage, read_write> hist: array<atomic<u32>>;   // [kSeries][kMaxYears][kBins]
@group(0) @binding(3) var<storage, read_write> keep: array<f32>;           // [n_keep][kKeepFeatures]
@group(0) @binding(4) var<uniform> prm: Params;

// --- Philox 4x32-10 (philox.hpp) ------------------------------------------------

// mulhilo32: the 64-bit product a * b as (high word, low word). The four 16 x 16-bit
// partial products are below 2^32 each, so no step overflows; u32 multiplication
// wraps modulo 2^32 in WGSL, which gives the low word directly.
fn mulhilo32(a: u32, b: u32) -> vec2<u32> {
  let a0 = a & 0xFFFFu;
  let a1 = a >> 16u;
  let b0 = b & 0xFFFFu;
  let b1 = b >> 16u;
  let p00 = a0 * b0;
  let p01 = a0 * b1;
  let p10 = a1 * b0;
  let p11 = a1 * b1;
  let mid = (p00 >> 16u) + (p01 & 0xFFFFu) + (p10 & 0xFFFFu);  // < 3 x 2^16
  let hi = p11 + (p01 >> 16u) + (p10 >> 16u) + (mid >> 16u);
  return vec2<u32>(hi, a * b);
}

fn philox4x32_10(c_in: vec4<u32>, k0_in: u32, k1_in: u32) -> vec4<u32> {
  var c = c_in;
  var k0 = k0_in;
  var k1 = k1_in;
  for (var r = 0; r < 10; r++) {
    let p0 = mulhilo32(0xD2511F53u, c.x);
    let p1 = mulhilo32(0xCD9E8D57u, c.z);
    c = vec4<u32>(p1.x ^ c.y ^ k0, p1.y, p0.x ^ c.w ^ k1, p0.y);
    k0 += 0x9E3779B9u;  // wraps modulo 2^32, as uint32_t does
    k1 += 0xBB67AE85u;
  }
  return c;
}

// Uniform in (0, 1], 24-bit resolution: u01f(). Exact in f32, so equal on CPU and GPU.
fn u01f(v: u32) -> f32 {
  return (f32(v >> 8u) + 1.0) * (1.0 / 16777216.0);
}

// DrawRng: key = seed, counter = (draw_lo, draw_hi, step, purpose).
struct DrawRng {
  k0: u32,
  k1: u32,
  draw_lo: u32,
  draw_hi: u32,
}

fn rng_bits(g: DrawRng, step: u32, purpose: u32) -> vec4<u32> {
  return philox4x32_10(vec4<u32>(g.draw_lo, g.draw_hi, step, purpose), g.k0, g.k1);
}

fn uniform4(g: DrawRng, step: u32, purpose: u32) -> vec4<f32> {
  let b = rng_bits(g, step, purpose);
  return vec4<f32>(u01f(b.x), u01f(b.y), u01f(b.z), u01f(b.w));
}

// Four standard normals by the Box-Muller transform (DrawRng::normal4).
fn normal4(g: DrawRng, step: u32, purpose: u32) -> vec4<f32> {
  let u = uniform4(g, step, purpose);
  let r0 = sqrt(-2.0 * log(u.x));
  let r1 = sqrt(-2.0 * log(u.z));
  return vec4<f32>(r0 * cos(kTwoPi * u.y), r0 * sin(kTwoPi * u.y), r1 * cos(kTwoPi * u.w), r1 * sin(kTwoPi * u.w));
}

// --- Model pieces (mc.hpp) --------------------------------------------------------

fn clamp01(v: f32) -> f32 {
  return select(select(v, 1.0, v > 1.0), 0.0, v < 0.0);
}

// feature_inverse_cdf(): one demand-feature draw at u in (0, 1].
fn feature_inverse_cdf(kind: i32, lo: f32, mode: f32, hi: f32, u: f32) -> f32 {
  let w = hi - lo;
  if (!(w > 0.0)) {
    return lo;
  }
  if (kind == kFeatureUniform) {
    return lo + w * u;
  }
  let a = mode - lo;
  if (u * w <= a) {
    return lo + sqrt(u * w * a);
  }
  return hi - sqrt((1.0 - u) * w * (hi - mode));
}

// feature_draw_total(): sum of sign x value over the demand features; stream 6.
fn feature_draw_total(g: DrawRng) -> f32 {
  var s = 0.0;
  var u = vec4<f32>(0.0, 0.0, 0.0, 0.0);
  for (var j = 0; j < inp.n_features; j++) {
    if ((j & 3) == 0) {
      u = uniform4(g, u32(j >> 2u), 6u);
    }
    s += inp.feat_sign[j] * feature_inverse_cdf(inp.feat_kind[j], inp.feat_lo[j], inp.feat_mode[j], inp.feat_hi[j],
                                                 u[j & 3]);
  }
  return s;
}

// quad_params(): offset of the quadratic-form parameters of `model` in a replicate.
fn quad_offset(model: i32) -> u32 {
  return select(0u, 4u, model == kFeGompertz && inp.quad_only != 0);
}

// state_log_index(): per-state log index of generation per person; pb = replicate start.
fn state_log_index(model: i32, pb: u32, y_k: f32) -> f32 {
  if (model == kFeGompertz && inp.quad_only == 0) {
    return -boot[pb + 4u] * exp(-boot[pb + 5u] * y_k);
  }
  let q = pb + quad_offset(model);
  let x = log(y_k * 0.1);
  return boot[q] * x + boot[q + 1u] * x * x;
}

// pick_model(): shared by each group of 32 consecutive draws.
fn pick_model(seed_lo: u32, seed_hi: u32, draw_lo: u32, draw_hi: u32) -> i32 {
  // DrawRng(seed ^ 0xA5A5A5A5, draw >> 5)
  let g = uniform4(DrawRng(seed_lo ^ 0xA5A5A5A5u, seed_hi, (draw_lo >> 5u) | (draw_hi << 27u), draw_hi >> 5u), 0u, 5u);
  if (g.x < inp.w_model[0]) {
    return kFeQuad;
  }
  if (g.x < inp.w_model[0] + inp.w_model[1]) {
    return kFeGompertz;
  }
  return kDamped;
}

// hist_bin() and the atomic histogram update of mc_gpu.cu.
fn emit(series: u32, t: u32, v: f32) {
  let lo = inp.hist_lo[series][t];
  let hi = inp.hist_hi[series][t];
  let pos = (v - lo) / (hi - lo) * f32(kBins);
  var bin: u32;
  if (!(pos < f32(kBins))) {
    bin = kBins - 1u;  // also NaN on the CPU
  } else if (pos < 1.0) {
    bin = 0u;
  } else {
    bin = min(u32(pos), kBins - 1u);  // min(): a guard should NaN reach this branch
  }
  atomicAdd(&hist[(series * kMaxYears + t) * kBins + bin], 1u);
}

// --- simulate_draw<float>() (mc.hpp) ----------------------------------------------

fn simulate_draw(draw: u32) {
  let seed_lo = prm.seed_lo;
  let seed_hi = prm.seed_hi;
  let rng = DrawRng(seed_lo, seed_hi, draw, 0u);  // draw < 2^32 (the host checks)

  let z = normal4(rng, 0u, 1u);    // population, urban, growth level, convergence
  let uu = uniform4(rng, 0u, 2u);  // spare, bootstrap pick, add-on, spare

  let model = pick_model(seed_lo, seed_hi, draw, 0u);
  var k = i32(uu[1] * f32(inp.n_boot));
  k = select(k, inp.n_boot - 1, k >= inp.n_boot);
  let pb = u32(k) * kBootStride;  // p = boot + k * kBootStride

  let lambda = inp.conv_mean + inp.conv_sd * z[3];
  var addon = inp.addon_lo + (inp.addon_hi - inp.addon_lo) * uu[2];
  if (inp.n_features > 0) {
    addon += feature_draw_total(rng);
  }
  let g_level = inp.g_level_sd * z[2];

  let quad = model == kFeQuad || (model == kFeGompertz && inp.quad_only != 0);
  let q = pb + quad_offset(model);
  let y0 = inp.y0;
  let x0 = log(y0 * 0.1);
  let base_quad = boot[q] * x0 + boot[q + 1u] * x0 * x0 + boot[q + 2u] * inp.u0;
  let base_gomp = -boot[pb + 4u] * exp(-boot[pb + 5u] * y0) + boot[pb + 6u] * inp.u0;

  var ln_y = log(y0);
  var shock = 0.0;
  var resid = inp.resid0;
  var dl = inp.dt_level;
  var db = inp.dt_trend;
  var gen = inp.gen0_twh;

  for (var t = 0; t < inp.n_years; t++) {
    let nz = normal4(rng, u32(t + 1), 3u);
    shock = inp.g_rho * shock + inp.g_shock_sd * nz[0];
    ln_y += inp.g_mean[t] + g_level + shock;
    let y = exp(ln_y);
    let u = clamp01(inp.urb_med[t] + inp.urb_sd[t] * z[1]);
    let pop = inp.pop_med[t] + inp.pop_sd[t] * z[0];
    let dtau = f32(t + 1) * 0.1;

    if (model == kDamped) {
      let f = dl + inp.dt_phi * db;
      let eps = inp.dt_sigma * nz[1];
      dl = f + inp.dt_alpha * eps;
      db = inp.dt_phi * db + inp.dt_alpha * inp.dt_beta * eps;
      gen = exp(f + eps);
    } else {
      var d: f32;
      if (quad) {
        let x = log(y * 0.1);
        d = boot[q] * x + boot[q + 1u] * x * x + boot[q + 2u] * u - base_quad + boot[q + 3u] * dtau;
        resid = inp.resid_rho * resid + inp.resid_innov_sd * nz[2];
        d += resid - inp.resid0;
      } else {
        d = -boot[pb + 4u] * exp(-boot[pb + 5u] * y) + boot[pb + 6u] * u - base_gomp + boot[pb + 7u] * dtau;
      }
      gen = pop * exp(inp.ln_e0 + d) * 0.001;  // millions x kWh -> TWh
    }
    gen += addon * f32(t + 1) / f32(inp.n_years);
    emit(0u, u32(t), gen);

    if (inp.n_states > 0) {
      // Regional requirement share: population x per-person index from the same income response.
      let rmodel = select(model, kFeQuad, model == kDamped);
      var reg = array<f32, 5>(0.0, 0.0, 0.0, 0.0, 0.0);
      var total = 0.0;
      let decay = exp(-lambda * (f32(t + 1) + inp.rel_income_lag));
      for (var s = 0; s < inp.n_states; s++) {
        let ys = y * exp(inp.state_ln_rel_income[s] * decay);
        let w = inp.state_pop_share[s][t] * exp(state_log_index(rmodel, pb, ys));
        reg[inp.state_region[s]] += w;
        total += w;
      }
      for (var r = 0u; r < 5u; r++) {
        emit(1u + r, u32(t), gen * reg[r] / total);
      }
    }
  }

  if (draw < prm.n_keep) {
    let o = draw * kKeepFeatures;
    keep[o + 0u] = z[0];
    keep[o + 1u] = z[1];
    keep[o + 2u] = z[2];
    keep[o + 3u] = lambda;
    keep[o + 4u] = f32(model);
    keep[o + 5u] = f32(k);
    keep[o + 6u] = addon;
    keep[o + 7u] = gen;
  }
}

@compute @workgroup_size(64)
fn mc_main(@builtin(global_invocation_id) gid: vec3<u32>) {
  if (gid.x >= prm.count) {
    return;
  }
  simulate_draw(prm.first + gid.x);
}

// --- Self-test: raw Philox outputs --------------------------------------------------
// Item i reads (counter, key) = (px_in[2i], px_in[2i + 1].xy) and writes
// philox4x32_10(counter, key) to px_out[i]; the host compares them bit for bit.

@group(0) @binding(5) var<storage, read> px_in: array<vec4<u32>>;
@group(0) @binding(6) var<storage, read_write> px_out: array<vec4<u32>>;

@compute @workgroup_size(64)
fn philox_main(@builtin(global_invocation_id) gid: vec3<u32>) {
  let i = gid.x;
  if (i >= arrayLength(&px_out)) {
    return;
  }
  let key = px_in[2u * i + 1u];
  px_out[i] = philox4x32_10(px_in[2u * i], key.x, key.y);
}
