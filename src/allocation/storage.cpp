#include "storage.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <map>
#include <numeric>
#include <stdexcept>

#include "../core/csv.hpp"
#include "../core/stats.hpp"

namespace e47 {

namespace {

namespace fs = std::filesystem;

constexpr double kKm2PerAcre = 4046.8564224e-6;  // international acre
constexpr double kKm2PerHa = 0.01;
constexpr double kGWhPerMWYear = 8.76;            // 1 MW for a year at 100% = 8.76 GWh
constexpr double kHoursPerYear = 8760.0;
constexpr double kBig = 1e12;
constexpr double kMinCyclesPerYear = 52.0;  // multi-day store: extra energy must cycle about weekly
constexpr double kMinHoursPerYear = 365.0;  // power above the largest shortfall: an hour a day at full power

// Days since 1970-01-01 of a proleptic Gregorian date (H. Hinnant's algorithm).
int days_from_civil(int y, int m, int d) {
  y -= m <= 2;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const int yoe = y - era * 400;
  const int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + doe - 719468;
}

int days_in_month(int m) {
  static const int d[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  return d[m - 1];
}

int grid_region(const std::string& name) {
  for (int r = 0; r < kRegions; ++r)
    if (name == region_name(r)) return r;
  return -1;
}

// Central value of a capacity-factor row: value, else midpoint of low and high (fraction).
// Same selection as the allocation inputs (inputs.cpp).
double cuf_of(const CsvTable& t, size_t r) {
  auto get = [&](const char* c) { return t.has(c) ? t.num(r, t.col(c)) : kNaN; };
  const double v = get("value_pct");
  if (!std::isnan(v)) return v / 100.0;
  const double lo = get("low_pct"), hi = get("high_pct");
  if (!std::isnan(lo) && !std::isnan(hi)) return 0.5 * (lo + hi) / 100.0;
  return kNaN;
}

// Verified parameter rows keyed by "technology|metric". Rows not marked verified are ignored.
std::map<std::string, double> verified_rows(const std::string& path) {
  const CsvTable t = CsvTable::read(path);
  std::map<std::string, double> m;
  for (size_t r = 0; r < t.rows(); ++r) {
    if (t.str(r, t.col("verified")) != "yes") continue;
    const double v = t.num(r, t.col("value"));
    if (!std::isnan(v)) m[t.str(r, t.col("technology")) + "|" + t.str(r, t.col("metric"))] = v;
  }
  return m;
}

double need(const std::map<std::string, double>& m, const std::string& key, const std::string& file) {
  auto it = m.find(key);
  if (it == m.end()) throw std::runtime_error("no verified row " + key + " in " + file);
  return it->second;
}

// Fleet-growth trend of one series: ln growth per year between the same calendar
// month a year apart (Sep, Oct and Nov appear in both 2025 and 2026), weighted by
// the smaller number of complete days of the pair.
double same_month_trend(const std::vector<ProfileDay>& days, const std::vector<double>& t_years, int s) {
  struct Acc {
    double e = 0, t = 0;
    int n = 0;
  };
  std::map<int, std::map<int, Acc>> by;  // month -> year -> accumulators
  for (size_t d = 0; d < days.size(); ++d) {
    double e = 0;
    for (double v : days[d].mw[s]) e += v;
    Acc& a = by[days[d].month][std::stoi(days[d].date.substr(0, 4))];
    a.e += e;
    a.t += t_years[d];
    ++a.n;
  }
  double num = 0, den = 0;
  for (const auto& [m, years] : by) {
    if (years.size() < 2) continue;
    const Acc& a = years.begin()->second;
    const Acc& b = years.rbegin()->second;
    if (a.e <= 0 || b.e <= 0) continue;
    const double n = std::min(a.n, b.n);
    num += n * std::log((b.e / b.n) / (a.e / a.n));
    den += n * (b.t / b.n - a.t / a.n);
  }
  return den > 0 ? num / den : 0.0;
}

// x[i] = clamp(R[i] - lambda, lo, hi) over [b, e) with sum w[i] x[i] = target:
// the source follows the residual as closely as its bounds and its energy allow,
// leaving a flat remainder (lambda) wherever it is not at a bound.
void waterfill(const std::vector<double>& R, const std::vector<double>& w, size_t b, size_t e, double lo, double hi,
               double target, std::vector<double>& x) {
  auto sum_at = [&](double lam) {
    double s = 0;
    for (size_t i = b; i < e; ++i) s += w[i] * std::clamp(R[i] - lam, lo, hi);
    return s;
  };
  double rmin = kBig, rmax = -kBig;
  for (size_t i = b; i < e; ++i) {
    rmin = std::min(rmin, R[i]);
    rmax = std::max(rmax, R[i]);
  }
  double a = rmin - hi - 1.0, c = rmax - lo + 1.0;  // sum_at(a) = max, sum_at(c) = min
  if (target >= sum_at(a)) {
    c = a;
  } else if (target <= sum_at(c)) {
    a = c;
  } else {
    for (int it = 0; it < 100; ++it) {
      const double mid = 0.5 * (a + c);
      if (sum_at(mid) > target)
        a = mid;
      else
        c = mid;
    }
  }
  const double lam = 0.5 * (a + c);
  for (size_t i = b; i < e; ++i) x[i] = std::clamp(R[i] - lam, lo, hi);
}

struct SimStats {
  double unmet = 0, curtailed = 0, charged = 0, discharged = 0;
};

// One pass of a greedy store over hours [0, n) from state of charge soc: charge
// from surplus (r > 0), discharge into shortfall (r < 0). Returns the final state;
// st (may be null) receives the statistics, weighted by w (per hour, may be null).
double pass(const double* r, const double* w, int n, double pc, double pd, double cap, double ec, double ed, double soc,
            SimStats* st) {
  for (int i = 0; i < n; ++i) {
    const double wi = w ? w[i] : 1.0;
    if (r[i] > 0) {
      const double c = std::max(0.0, std::min({r[i], pc, (cap - soc) / ec}));
      soc += c * ec;
      if (st) {
        st->charged += wi * c;
        st->curtailed += wi * (r[i] - c);
      }
    } else if (r[i] < 0) {
      const double d = std::max(0.0, std::min({-r[i], pd, soc * ed}));
      soc -= d / ed;
      if (st) {
        st->discharged += wi * d;
        st->unmet += wi * (-r[i] - d);
      }
    }
  }
  return soc;
}

// Greedy store in its cyclic steady state: the pass starts and ends at the same
// state of charge. For one store and a shortfall-energy objective, greedy use is
// optimal: stored energy covers the same shortfall whenever it is used.
SimStats simulate(const double* r, const double* w, int n, double pc, double pd, double cap, double ec, double ed) {
  const double tol = 1e-9 * std::max(1.0, cap);
  double s = 0;
  bool steady = false;
  for (int lap = 0; lap < 3 && !steady; ++lap) {
    const double end = pass(r, w, n, pc, pd, cap, ec, ed, s, nullptr);
    steady = std::abs(end - s) <= tol;
    if (!steady) s = end;
  }
  if (!steady) {
    // The end state rises with the start state at a slope of at most 1, so
    // end(s0) - s0 falls with s0: bisect for the fixed point in [0, cap].
    double lo = 0, hi = cap;
    for (int it = 0; it < 60; ++it) {
      const double mid = 0.5 * (lo + hi);
      if (pass(r, w, n, pc, pd, cap, ec, ed, mid, nullptr) > mid)
        lo = mid;
      else
        hi = mid;
    }
    s = 0.5 * (lo + hi);
  }
  SimStats st;
  pass(r, w, n, pc, pd, cap, ec, ed, s, &st);
  return st;
}

// Smallest x in [lo, hi] with f(x) <= goal, f non-increasing.
template <class F>
double smallest(F f, double lo, double hi, double goal) {
  if (f(lo) <= goal) return lo;
  for (int it = 0; it < 60; ++it) {
    const double mid = 0.5 * (lo + hi);
    if (f(mid) <= goal)
      hi = mid;
    else
      lo = mid;
  }
  return hi;
}

// Hourly residual (GW; positive = surplus) of a plan after firm dispatch.
struct Residual {
  std::vector<double> r, w;  // per hour of the representative year, chronological
  double vre_twh = 0;
  double gas_hours = 0, gas_twh = 0, gas_peak_gw = 0, gas_evening_twh = 0;  // StorageSizing
};

Residual dispatch(const StorageInputs& in, double load_twh, const std::array<double, kTechs>& twh, bool firm_flat,
                  double firm_solar, double firm_wind) {
  const size_t nd = in.days.size(), n = nd * kDayHours;
  Residual out;
  out.r.assign(n, 0.0);
  out.w.assign(n, 0.0);
  std::vector<double> R(n), hydro(n, 0.0), coal(n, 0.0), gas(n, 0.0), hlo(n), hhi(n);
  const double solar = (twh[kSolar] + firm_solar) * 1e3, wind = (twh[kWind] + twh[kOffshore] + firm_wind) * 1e3;
  const double flat = (twh[kNuclear] + twh[kBio] + (firm_flat ? twh[kFirmRE] : 0.0)) * 1e3 / kHoursPerYear;
  for (size_t d = 0; d < nd; ++d) {
    const ProfileDay& day = in.days[d];
    for (int h = 0; h < kDayHours; ++h) {
      const size_t i = d * kDayHours + h;
      out.w[i] = day.weight;
      const double load = load_twh * 1e3 * day.mw[kPLoad][h] / in.annual_mwh[kPLoad];
      const double vre = solar * day.mw[kPSolar][h] / in.annual_mwh[kPSolar] + wind * day.mw[kPWind][h] / in.annual_mwh[kPWind];
      out.vre_twh += day.weight * vre / 1e3;
      R[i] = load - vre - flat;
      hydro[i] = twh[kHydro] * 1e3 * day.mw[kPHydro][h] / in.annual_mwh[kPHydro];
    }
  }
  // Hydro: each day keeps its observed energy (water availability) and moves it
  // within that day's observed hourly range, towards the hours of highest residual.
  for (size_t d = 0; d < nd; ++d) {
    const size_t b = d * kDayHours, e = b + kDayHours;
    double lo = kBig, hi = 0, budget = 0;
    for (size_t i = b; i < e; ++i) {
      lo = std::min(lo, hydro[i]);
      hi = std::max(hi, hydro[i]);
      budget += out.w[i] * hydro[i];
    }
    if (hi > 0) waterfill(R, out.w, b, e, lo, hi, budget, hydro);
  }
  for (size_t i = 0; i < n; ++i) R[i] -= hydro[i];
  // Coal: annual energy of the plan; hourly output between the regulated minimum
  // level and the fleet's highest hourly output relative to its mean (Merit).
  if (twh[kCoal] > 0) {
    const double mean = twh[kCoal] * 1e3 / kHoursPerYear;
    const double hi = std::max(1.0, in.coal_peak_ratio) * mean;
    const double lo = std::min(mean, in.coal_min_level * hi);
    waterfill(R, out.w, 0, n, lo, hi, twh[kCoal] * 1e3, coal);
    for (size_t i = 0; i < n; ++i) R[i] -= coal[i];
  }
  // Gas: annual energy of the plan, peaking (fills the highest residual hours).
  if (twh[kGas] > 0) {
    const double mean = twh[kGas] * 1e3 / kHoursPerYear;
    waterfill(R, out.w, 0, n, 0.0, std::max(1.0, in.gas_peak_ratio) * mean, twh[kGas] * 1e3, gas);
    for (size_t i = 0; i < n; ++i) {
      R[i] -= gas[i];
      if (gas[i] > 1e-3) out.gas_hours += out.w[i];  // above 1 MW
      out.gas_twh += out.w[i] * gas[i] / 1e3;
      out.gas_peak_gw = std::max(out.gas_peak_gw, gas[i]);
      if (static_cast<int>(i % kDayHours) >= kEveningFromHour) out.gas_evening_twh += out.w[i] * gas[i] / 1e3;
    }
  }
  for (size_t i = 0; i < n; ++i) out.r[i] = -R[i];
  return out;
}

StorageSizing size_storage(const StorageInputs& in, const Residual& res) {
  StorageSizing s;
  const int nd = static_cast<int>(in.days.size());
  const int n = nd * kDayHours;
  const double eta = in.round_trip, ec = std::sqrt(eta), ed = std::sqrt(eta);
  const double* r = res.r.data();
  s.vre_twh = res.vre_twh;
  s.gas_hours = res.gas_hours;
  s.gas_twh = res.gas_twh;
  s.gas_peak_gw = res.gas_peak_gw;
  s.gas_evening_twh = res.gas_evening_twh;
  for (int i = 0; i < n; ++i) {
    const double wi = res.w[i];
    if (r[i] < 0) {
      s.deficit_twh += wi * -r[i] / 1e3;
      s.discharge_gw = std::max(s.discharge_gw, -r[i]);
      if (r[i] < -1e-3) s.deficit_hours += wi;  // shortfall above 1 MW
    } else {
      s.surplus_twh += wi * r[i] / 1e3;
    }
  }
  const double pd = s.discharge_gw;
  if (pd <= 0) return s;  // no shortfall in any hour: no storage needed

  // Daily cycle. Each complete day is a cyclic 24-hour sequence (the evening and
  // night deficit is covered by the same day's surplus).
  std::vector<double> e_day(nd, 0.0), cap_day(nd, 0.0), u_day(nd, 0.0);
  double smax = 0;
  for (int d = 0; d < nd; ++d) {
    double surplus = 0;
    for (int h = 0; h < kDayHours; ++h) {
      surplus += std::max(0.0, r[d * kDayHours + h]);
      smax = std::max(smax, r[d * kDayHours + h]);
    }
    cap_day[d] = surplus * ec + 1.0;  // more than the day could ever store
  }
  // Power rating (charge and discharge): at least the largest hourly shortfall;
  // above it, charging power is added while each extra GW still serves at least
  // kMinHoursPerYear GWh of shortfall a year, with each day's energy unlimited.
  auto unmet_at = [&](double pc) {
#pragma omp parallel for schedule(static)
    for (int d = 0; d < nd; ++d)
      u_day[d] = in.days[d].weight *
                 simulate(r + static_cast<size_t>(d) * kDayHours, nullptr, kDayHours, pc, pd, cap_day[d], ec, ed).unmet;
    return std::accumulate(u_day.begin(), u_day.end(), 0.0);  // serial sum: deterministic
  };
  const double pstep = std::max(1.0, 0.01 * pd);
  auto served_per_gw = [&](double pc) { return (unmet_at(pc) - unmet_at(pc + pstep)) / pstep; };
  double plo = pd, phi = std::max(pd, smax);
  if (served_per_gw(plo) < kMinHoursPerYear) {
    phi = plo;
  } else {
    for (int it = 0; it < 40 && phi - plo > pstep; ++it) {
      const double mid = 0.5 * (plo + phi);
      if (served_per_gw(mid) >= kMinHoursPerYear)
        plo = mid;
      else
        phi = mid;
    }
  }
  s.power_gw = plo;
  const double pc = s.power_gw;
  // Energy: for each day the smallest store that leaves no more shortfall than an unlimited one.
#pragma omp parallel for schedule(static)
  for (int d = 0; d < nd; ++d) {
    const double* rd = r + static_cast<size_t>(d) * kDayHours;
    const double u_none = simulate(rd, nullptr, kDayHours, pc, pd, 0.0, ec, ed).unmet;
    const double u_best = simulate(rd, nullptr, kDayHours, pc, pd, cap_day[d], ec, ed).unmet;
    const double tol = 1e-6 * (u_none - u_best) + 1e-4;
    e_day[d] = smallest([&](double cap) { return simulate(rd, nullptr, kDayHours, pc, pd, cap, ec, ed).unmet; }, 0.0,
                        cap_day[d], u_best + tol);
  }
  const auto worst = std::max_element(e_day.begin(), e_day.end());
  s.energy_day_gwh = *worst;
  s.design_day = in.days[worst - e_day.begin()].date;
  s.energy_day_median_gwh = quantile(e_day, 0.5);
  s.duration_h = s.energy_day_gwh / s.power_gw;
  // Annual operation with the design-day store, every day cycled on its own.
  for (int d = 0; d < nd; ++d) {
    const SimStats st =
        simulate(r + static_cast<size_t>(d) * kDayHours, nullptr, kDayHours, pc, pd, s.energy_day_gwh, ec, ed);
    const double w = in.days[d].weight / 1e3;
    s.unmet_day_twh += w * st.unmet;
    s.curtailed_day_twh += w * st.curtailed;
    s.throughput_twh += w * st.discharged;
    s.losses_twh += w * (st.charged - st.discharged);
    s.unmet_day_free_charge_twh +=
        w * simulate(r + static_cast<size_t>(d) * kDayHours, nullptr, kDayHours, kBig, pd, s.energy_day_gwh, ec, ed).unmet;
  }
  s.cycles = s.energy_day_gwh > 0 ? s.throughput_twh * 1e3 / s.energy_day_gwh : 0.0;

  // Carry-over between days: the complete days in calendar order as one cyclic
  // record, with the same charge and discharge power. The store grows beyond the
  // daily size while each extra GWh of energy still serves at least
  // kMinCyclesPerYear GWh of shortfall a year (about one full cycle a week);
  // energy beyond that would be seasonal storage.
  const double* w = res.w.data();
  double cap_all = 1.0;
  for (int i = 0; i < n; ++i)
    if (r[i] > 0) cap_all += r[i] * ec;
  cap_all = std::max(cap_all, 2.0 * s.energy_day_gwh);
  auto chrono = [&](double cap) { return simulate(r, w, n, pc, pd, cap, ec, ed); };
  const double step = std::max(1.0, 0.02 * s.energy_day_gwh);
  auto served_per_gwh = [&](double cap) { return (chrono(cap).unmet - chrono(cap + step).unmet) / step; };
  double lo = s.energy_day_gwh, hi = cap_all;
  if (served_per_gwh(lo) < kMinCyclesPerYear) {
    hi = lo;
  } else {
    for (int it = 0; it < 40 && hi - lo > step; ++it) {
      const double mid = 0.5 * (lo + hi);
      if (served_per_gwh(mid) >= kMinCyclesPerYear)
        lo = mid;
      else
        hi = mid;
    }
  }
  s.energy_multi_gwh = lo;
  const SimStats m = chrono(s.energy_multi_gwh);
  s.unmet_multi_twh = m.unmet / 1e3;
  s.curtailed_multi_twh = m.curtailed / 1e3;
  for (size_t k = 0; k < s.curve_unmet_twh.size(); ++k)
    s.curve_unmet_twh[k] = chrono(StorageSizing::kCurveMult[k] * s.energy_day_gwh).unmet / 1e3;
  return s;
}

}  // namespace

double LandUse::solar_total() const { return std::accumulate(solar_km2.begin(), solar_km2.end(), 0.0); }
double LandUse::wind_total_area() const { return std::accumulate(wind_total_km2.begin(), wind_total_km2.end(), 0.0); }
double LandUse::total_km2() const {
  return solar_total() + std::accumulate(wind_direct_km2.begin(), wind_direct_km2.end(), 0.0);
}

StorageInputs load_storage_inputs(const std::string& root) {
  StorageInputs in;
  try {
    const fs::path r(root);
    // 1. Merit hourly generation by source; complete days only.
    {
      const CsvTable t = CsvTable::read((r / "data/raw/npp/npp_merit_generation_hourly.csv").string());
      const size_t c_dt = t.col("datetime_ist"), c_src = t.col("source"), c_mw = t.col("mean_mw"),
                   c_ok = t.col("is_complete"), c_sub = t.col("is_subtotal");
      const std::map<std::string, int> src = {{"thermal", 0}, {"gas", 1}, {"hydro", 2},
                                              {"nuclear", 3}, {"solar", 4}, {"wind", 5}};
      struct RawDay {
        std::array<std::array<double, kDayHours>, 6> mw{};
        int filled = 0;
      };
      std::map<std::string, RawDay> raw;  // ISO dates sort chronologically
      for (size_t i = 0; i < t.rows(); ++i) {
        if (t.str(i, c_sub) == "True" || t.str(i, c_ok) != "True") continue;
        auto it = src.find(t.str(i, c_src));
        const std::string& dt = t.str(i, c_dt);
        const double v = t.num(i, c_mw);
        if (it == src.end() || dt.size() < 13 || std::isnan(v)) continue;
        RawDay& day = raw[dt.substr(0, 10)];
        day.mw[it->second][std::stoi(dt.substr(11, 2))] = v;
        ++day.filled;
      }
      std::vector<double> t_years;
      for (const auto& [date, day] : raw) {
        if (day.filled != 6 * kDayHours) continue;
        ProfileDay p;
        p.date = date;
        p.month = std::stoi(date.substr(5, 2));
        for (int h = 0; h < kDayHours; ++h) {
          double total = 0;
          for (int k = 0; k < 6; ++k) total += day.mw[k][h];
          p.mw[kPLoad][h] = total;  // total generation, the load shape
          p.mw[kPSolar][h] = std::max(0.0, day.mw[4][h]);  // small negative night readings set to 0
          p.mw[kPWind][h] = std::max(0.0, day.mw[5][h]);
          p.mw[kPHydro][h] = std::max(0.0, day.mw[2][h]);
          p.mw[kPCoal][h] = day.mw[0][h];
          p.mw[kPGas][h] = std::max(0.0, day.mw[1][h]);
        }
        in.days.push_back(p);
        t_years.push_back(days_from_civil(std::stoi(date.substr(0, 4)), p.month, std::stoi(date.substr(8, 2))) / 365.25);
      }
      if (in.days.size() < 300) throw std::runtime_error("too few complete days in the Merit hourly series");
      // Trends: fleet growth is removed from the load, solar, coal and gas shapes;
      // wind and hydro differences between the two years are weather and are kept.
      const double t_mid = mean(t_years);
      for (int s = 0; s < kProfileSeries; ++s) {
        in.trend[s] = same_month_trend(in.days, t_years, s);
        in.detrended[s] = s == kPLoad || s == kPSolar || s == kPCoal || s == kPGas;
      }
      for (size_t d = 0; d < in.days.size(); ++d)
        for (int s = 0; s < kProfileSeries; ++s) {
          if (!in.detrended[s]) continue;
          const double f = std::exp(-in.trend[s] * (t_years[d] - t_mid));
          for (double& v : in.days[d].mw[s]) v *= f;
        }
      // Month weights: each calendar month counts its days in a 365-day year.
      std::array<int, 13> count{};
      for (const auto& p : in.days) ++count[p.month];
      for (int m = 1; m <= 12; ++m)
        if (count[m] == 0) throw std::runtime_error("no complete Merit day in month " + std::to_string(m));
      for (auto& p : in.days) p.weight = static_cast<double>(days_in_month(p.month)) / count[p.month];
      double peak_coal = 0, peak_gas = 0;
      for (const auto& p : in.days)
        for (int s = 0; s < kProfileSeries; ++s)
          for (int h = 0; h < kDayHours; ++h) {
            in.annual_mwh[s] += p.weight * p.mw[s][h];
            if (s == kPCoal) peak_coal = std::max(peak_coal, p.mw[s][h]);
            if (s == kPGas) peak_gas = std::max(peak_gas, p.mw[s][h]);
          }
      for (int s = 0; s < kProfileSeries; ++s)
        if (in.annual_mwh[s] <= 0) throw std::runtime_error("empty Merit series");
      in.coal_peak_ratio = peak_coal / (in.annual_mwh[kPCoal] / kHoursPerYear);
      in.gas_peak_ratio = peak_gas / (in.annual_mwh[kPGas] / kHoursPerYear);
      in.notes.push_back("Hourly shapes: NPP Merit all-India generation by source, " + std::to_string(in.days.size()) +
                         " complete days " + in.days.front().date + " to " + in.days.back().date +
                         ", each month weighted to its days in a 365-day year.");
    }
    // 2. Storage parameters and the comparison benchmark.
    {
      const std::string f = (r / "data/external/techno_economic/storage_parameters.csv").string();
      const auto m = verified_rows(f);
      in.round_trip = need(m, "battery|round_trip_efficiency", f) / 100.0;
      in.coal_min_level = need(m, "coal|minimum_power_level", f) / 100.0;
      in.bench_power_gw = need(m, "benchmark_cea_2029_30|storage_power", f);
      in.bench_energy_gwh = need(m, "benchmark_cea_2029_30|storage_energy", f);
      in.bench_generation_twh = need(m, "benchmark_cea_2029_30|gross_generation", f);
      in.bench_vre_share =
          (need(m, "benchmark_cea_2029_30|solar_share", f) + need(m, "benchmark_cea_2029_30|wind_share", f)) / 100.0;
    }
    // 3. Land-use intensities.
    {
      const std::string f = (r / "data/external/techno_economic/land_use.csv").string();
      const auto m = verified_rows(f);
      in.solar_acres_per_mw = need(m, "solar_pv|land_per_mw_india", f);
      in.solar_acres_per_gwh = need(m, "solar_pv|total_area_per_gwh_large_pv", f);
      in.wind_total_ha_per_mw = need(m, "wind_onshore|total_area_per_mw", f);
      in.wind_direct_ha_per_mw = need(m, "wind_onshore|direct_permanent_per_mw", f);
    }
    // 4. Regional solar and onshore wind CUF (CEA NEP planning values), as in the allocation inputs.
    {
      const CsvTable t = CsvTable::read((r / "data/external/techno_economic/capacity_factors.csv").string());
      for (size_t i = 0; i < t.rows(); ++i) {
        if (t.str(i, t.col("geography_level")) != "region") continue;
        const int reg = grid_region(t.str(i, t.col("region")));
        const std::string tech = t.str(i, t.col("source_tech")), metric = t.str(i, t.col("metric"));
        const double v = cuf_of(t, i);
        if (reg < 0 || std::isnan(v)) continue;
        if (tech == "solar" && metric == "planning_cuf_new") in.cuf_solar[reg] = v;
        if (tech == "wind_onshore" && metric == "planning_cuf") in.cuf_wind[reg] = v;
      }
      for (auto* a : {&in.cuf_solar, &in.cuf_wind}) {
        double mn = 2;
        for (double v : *a)
          if (v > 0) mn = std::min(mn, v);
        if (mn > 1) throw std::runtime_error("no regional capacity factor for the land conversion");
        for (double& v : *a)
          if (v <= 0) v = mn;
      }
    }
    // 5. Areas: grid regions (sum of states and union territories) and India (ISFR 2023).
    {
      const CsvTable t = CsvTable::read((r / "data/external/state_region_map.csv").string());
      for (size_t i = 0; i < t.rows(); ++i) {
        const std::string type = t.str(i, t.col("row_type"));
        const double a = t.num(i, t.col("area_km2"));
        if (std::isnan(a)) continue;
        if (type == "national_total") in.india_area_km2 = a;
        if (type != "state" && type != "union_territory") continue;
        const int reg = grid_region(t.str(i, t.col("grid_region")));
        if (reg >= 0) in.region_area_km2[reg] += a;
      }
      if (in.india_area_km2 <= 0) throw std::runtime_error("no India area in state_region_map.csv");
    }
    {
      const CsvTable t = CsvTable::read((r / "data/external/climate_land.csv").string());
      for (size_t i = 0; i < t.rows(); ++i)
        if (t.str(i, t.col("state")) == "India" && t.str(i, t.col("variable")) == "wasteland_area_km2" &&
            t.str(i, t.col("year")) == "2015-16")
          in.wasteland_km2 = t.num(i, t.col("value"));
    }
    in.notes.push_back("Storage round-trip efficiency " + std::to_string(static_cast<int>(std::lround(100 * in.round_trip))) +
                       "% (NREL ATB 2025); coal fleet minimum " +
                       std::to_string(static_cast<int>(std::lround(100 * in.coal_min_level))) +
                       "% of its highest hourly output (CEA flexible operation regulations 2023).");
    in.notes.push_back("Land: solar 4 acres/MW (MNRE) converted with the regional CUF, high case 3.4 acres per GWh/yr "
                       "(NREL); wind 0.3 ha/MW footprint and 34.5 ha/MW project area (NREL).");
    in.ok = true;
    in.status = "ok";
  } catch (const std::exception& e) {
    in = StorageInputs{};
    in.status = std::string("storage inputs not loaded: ") + e.what();
  }
  return in;
}

StorageLandResult storage_and_land(const StorageInputs& in, const AllocationInputs& alloc, const PolicySettings& p,
                                   const AllocationResult& plan) {
  StorageLandResult res;
  if (!in.ok) {
    res.status = in.status;
    return res;
  }
  if (!plan.ok) {
    res.status = "no plan: " + plan.status;
    return res;
  }
  Timer timer;
  double req = 0, unserved = 0;
  for (int r = 0; r < kRegions; ++r) {
    req += plan.requirement[r];
    unserved += plan.unserved[r];
  }
  // Storage cannot supply energy the plan lacks over the year: the hourly load is
  // the requirement the plan meets, shaped like today's total generation.
  res.load_twh = req - unserved;
  // Firmed RE back to the solar and onshore wind it is built from. The LP draws
  // it from each region's shared solar plus wind resource, so it is split in
  // proportion to the usable solar and wind potential the plan leaves unused in
  // that region (same bounds as the LP); this keeps both within their limits.
  const Bounds b = make_bounds(alloc, p, plan.requirement);
  std::array<double, kRegions> fs{}, fw{};
  for (int r = 0; r < kRegions; ++r) {
    const double s = plan.gen[r][kSolar], w = plan.gen[r][kWind], f = plan.gen[r][kFirmRE];
    const double hs = std::max(0.0, b.hi[r][kSolar] - s), hw = std::max(0.0, b.hi[r][kWind] - w);
    const double share = hs + hw > 0 ? hs / (hs + hw) : (s + w > 0 ? s / (s + w) : 1.0);
    fs[r] = f * share;
    fw[r] = f * (1.0 - share);
    res.firm_re_solar_twh += fs[r];
    res.firm_re_wind_twh += fw[r];
  }
  res.system = size_storage(in, dispatch(in, res.load_twh, plan.national, true, 0.0, 0.0));
  res.total = size_storage(in, dispatch(in, res.load_twh, plan.national, false, res.firm_re_solar_twh, res.firm_re_wind_twh));

  // Land: generation (GWh/yr) x energy-based intensity (km2 per GWh/yr).
  LandUse& L = res.land;
  double potential = 0;
  for (int r = 0; r < kRegions; ++r) {
    const double solar_gwh = (plan.gen[r][kSolar] + fs[r]) * 1e3;
    const double wind_gwh = (plan.gen[r][kWind] + fw[r]) * 1e3;
    L.solar_km2[r] = solar_gwh * in.solar_acres_per_mw * kKm2PerAcre / (in.cuf_solar[r] * kGWhPerMWYear);
    L.solar_km2_high += solar_gwh * in.solar_acres_per_gwh * kKm2PerAcre;
    L.wind_direct_km2[r] = wind_gwh * in.wind_direct_ha_per_mw * kKm2PerHa / (in.cuf_wind[r] * kGWhPerMWYear);
    L.wind_total_km2[r] = wind_gwh * in.wind_total_ha_per_mw * kKm2PerHa / (in.cuf_wind[r] * kGWhPerMWYear);
    L.solar_twh += solar_gwh / 1e3;
    L.wind_twh += wind_gwh / 1e3;
    res.pct_region[r] = in.region_area_km2[r] > 0
                            ? 100.0 * (L.solar_km2[r] + L.wind_direct_km2[r]) / in.region_area_km2[r]
                            : 0.0;
    potential += potential_twh(alloc, p, r, kSolar);  // the policy's solar potential set
  }
  res.pct_india = 100.0 * L.total_km2() / in.india_area_km2;
  res.pct_wasteland = in.wasteland_km2 > 0 ? 100.0 * L.total_km2() / in.wasteland_km2 : 0.0;
  res.solar_share_of_potential = potential > 0 ? L.solar_twh / potential : 0.0;
  res.solar_share_of_usable = potential > 0 && p.solar_land > 0 ? L.solar_twh / (p.solar_land * potential) : 0.0;
  res.ms = timer.ms();
  res.ok = true;
  res.status = "ok";
  return res;
}

}  // namespace e47
