#pragma once
// One real price base for every money figure: real 2024 USD, the price year of IEA WEO
// 2025 (design: docs/plans/2026-11-21-inflation-features-design.md, section A).
//
// Price indices, exchange rates and inflation goals come from
// data/external/macro/price_indices.csv (all official, all secondary; sources in
// data/external/macro/SOURCES.md):
//   India GDP deflator, 2011-12 series (MoSPI National Accounts), fiscal years, 2011-12 = 100
//   India WPI, all commodities, 2011-12 = 100 (Office of the Economic Adviser), fiscal years
//   US GDP implicit price deflator (BEA), calendar years, 2017 = 100
//   RBI rupee-dollar reference rate, calendar-year and fiscal-year averages
//   RBI CPI inflation target (section 45ZA of the RBI Act) and the FOMC longer-run goal.
//
// A cost in INR of fiscal year f becomes real 2024 USD in two steps: deflate it within INR to
// FY2024-25 (the fiscal-year proxy for 2024; MoSPI publishes no calendar-year deflator), then
// convert at the calendar-2024 RBI average (Rs 83.6693 per USD). A USD figure of year y is
// rebased with the US GDP deflator.
//
// Past the last published year an index is extended at the latest notified inflation goal
// (India: the RBI CPI target, 4 percent a year; US: the FOMC goal, 2 percent a year). Such a
// value is not an official figure, so every call that uses one can return a note saying so.
// Exchange rates are never extended.
#include <map>
#include <string>
#include <vector>

namespace e47 {

// The price base: real USD of kPriceBaseYear. A rupee cost is deflated to kPriceBaseFy (the
// fiscal-year proxy of that year) and converted at the RBI average of kPriceBaseYear.
constexpr int kPriceBaseYear = 2024;
inline constexpr char kPriceBaseFy[] = "2024-25";
inline constexpr char kPriceBaseRatePeriod[] = "2024";

// Fisher relation: the real rate of a nominal rate at an inflation rate, all fractions.
//   real = (1 + nominal) / (1 + inflation) - 1.  Throws if inflation <= -1.
double real_rate(double nominal, double inflation);

// Fiscal start year of a label "2021-22" (2021). Throws unless the label is YYYY-YY with
// the second part one year after the first.
int fiscal_start_year(const std::string& fy);

class PriceBase {
 public:
  enum class Index : int { kGdpDeflator, kWpi };  // India deflators for INR

  PriceBase() = default;  // empty: every lookup throws
  // Reads price_indices.csv. Throws if a series the price base needs is missing.
  static PriceBase load(const std::string& price_indices_csv);

  // INR of fiscal year from_fy ("2021-22") in INR of fiscal year to_fy, with the India GDP
  // deflator (default) or the WPI. A year past the published series is extended at the RBI
  // CPI target; *note (if given) is then set to a sentence that says so, else left empty.
  // Throws on a malformed label or a year before the series starts.
  double deflate_inr(double value, const std::string& from_fy, const std::string& to_fy,
                     Index idx = Index::kGdpDeflator, std::string* note = nullptr) const;
  // Index level of a fiscal year (start year), extended as in deflate_inr.
  double inr_index(int fy_start, Index idx = Index::kGdpDeflator, std::string* note = nullptr) const;

  // USD of calendar year from_year in USD of to_year, with the US GDP implicit price
  // deflator. Past the published series: the FOMC goal, noted as in deflate_inr.
  double usd_rebase(double value, int from_year, int to_year, std::string* note = nullptr) const;

  // RBI reference rate, Rs per USD: period "2024" is a calendar-year average, "2024-25" a
  // fiscal-year average. Throws if the period has no published rate.
  double inr_per_usd(const std::string& period) const;
  double inr_to_usd(double inr, const std::string& period) const { return inr / inr_per_usd(period); }

  // RBI CPI inflation target of the latest notified fiscal year, fraction (0.04), and that year.
  double inflation_target() const { return target_; }
  int inflation_target_last_fy() const { return target_last_fy_; }
  // FOMC longer-run inflation goal (PCE price index), fraction (0.02).
  double us_inflation_goal() const { return us_goal_; }

  // First and last published year of an India index (fiscal start years) and of the US deflator.
  int inr_first(Index idx = Index::kGdpDeflator) const;
  int inr_last(Index idx = Index::kGdpDeflator) const;
  int usd_last() const;

 private:
  const std::map<int, double>& series(Index idx) const;
  std::map<int, double> gdp_deflator_, wpi_;  // fiscal start year -> index
  std::map<int, double> us_deflator_;         // calendar year -> index
  std::map<std::string, double> inr_per_usd_; // period label -> Rs per USD
  double target_ = 0, us_goal_ = 0;
  int target_last_fy_ = 0;
};

}  // namespace e47
