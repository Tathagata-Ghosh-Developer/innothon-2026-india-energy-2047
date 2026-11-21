#include "price_base.hpp"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <set>
#include <stdexcept>

#include "../regional/convergence.hpp"  // normalize_state
#include "csv.hpp"

namespace e47 {

namespace {

bool all_digits(const std::string& s, size_t from, size_t n) {
  for (size_t i = from; i < from + n; ++i)
    if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
  return true;
}

// Calendar year "2024", or 0 if the label is not one.
int calendar_year(const std::string& s) { return s.size() == 4 && all_digits(s, 0, 4) ? std::stoi(s) : 0; }

std::string fy_label(int start) {
  char b[16];
  std::snprintf(b, sizeof b, "%d-%02d", start, (start + 1) % 100);
  return b;
}

// Index value of `year`, extended past the last published year at `rate` a year.
double extended(const std::map<int, double>& s, int year, double rate, const char* what, const char* rate_name,
                bool fiscal, std::string* note) {
  if (s.empty()) throw std::runtime_error(std::string("price base: no ") + what + " loaded");
  const int first = s.begin()->first, last = s.rbegin()->first;
  auto label = [&](int y) { return fiscal ? "FY" + fy_label(y) : std::to_string(y); };
  if (year < first)
    throw std::invalid_argument(std::string("price base: ") + what + " starts in " + label(first) + ", not " + label(year));
  if (year <= last) return s.at(year);
  if (note) {
    char b[512];
    std::snprintf(b, sizeof b,
                  "%s published to %s; %s extended at %s, %.1f percent a year (not an official figure).", what,
                  label(last).c_str(), label(year).c_str(), rate_name, 100.0 * rate);
    *note = b;
  }
  return s.at(last) * std::pow(1.0 + rate, year - last);
}

void require_contiguous(const std::map<int, double>& s, const char* series) {
  if (s.empty()) throw std::runtime_error(std::string("price_indices.csv: no rows for ") + series);
  int prev = s.begin()->first - 1;
  for (const auto& [y, v] : s) {
    if (y != prev + 1) throw std::runtime_error(std::string("price_indices.csv: gap in ") + series);
    if (!(v > 0)) throw std::runtime_error(std::string("price_indices.csv: non-positive value in ") + series);
    prev = y;
  }
}

}  // namespace

double real_rate(double nominal, double inflation) {
  if (!(inflation > -1.0)) throw std::invalid_argument("real_rate: inflation must be above -100 percent");
  return (1.0 + nominal) / (1.0 + inflation) - 1.0;
}

int fiscal_start_year(const std::string& fy) {
  if (fy.size() != 7 || fy[4] != '-' || !all_digits(fy, 0, 4) || !all_digits(fy, 5, 2))
    throw std::invalid_argument("not a fiscal year label (YYYY-YY): '" + fy + "'");
  const int start = std::stoi(fy.substr(0, 4));
  if (std::stoi(fy.substr(5, 2)) != (start + 1) % 100)
    throw std::invalid_argument("fiscal year label does not span one year: '" + fy + "'");
  return start;
}

PriceBase PriceBase::load(const std::string& path) {
  const CsvTable t = CsvTable::read(path);
  const size_t cs = t.col("series"), cp = t.col("period"), cv = t.col("value");
  PriceBase pb;
  std::map<int, double> target, us_goal;
  for (size_t r = 0; r < t.rows(); ++r) {
    const std::string& s = t.str(r, cs);
    const std::string& p = t.str(r, cp);
    const double v = t.num(r, cv);
    if (std::isnan(v)) continue;
    if (s == "india_gdp_deflator") pb.gdp_deflator_[fiscal_start_year(p)] = v;
    else if (s == "india_wpi_all_commodities") pb.wpi_[fiscal_start_year(p)] = v;
    else if (s == "us_gdp_implicit_price_deflator") pb.us_deflator_[calendar_year(p)] = v;
    else if (s == "rbi_inr_per_usd_calendar_average" && calendar_year(p) > 0) pb.inr_per_usd_[p] = v;
    else if (s == "rbi_inr_per_usd_fy_average") pb.inr_per_usd_[fy_label(fiscal_start_year(p))] = v;
    else if (s == "rbi_inflation_target_cpi") target[fiscal_start_year(p)] = v / 100.0;
    else if (s == "us_fomc_inflation_goal_pce" && calendar_year(p) > 0) us_goal[calendar_year(p)] = v / 100.0;
  }
  pb.us_deflator_.erase(0);
  require_contiguous(pb.gdp_deflator_, "india_gdp_deflator");
  require_contiguous(pb.wpi_, "india_wpi_all_commodities");
  require_contiguous(pb.us_deflator_, "us_gdp_implicit_price_deflator");
  if (pb.inr_per_usd_.empty()) throw std::runtime_error("price_indices.csv: no RBI rupee-dollar rates in " + path);
  if (target.empty()) throw std::runtime_error("price_indices.csv: no rbi_inflation_target_cpi rows in " + path);
  if (us_goal.empty()) throw std::runtime_error("price_indices.csv: no us_fomc_inflation_goal_pce row in " + path);
  // The latest notified (reaffirmed) value of each goal.
  pb.target_last_fy_ = target.rbegin()->first;
  pb.target_ = target.rbegin()->second;
  pb.us_goal_ = us_goal.rbegin()->second;
  return pb;
}

const std::map<int, double>& PriceBase::series(Index idx) const {
  if (idx == Index::kGdpDeflator) return gdp_deflator_;
  if (idx == Index::kWpi) return wpi_;
  throw std::invalid_argument("price base: unknown INR index");
}

int PriceBase::inr_first(Index idx) const {
  const auto& s = series(idx);
  if (s.empty()) throw std::runtime_error("price base: not loaded");
  return s.begin()->first;
}

int PriceBase::inr_last(Index idx) const {
  const auto& s = series(idx);
  if (s.empty()) throw std::runtime_error("price base: not loaded");
  return s.rbegin()->first;
}

int PriceBase::usd_last() const {
  if (us_deflator_.empty()) throw std::runtime_error("price base: not loaded");
  return us_deflator_.rbegin()->first;
}

double PriceBase::inr_index(int fy_start, Index idx, std::string* note) const {
  const char* what = idx == Index::kWpi ? "India WPI (all commodities, 2011-12 = 100)"
                                        : "India GDP deflator (MoSPI, 2011-12 series)";
  return extended(series(idx), fy_start, target_, what, "the RBI CPI target", true, note);
}

double PriceBase::deflate_inr(double value, const std::string& from_fy, const std::string& to_fy, Index idx,
                              std::string* note) const {
  std::string n_from, n_to;
  const double a = inr_index(fiscal_start_year(from_fy), idx, &n_from);
  const double b = inr_index(fiscal_start_year(to_fy), idx, &n_to);
  if (note) {
    note->clear();
    if (!n_from.empty()) *note = n_from;
    if (!n_to.empty() && n_to != n_from) *note += (note->empty() ? "" : " ") + n_to;
  }
  return value * b / a;
}

double PriceBase::usd_rebase(double value, int from_year, int to_year, std::string* note) const {
  const char* what = "US GDP implicit price deflator (BEA)";
  std::string n_from, n_to;
  const double a = extended(us_deflator_, from_year, us_goal_, what, "the FOMC longer-run goal", false, &n_from);
  const double b = extended(us_deflator_, to_year, us_goal_, what, "the FOMC longer-run goal", false, &n_to);
  if (note) {
    note->clear();
    if (!n_from.empty()) *note = n_from;
    if (!n_to.empty() && n_to != n_from) *note += (note->empty() ? "" : " ") + n_to;
  }
  return value * b / a;
}

double PriceBase::inr_per_usd(const std::string& period) const {
  auto it = inr_per_usd_.find(period);
  if (it == inr_per_usd_.end())
    throw std::invalid_argument("price base: no RBI rupee-dollar average for period '" + period + "'");
  return it->second;
}

double PriceBase::nominal_inr_from_real_usd(double usd_2024, int year) const {
  if (year < kPriceBaseYear) throw std::invalid_argument("nominal outlook: year before the price base year");
  return usd_2024 * inr_per_usd(kPriceBaseRatePeriod) * std::pow(1.0 + target_, year - kPriceBaseYear);
}

double PriceBase::nominal_usd_from_real_usd(double usd_2024, int year) const {
  if (year < kPriceBaseYear) throw std::invalid_argument("nominal outlook: year before the price base year");
  return usd_2024 * std::pow(1.0 + us_goal_, year - kPriceBaseYear);
}

GsdpPriceBasis GsdpPriceBasis::load(const std::string& path, const PriceBase& prices) {
  const CsvTable t = CsvTable::read(path);
  const size_t cs = t.col("state"), cy = t.col("from_year"), cb = t.col("basis"), cn = t.col("note");
  GsdpPriceBasis g;
  g.prices_ = prices;
  std::set<std::string> seen;
  for (size_t r = 0; r < t.rows(); ++r) {
    GsdpBasisRow row;
    row.state = trim(t.str(r, cs));
    row.key = normalize_state(row.state);
    const double y = t.num(r, cy);
    row.basis = trim(t.str(r, cb));
    row.note = t.str(r, cn);
    const std::string where = path + " row " + std::to_string(r + 2) + " (" + row.state + ")";
    if (row.key.empty()) throw std::runtime_error("GSDP price basis: no state in " + where);
    if (std::isnan(y) || y < 2011) throw std::runtime_error("GSDP price basis: from_year must be 2011 or later in " + where);
    if (row.basis != "current_prices")
      throw std::runtime_error("GSDP price basis: basis must be current_prices in " + where);
    if (!seen.insert(row.key).second) throw std::runtime_error("GSDP price basis: state listed twice in " + where);
    row.from_year = static_cast<int>(y);
    // Every listed year must have a deflator (published, or extended at the RBI target).
    (void)prices.inr_index(row.from_year);
    g.rows_.push_back(row);
  }
  return g;
}

bool GsdpPriceBasis::is_current(const std::string& state, int year) const {
  const std::string key = normalize_state(state);
  for (const GsdpBasisRow& r : rows_)
    if (r.key == key) return year >= r.from_year;
  return false;
}

double GsdpPriceBasis::constant_2011_12(const std::string& state, int year, double gsdp) const {
  if (!is_current(state, year)) return gsdp;
  const double d = prices_.inr_index(year) / prices_.inr_index(2011);  // 2011-12 = 1
  return gsdp / d;
}

}  // namespace e47
