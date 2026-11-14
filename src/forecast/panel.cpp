#include "panel.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "../core/csv.hpp"

namespace e47 {

namespace {

using YearMap = std::map<int, double>;
using IsoYearMap = std::unordered_map<std::string, YearMap>;

// World Bank WDI bulk CSV: 4 preamble lines, then a wide table with one column per year.
IsoYearMap read_wdi_wide(const std::string& path) {
  const CsvTable t = CsvTable::read(path, ',', 4);
  const size_t c_iso = t.col("Country Code");
  IsoYearMap out;
  for (size_t c = 0; c < t.cols(); ++c) {
    const std::string& h = t.header()[c];
    const bool is_year = h.size() == 4 && std::all_of(h.begin(), h.end(), [](unsigned char ch) {
                           return std::isdigit(ch) != 0;
                         });
    if (!is_year) continue;
    const int year = std::stoi(h);
    for (size_t r = 0; r < t.rows(); ++r) {
      const double v = t.num(r, c);
      if (!std::isnan(v)) out[t.str(r, c_iso)][year] = v;
    }
  }
  return out;
}

}  // namespace

const CountrySeries* Panel::find(const std::string& iso3) const {
  for (const auto& c : countries)
    if (c.iso3 == iso3) return &c;
  return nullptr;
}

size_t Panel::n_obs() const {
  size_t n = 0;
  for (const auto& c : countries) n += c.obs.size();
  return n;
}

Panel load_panel(const PanelSources& src, const PanelFilter& f) {
  // Generation: Ember long format, one row per area, year and source.
  const CsvTable em = CsvTable::read(src.ember_generation_csv);
  const size_t c_area = em.col("Area"), c_iso = em.col("ISO 3 code"), c_year = em.col("Year");
  const size_t c_type = em.col("Area type"), c_src = em.col("Electricity source");
  const size_t c_gen = em.col("Generation (TWh)");

  IsoYearMap gen;
  std::unordered_map<std::string, std::string> names;
  for (size_t r = 0; r < em.rows(); ++r) {
    if (em.str(r, c_type) != "Country or economy") continue;
    if (em.str(r, c_src) != "Total generation") continue;
    const double v = em.num(r, c_gen);
    const double y = em.num(r, c_year);
    const std::string& iso = em.str(r, c_iso);
    if (iso.size() != 3 || std::isnan(v) || std::isnan(y) || v <= 0) continue;
    gen[iso][static_cast<int>(y)] = v;
    names[iso] = em.str(r, c_area);
  }

  const IsoYearMap gdp = read_wdi_wide(src.wb_gdp_pc_csv);
  const IsoYearMap urb = read_wdi_wide(src.wb_urban_csv);
  const IsoYearMap pop = read_wdi_wide(src.wb_population_csv);

  Panel p;
  for (const auto& [iso, gy] : gen) {
    auto ig = gdp.find(iso), iu = urb.find(iso), ip = pop.find(iso);
    if (ig == gdp.end() || iu == urb.end() || ip == pop.end()) continue;
    CountrySeries cs{iso, names[iso], {}};
    for (const auto& [year, g] : gy) {
      if (year < f.first_year || year > f.last_year) continue;
      auto a = ig->second.find(year), b = iu->second.find(year), c = ip->second.find(year);
      if (a == ig->second.end() || b == iu->second.end() || c == ip->second.end()) continue;
      if (a->second <= 0 || c->second <= 0) continue;
      cs.obs.push_back(Obs{year, g, c->second, a->second, b->second / 100.0});
    }
    if (static_cast<int>(cs.obs.size()) < f.min_years) continue;
    double max_pop = 0;
    for (const auto& o : cs.obs) max_pop = std::max(max_pop, o.pop);
    if (max_pop < f.min_population) continue;
    p.countries.push_back(std::move(cs));
  }
  std::sort(p.countries.begin(), p.countries.end(),
            [](const CountrySeries& a, const CountrySeries& b) { return a.iso3 < b.iso3; });
  if (p.countries.empty()) throw std::runtime_error("panel is empty: check input paths");
  return p;
}

Panel truncate_panel(const Panel& p, int last_year, int min_years) {
  Panel out;
  for (const auto& c : p.countries) {
    CountrySeries cs{c.iso3, c.name, {}};
    for (const auto& o : c.obs)
      if (o.year <= last_year) cs.obs.push_back(o);
    if (static_cast<int>(cs.obs.size()) >= min_years) out.countries.push_back(std::move(cs));
  }
  return out;
}

}  // namespace e47
