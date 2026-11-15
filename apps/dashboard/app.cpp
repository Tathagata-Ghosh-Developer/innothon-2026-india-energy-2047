#include "app.hpp"

#include <implot.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numeric>

#include "core/stats.hpp"

namespace e47 {

namespace {

using namespace std::chrono_literals;

const ImVec4 kTechColor[kTechs] = {
    {0.42f, 0.42f, 0.45f, 1}, {0.85f, 0.55f, 0.25f, 1}, {0.62f, 0.45f, 0.85f, 1},
    {0.25f, 0.55f, 0.90f, 1}, {0.98f, 0.80f, 0.20f, 1}, {0.30f, 0.78f, 0.72f, 1},
    {0.10f, 0.52f, 0.60f, 1}, {0.45f, 0.72f, 0.32f, 1}, {0.95f, 0.55f, 0.65f, 1}};
const ImVec4 kRegionColor[kRegions] = {
    {0.36f, 0.62f, 0.95f, 1}, {0.98f, 0.62f, 0.25f, 1}, {0.40f, 0.80f, 0.45f, 1},
    {0.92f, 0.38f, 0.38f, 1}, {0.72f, 0.52f, 0.92f, 1}};
const ImVec4 kMuted{0.62f, 0.66f, 0.72f, 1};
const ImVec4 kAccent{0.40f, 0.70f, 1.00f, 1};

std::string with_commas(double v, int prec = 0) {
  char b[64];
  std::snprintf(b, sizeof b, "%.*f", prec, std::abs(v));
  std::string s(b);
  const size_t dot = s.find('.');
  std::string ip = dot == std::string::npos ? s : s.substr(0, dot);
  std::string frac = dot == std::string::npos ? "" : s.substr(dot);
  std::string out;
  for (size_t i = 0; i < ip.size(); ++i) {
    out.push_back(ip[i]);
    const size_t rem = ip.size() - i - 1;
    if (rem > 0 && rem % 3 == 0) out.push_back(',');
  }
  return (v < 0 ? "-" : "") + out + frac;
}

bool slider(const char* label, double* v, double lo, double hi, const char* fmt, double scale, const char* tip) {
  float f = static_cast<float>(*v * scale);
  const bool changed = ImGui::SliderFloat(label, &f, static_cast<float>(lo * scale), static_cast<float>(hi * scale), fmt);
  if (changed) *v = f / scale;
  if (tip) ImGui::SetItemTooltip("%s", tip);
  return changed;
}

void help(const char* text) {
  ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
  ImGui::TextWrapped("%s", text);
  ImGui::PopStyleColor();
}

}  // namespace

const char* App::tab_slug(int t) {
  static const char* s[kTabCount] = {"1_national_forecast", "2_regional_requirement", "3_generation_mix_2047",
                                     "4_models_backtest",   "5_uncertainty_sensitivity", "6_data_assumptions"};
  return s[t];
}

App::App(std::filesystem::path root) : root_(std::move(root)) {
  const std::string r = root_.string();
  loading_ = std::async(std::launch::async, [r] { return std::make_unique<Pipeline>(r); });
}

App::~App() {
  if (job_.valid()) job_.wait();
  if (loading_.valid()) loading_.wait();
}

void App::poll() {
  if (!pl_ && loading_.valid() && loading_.wait_for(0s) == std::future_status::ready) {
    try {
      pl_ = loading_.get();
    } catch (const std::exception& e) {
      error_ = e.what();
    }
  }
  if (busy_ && job_.wait_for(0s) == std::future_status::ready) {
    try {
      cur_ = std::make_shared<Job>(job_.get());
    } catch (const std::exception& e) {
      error_ = e.what();
    }
    busy_ = false;
  }
  if (pl_ && dirty_ && !busy_) launch();
}

void App::launch() {
  dirty_ = false;
  busy_ = true;
  sc_.draws = 1ull << draws_log2_;
  const Pipeline* pl = pl_.get();
  const ScenarioControls c = sc_;
  const PolicySettings p = pol_;
  job_ = std::async(std::launch::async, [pl, c, p] {
    Timer t;
    Job j;
    j.forecast = pl->forecast(c);
    j.strategy = pl->strategy(j.forecast, c, p);
    j.total_ms = t.ms();
    return j;
  });
}

void App::stat_card(const char* label, const std::string& value, const char* note, float width) {
  const float big = font_big_ ? font_big_->FontSize : ImGui::GetFontSize() * 1.6f;
  ImGui::BeginChild(label, ImVec2(width, big + ImGui::GetTextLineHeightWithSpacing() * 3.2f), ImGuiChildFlags_Borders);
  ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
  ImGui::TextUnformatted(label);
  ImGui::PopStyleColor();
  if (font_big_) ImGui::PushFont(font_big_);
  ImGui::TextUnformatted(value.c_str());
  if (font_big_) ImGui::PopFont();
  if (note) help(note);
  ImGui::EndChild();
}

void App::frame() {
  poll();
  const ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(vp->WorkPos);
  ImGui::SetNextWindowSize(vp->WorkSize);
  ImGui::Begin("India Energy 2047", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings);
  if (!error_.empty()) {
    ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Error: %s", error_.c_str());
  }
  if (!pl_ || !cur_) {
    if (font_head_) ImGui::PushFont(font_head_);
    ImGui::TextUnformatted("India Electricity Generation 2047");
    if (font_head_) ImGui::PopFont();
    ImGui::TextUnformatted(!pl_ ? "Loading data and fitting models (bootstrap on all CPU threads)..."
                                : "Running the first scenario ensemble...");
    ImGui::End();
    return;
  }
  const float side = 400.0f * ImGui::GetFontSize() / 15.0f;
  ImGui::BeginChild("sidebar", ImVec2(side, 0), ImGuiChildFlags_Borders);
  sidebar();
  ImGui::EndChild();
  ImGui::SameLine();
  ImGui::BeginChild("main", ImVec2(0, 0));
  header();
  if (ImGui::BeginTabBar("tabs")) {
    static const char* titles[kTabCount] = {"National forecast",  "Regional requirement",       "2047 generation mix",
                                            "Models and backtest", "Uncertainty and sensitivity", "Data and assumptions"};
    for (int t = 0; t < kTabCount; ++t) {
      const ImGuiTabItemFlags fl = force_tab_ == t ? ImGuiTabItemFlags_SetSelected : 0;
      if (ImGui::BeginTabItem(titles[t], nullptr, fl)) {
        switch (t) {
          case 0: tab_forecast(); break;
          case 1: tab_regions(); break;
          case 2: tab_mix(); break;
          case 3: tab_models(); break;
          case 4: tab_sensitivity(); break;
          default: tab_data(); break;
        }
        ImGui::EndTabItem();
      }
    }
    force_tab_ = -1;
    ImGui::EndTabBar();
  }
  ImGui::EndChild();
  ImGui::End();
}

void App::header() {
  if (font_head_) ImGui::PushFont(font_head_);
  ImGui::TextUnformatted("India Electricity Generation 2047");
  if (font_head_) ImGui::PopFont();
  const Job& j = *cur_;
  ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
  ImGui::Text("Energy generated only (TWh). %s scenarios on %s: ensemble %.1f ms, full update %.0f ms.%s",
              with_commas(static_cast<double>(j.forecast.mc.draws)).c_str(), j.forecast.mc.device.c_str(),
              j.forecast.mc.ms, j.total_ms, busy_ ? "  Recomputing..." : "");
  ImGui::PopStyleColor();
  ImGui::Spacing();
}

void App::sidebar() {
  if (font_head_) ImGui::PushFont(font_head_);
  ImGui::TextUnformatted("Scenario");
  if (font_head_) ImGui::PopFont();
  ImGui::PushItemWidth(-ImGui::GetFontSize() * 0.5f);
  {
    // Scenario presets keep the scenario count and device choice.
    const auto& presets = scenario_presets();
    for (size_t i = 0; i < presets.size(); ++i) {
      if (i) ImGui::SameLine();
      if (ImGui::Button((presets[i].name + "##sc").c_str())) {
        const uint64_t draws = sc_.draws;
        const bool gpu = sc_.use_gpu;
        sc_ = ScenarioControls{};
        sc_.draws = draws;
        sc_.use_gpu = gpu;
        presets[i].apply(sc_);
        dirty_ = true;
      }
      ImGui::SetItemTooltip("%s", presets[i].description.c_str());
    }
  }
  const DriverData& d = pl_->drivers();
  double g0 = sc_.g_start >= 0 ? sc_.g_start : d.g_hist_mean;
  ImGui::TextUnformatted("GDP per capita growth, 2026-30");
  if (slider("##g0", &g0, 0.02, 0.08, "%.2f%% a year", 100,
             "Mean annual growth of real GDP per capita (PPP). Default: India's 2005-2025 average."))
    sc_.g_start = g0, dirty_ = true;
  ImGui::TextUnformatted("Growth slowdown by 2047");
  dirty_ |= slider("##slow", &sc_.g_slowdown, 0.0, 0.04, "-%.2f pp", 100,
                   "Fall in annual growth between 2030 and 2047, as income rises.");
  ImGui::TextUnformatted("Growth uncertainty (persistent)");
  dirty_ |= slider("##gsd", &sc_.g_level_sd, 0.0, 0.02, "%.2f pp", 100, "Standard deviation of a draw-level shift in annual growth.");
  ImGui::TextUnformatted("Urban share in 2047, shift");
  dirty_ |= slider("##ush", &sc_.urban_shift_2047, -0.08, 0.08, "%+.1f pp", 100,
                   "Added to the UN WUP 2018 projection for 2047.");
  ImGui::TextUnformatted("Population band (x UN low-high)");
  dirty_ |= slider("##pop", &sc_.pop_band_scale, 0.0, 2.0, "%.2fx", 1, "Width of the population uncertainty relative to UN WPP 2024 low/high variants.");
  ImGui::TextUnformatted("New loads in 2047 (TWh, low-high)");
  dirty_ |= slider("##alo", &sc_.addon_lo, 0.0, 600.0, "low %.0f", 1, "Green hydrogen, data centres and other new loads not in the historical relationship.");
  dirty_ |= slider("##ahi", &sc_.addon_hi, 0.0, 800.0, "high %.0f", 1, nullptr);
  ImGui::TextUnformatted("Model weights (quad / Gompertz / trend)");
  {
    const auto& bw = pl_->fitted().backtest_weights;
    float w[3];
    for (int m = 0; m < 3; ++m) w[m] = static_cast<float>(sc_.w_model[m] >= 0 ? sc_.w_model[m] : bw[m]);
    if (ImGui::SliderFloat3("##w", w, 0.0f, 1.0f, "%.2f")) {
      for (int m = 0; m < 3; ++m) sc_.w_model[m] = w[m];
      dirty_ = true;
    }
    ImGui::SetItemTooltip("Defaults are proportional to 1 / backtest error (2010 origin).");
    if (ImGui::SmallButton("Reset to backtest weights")) {
      sc_.w_model = {-1, -1, -1};
      dirty_ = true;
    }
  }
  ImGui::TextUnformatted("Plan for requirement quantile");
  dirty_ |= slider("##rq", &sc_.requirement_quantile, 0.1, 0.9, "P%.0f", 100, "The 2047 mix is sized for this quantile of the requirement.");
  ImGui::TextUnformatted("Scenarios (2^n)");
  if (ImGui::SliderInt("##draws", &draws_log2_, 16, 25)) dirty_ = true;
  ImGui::SetItemTooltip("%s scenarios", with_commas(std::pow(2.0, draws_log2_)).c_str());
  if (cuda_available()) dirty_ |= ImGui::Checkbox("Use GPU (CUDA)", &sc_.use_gpu);

  ImGui::Spacing();
  if (font_head_) ImGui::PushFont(font_head_);
  ImGui::TextUnformatted("Policy and planning");
  if (font_head_) ImGui::PopFont();
  {
    const auto& presets = policy_presets();
    for (size_t i = 0; i < presets.size(); ++i) {
      if (i) ImGui::SameLine();
      if (ImGui::Button((presets[i].name + "##pol").c_str())) {
        pol_ = PolicySettings{};
        presets[i].apply(pol_);
        dirty_ = true;
      }
      ImGui::SetItemTooltip("%s", presets[i].description.c_str());
    }
  }
  ImGui::TextUnformatted("Minimum non-fossil share");
  dirty_ |= slider("##nf", &pol_.nonfossil_min, 0.4, 0.95, "%.0f%%", 100, "Share of 2047 generation from non-fossil sources.");
  ImGui::TextUnformatted("Coal floor / cap (x 2024 coal)");
  dirty_ |= slider("##cf", &pol_.coal_floor, 0.0, 1.0, "floor %.2f", 1, "Existing young coal fleet still generating in 2047.");
  dirty_ |= slider("##cc", &pol_.coal_cap, 0.5, 1.5, "cap %.2f", 1, "No coal generation beyond the existing and under-construction fleet.");
  ImGui::TextUnformatted("Firm generation: national / regional");
  dirty_ |= slider("##fn", &pol_.firm_national, 0.2, 0.7, "national %.0f%%", 100, "Coal, gas, nuclear, hydro, bioenergy and firmed RE.");
  dirty_ |= slider("##fr", &pol_.firm_regional, 0.0, 0.6, "regional %.0f%%", 100, "Each region's own firm generation.");
  ImGui::TextUnformatted("Maximum gas / nuclear share");
  dirty_ |= slider("##gm", &pol_.gas_max, 0.0, 0.15, "gas %.0f%%", 100, nullptr);
  dirty_ |= slider("##nm", &pol_.nuclear_max, 0.0, 0.25, "nuclear %.0f%%", 100, nullptr);
  ImGui::TextUnformatted("Usable resource (solar / wind / hydro)");
  dirty_ |= slider("##sl", &pol_.solar_land, 0.05, 0.6, "solar %.0f%%", 100, "Fraction of solar generation potential usable by 2047.");
  dirty_ |= slider("##wl", &pol_.wind_land, 0.05, 0.6, "wind %.0f%%", 100, nullptr);
  dirty_ |= slider("##hf", &pol_.hydro_frac, 0.2, 1.0, "hydro %.0f%%", 100, nullptr);
  ImGui::TextUnformatted("Corridor capacity (x 2032 plan)");
  dirty_ |= slider("##ts", &pol_.transfer_scale, 0.5, 4.0, "%.1fx", 1, nullptr);
  ImGui::TextUnformatted("Build-rate acceleration");
  dirty_ |= slider("##ba", &pol_.build_accel, 1.0, 4.0, "%.1fx record year", 1,
                   "2047 generation of each source <= 2025 + years x this x the record annual increase.");
  ImGui::TextUnformatted("Carbon price");
  dirty_ |= slider("##cp", &pol_.carbon_price, 0.0, 100.0, "%.0f USD/t", 1, nullptr);
  if (ImGui::Button("Reset policy")) {
    pol_ = PolicySettings{};
    dirty_ = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("Reset scenario")) {
    sc_ = ScenarioControls{};
    draws_log2_ = 22;
    dirty_ = true;
  }
  ImGui::PopItemWidth();
}

void App::tab_forecast() {
  const ForecastResult& f = cur_->forecast;
  const Fitted& fit = pl_->fitted();
  const size_t n = f.years.size(), last = n - 1;
  const auto& q = f.quant.q[0];
  const HistoryPoint& h0 = fit.history.back();
  const double p50 = q[last][3];
  const double pop47 = f.inputs.pop_med[last] * 1e6;
  const float w = (ImGui::GetContentRegionAvail().x - 3 * ImGui::GetStyle().ItemSpacing.x) / 4;
  stat_card("2047 requirement (median)", with_commas(p50) + " TWh", "Energy generated, P50 of the ensemble", w);
  ImGui::SameLine();
  stat_card("90% range", with_commas(q[last][0]) + " - " + with_commas(q[last][6]), "P5 - P95, TWh", w);
  ImGui::SameLine();
  char buf[64];
  std::snprintf(buf, sizeof buf, "%.2fx  (%.1f%%/yr)", p50 / h0.gen_twh,
                100 * (std::pow(p50 / h0.gen_twh, 1.0 / (f.years.back() - h0.year)) - 1));
  stat_card("Growth vs 2025", buf, "Multiple of 2025 generation, compound rate", w);
  ImGui::SameLine();
  std::snprintf(buf, sizeof buf, "%s kWh", with_commas(p50 * 1e9 / pop47).c_str());
  stat_card("Per person, 2047", buf, ("2025: " + with_commas(h0.gen_twh * 1e9 / h0.pop) + " kWh").c_str(), w);

  ImGui::Checkbox("Show model central paths", &show_models_);
  ImGui::SameLine();
  ImGui::Checkbox("Show CEA NEP generation projections", &show_nep_);

  std::vector<double> hx, hy;
  for (const auto& h : fit.history) {
    hx.push_back(h.year);
    hy.push_back(h.gen_twh);
  }
  std::vector<double> x(n + 1), p[7];
  x[0] = h0.year;
  for (int k = 0; k < 7; ++k) {
    p[k].resize(n + 1);
    p[k][0] = h0.gen_twh;
  }
  for (size_t t = 0; t < n; ++t) {
    x[t + 1] = f.years[t];
    for (int k = 0; k < 7; ++k) p[k][t + 1] = q[t][k];
  }
  if (ImPlot::BeginPlot("##national", ImVec2(-1, ImGui::GetContentRegionAvail().y * 0.80f))) {
    ImPlot::SetupAxes("Year", "Electricity generation (TWh)");
    ImPlot::SetupAxisLimits(ImAxis_X1, 1990, 2048, ImPlotCond_Once);
    ImPlot::SetupAxisLimits(ImAxis_Y1, 0, q[last][6] * 1.08, ImPlotCond_Always);
    ImPlot::SetupLegend(ImPlotLocation_NorthWest);
    ImPlot::SetNextFillStyle(kAccent, 0.18f);
    ImPlot::PlotShaded("90% range (P5-P95)", x.data(), p[0].data(), p[6].data(), static_cast<int>(x.size()));
    ImPlot::SetNextFillStyle(kAccent, 0.35f);
    ImPlot::PlotShaded("50% range (P25-P75)", x.data(), p[2].data(), p[4].data(), static_cast<int>(x.size()));
    ImPlot::SetNextLineStyle(kAccent, 3.0f);
    ImPlot::PlotLine("Median", x.data(), p[3].data(), static_cast<int>(x.size()));
    ImPlot::SetNextLineStyle(ImVec4(0.95f, 0.95f, 0.95f, 1), 2.5f);
    ImPlot::PlotLine("Observed (Ember)", hx.data(), hy.data(), static_cast<int>(hx.size()));
    if (show_models_) {
      const char* names[3] = {"FE-Quad central", "FE-Gompertz central", "Damped trend central"};
      const ImVec4 cols[3] = {{0.95f, 0.65f, 0.30f, 1}, {0.45f, 0.85f, 0.55f, 1}, {0.85f, 0.45f, 0.65f, 1}};
      for (int m = 0; m < 3; ++m) {
        std::vector<double> cy(n + 1);
        cy[0] = h0.gen_twh;
        for (size_t t = 0; t < n; ++t) cy[t + 1] = f.central[m][t];
        ImPlot::SetNextLineStyle(cols[m], 1.5f);
        ImPlot::PlotLine(names[m], x.data(), cy.data(), static_cast<int>(cy.size()));
      }
    }
    if (show_nep_ && !pl_->drivers().nep_generation.empty()) {
      std::vector<double> nx, ny;
      for (auto [xx, yy] : pl_->drivers().nep_generation) {
        nx.push_back(xx);
        ny.push_back(yy);
      }
      ImPlot::SetNextMarkerStyle(ImPlotMarker_Diamond, 7, ImVec4(1, 0.85f, 0.3f, 1));
      ImPlot::PlotScatter("CEA NEP 2022-32 (gross generation)", nx.data(), ny.data(), static_cast<int>(nx.size()));
    }
    ImPlot::EndPlot();
  }
  help("Each scenario combines a population path (UN WPP 2024), an urban-share path (UN WUP 2018), a GDP-per-capita path "
       "with persistent and annual shocks, one of three models with bootstrap parameters, and optional new loads. "
       "Projections are anchored on India's observed 2025 generation.");
}

void App::tab_regions() {
  const ForecastResult& f = cur_->forecast;
  const StrategyResult& s = cur_->strategy;
  const AllocationInputs& alloc = pl_->allocation_inputs();
  const size_t n = f.years.size(), last = n - 1;
  std::vector<double> x(f.years.begin(), f.years.end());
  ImGui::RadioButton("Medians with 50% bands", &region_view_, 0);
  ImGui::SameLine();
  ImGui::RadioButton("Stacked medians", &region_view_, 1);
  const float ph = ImGui::GetContentRegionAvail().y * 0.55f;
  if (ImPlot::BeginPlot("##regions", ImVec2(ImGui::GetContentRegionAvail().x * 0.62f, ph))) {
    ImPlot::SetupAxes("Year", "Requirement (TWh)");
    ImPlot::SetupLegend(ImPlotLocation_NorthWest);
    std::vector<double> base(n, 0.0);
    for (int r = 0; r < kRegions; ++r) {
      const auto& q = f.quant.q[1 + r];
      std::vector<double> lo(n), mid(n), hi(n);
      for (size_t t = 0; t < n; ++t) {
        lo[t] = q[t][2];
        mid[t] = q[t][3];
        hi[t] = q[t][4];
      }
      if (region_view_ == 0) {
        ImPlot::SetNextFillStyle(kRegionColor[r], 0.25f);
        ImPlot::PlotShaded(region_name(r), x.data(), lo.data(), hi.data(), static_cast<int>(n));
        ImPlot::SetNextLineStyle(kRegionColor[r], 2.5f);
        ImPlot::PlotLine(region_name(r), x.data(), mid.data(), static_cast<int>(n));
      } else {
        std::vector<double> top(n);
        for (size_t t = 0; t < n; ++t) top[t] = base[t] + mid[t];
        ImPlot::SetNextFillStyle(kRegionColor[r], 0.75f);
        ImPlot::PlotShaded(region_name(r), x.data(), base.data(), top.data(), static_cast<int>(n));
        base = top;
      }
    }
    ImPlot::EndPlot();
  }
  ImGui::SameLine();
  // Requirement share 2047 vs generation share 2024.
  {
    double req_total = 0, gen_total = 0;
    double req_share[kRegions], gen_share[kRegions];
    double gen[kRegions];
    for (int r = 0; r < kRegions; ++r) {
      gen[r] = 0;
      for (int k = 0; k < kTechs; ++k) gen[r] += alloc.base_twh[r][k];
      gen_total += gen[r];
      req_total += s.requirement[r];
    }
    std::vector<double> vals(2 * kRegions);
    for (int r = 0; r < kRegions; ++r) {
      req_share[r] = 100 * s.requirement[r] / req_total;
      gen_share[r] = 100 * gen[r] / gen_total;
      vals[r] = gen_share[r];
      vals[kRegions + r] = req_share[r];
    }
    if (ImPlot::BeginPlot("Share of national total (%)", ImVec2(-1, ph))) {
      static const char* short_names[kRegions] = {"North", "West", "South", "East", "NE"};
      const double ticks[kRegions] = {0, 1, 2, 3, 4};
      ImPlot::SetupAxes(nullptr, "%", 0, ImPlotAxisFlags_AutoFit);
      ImPlot::SetupAxisTicks(ImAxis_X1, ticks, kRegions, short_names);
      ImPlot::SetupLegend(ImPlotLocation_NorthEast);
      static const char* items[2] = {"Generation 2024", "Requirement 2047"};
      ImPlot::PlotBarGroups(items, vals.data(), 2, kRegions, 0.7);
      ImPlot::EndPlot();
    }
  }
  if (ImGui::BeginTable("regtable", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
    for (const char* hname : {"Region", "2047 P5", "2047 P50", "2047 P95", "Planned requirement", "Share 2047",
                              "Generation 2024", "Growth"})
      ImGui::TableSetupColumn(hname);
    ImGui::TableHeadersRow();
    double tot = 0;
    for (int r = 0; r < kRegions; ++r) tot += s.requirement[r];
    for (int r = 0; r < kRegions; ++r) {
      const auto& q = f.quant.q[1 + r][last];
      double g24 = 0;
      for (int k = 0; k < kTechs; ++k) g24 += alloc.base_twh[r][k];
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::TextColored(kRegionColor[r], "%s", region_name(r));
      ImGui::TableNextColumn();
      ImGui::Text("%s", with_commas(q[0]).c_str());
      ImGui::TableNextColumn();
      ImGui::Text("%s", with_commas(q[3]).c_str());
      ImGui::TableNextColumn();
      ImGui::Text("%s", with_commas(q[6]).c_str());
      ImGui::TableNextColumn();
      ImGui::Text("%s TWh", with_commas(s.requirement[r]).c_str());
      ImGui::TableNextColumn();
      ImGui::Text("%.1f%%", 100 * s.requirement[r] / tot);
      ImGui::TableNextColumn();
      ImGui::Text("%s", with_commas(g24).c_str());
      ImGui::TableNextColumn();
      ImGui::Text("%.2fx", s.requirement[r] / std::max(1.0, g24));
    }
    ImGui::EndTable();
  }
  help("Requirement is driven by each state's population, income and urban share through the same per-person response "
       "as the national model, then scaled to the national total. It is not where power is generated: today the "
       "Western and Eastern regions generate more than their share and export to the North.");
}

void App::tab_mix() {
  const StrategyResult& s = cur_->strategy;
  const AllocationInputs& alloc = pl_->allocation_inputs();
  ImGui::RadioButton("Least-cost optimisation (LP)", &mix_method_, 0);
  ImGui::SameLine();
  ImGui::RadioButton("Resource-score allocation", &mix_method_, 1);
  const AllocationResult& a = mix_method_ == 0 ? s.least_cost : s.score;
  if (!a.ok) {
    ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Optimisation status: %s", a.status.c_str());
    return;
  }
  double unserved = 0;
  for (double u : a.unserved) unserved += u;
  const float w = (ImGui::GetContentRegionAvail().x - 3 * ImGui::GetStyle().ItemSpacing.x) / 4;
  char buf[64];
  std::snprintf(buf, sizeof buf, "%.1f%%", 100 * a.nonfossil_share());
  stat_card("Non-fossil share 2047", buf, "Of national generation", w);
  ImGui::SameLine();
  stat_card("CO2 from generation", with_commas(a.co2_mt) + " Mt", "CEA emission factors (coal 0.969, gas 0.452 t/MWh)", w);
  ImGui::SameLine();
  stat_card("Generation cost", with_commas(a.cost_busd) + " bn USD/yr", "2047 levelised costs (IEA WEO 2025 basis)", w);
  ImGui::SameLine();
  stat_card("Unmet requirement", with_commas(unserved, 1) + " TWh", unserved > 0.1 ? "Constraints too tight: relax a limit" : "System is adequate", w);

  const float ph = ImGui::GetContentRegionAvail().y * 0.48f;
  std::vector<const char*> labels(kTechs);
  for (int k = 0; k < kTechs; ++k) labels[k] = tech_label(k);
  // Stacked bars: national 2024, national 2047, then five regions in 2047.
  constexpr int groups = 2 + kRegions;
  std::vector<double> vals(static_cast<size_t>(kTechs) * groups, 0.0);
  for (int k = 0; k < kTechs; ++k) {
    for (int r = 0; r < kRegions; ++r) {
      vals[k * groups + 0] += alloc.base_twh[r][k];
      vals[k * groups + 2 + r] = a.gen[r][k];
    }
    vals[k * groups + 1] = a.national[k];
  }
  if (ImPlot::BeginPlot("Generation by source (TWh)", ImVec2(ImGui::GetContentRegionAvail().x * 0.62f, ph))) {
    static const char* glabels[groups] = {"India 2024", "India 2047", "North", "West", "South", "East", "NE"};
    const double ticks[groups] = {0, 1, 2, 3, 4, 5, 6};
    ImPlot::SetupAxes(nullptr, "TWh", 0, ImPlotAxisFlags_AutoFit);
    ImPlot::SetupAxisTicks(ImAxis_X1, ticks, groups, glabels);
    ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
    static ImPlotColormap cmap = -1;
    if (cmap < 0) cmap = ImPlot::AddColormap("e47_techs", kTechColor, kTechs);
    ImPlot::PushColormap(cmap);
    ImPlot::PlotBarGroups(labels.data(), vals.data(), kTechs, groups, 0.7, 0, ImPlotBarGroupsFlags_Stacked);
    ImPlot::PopColormap();
    ImPlot::EndPlot();
  }
  ImGui::SameLine();
  if (ImPlot::BeginPlot("Net export (+) / import (-), TWh", ImVec2(-1, ph))) {
    static const char* rl[kRegions] = {"North", "West", "South", "East", "NE"};
    const double ticks[kRegions] = {0, 1, 2, 3, 4};
    ImPlot::SetupAxes(nullptr, "TWh", 0, ImPlotAxisFlags_AutoFit);
    ImPlot::SetupAxisTicks(ImAxis_X1, ticks, kRegions, rl);
    for (int r = 0; r < kRegions; ++r) {
      const double xx = r, yy = a.net_export[r];
      ImPlot::SetNextFillStyle(kRegionColor[r]);
      ImPlot::PlotBars(region_name(r), &xx, &yy, 1, 0.6);
    }
    ImPlot::EndPlot();
  }
  if (ImGui::BeginTable("mixtable", kTechs + 3,
                        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
    ImGui::TableSetupColumn("Region");
    for (int k = 0; k < kTechs; ++k) ImGui::TableSetupColumn(tech_label(k));
    ImGui::TableSetupColumn("Total");
    ImGui::TableSetupColumn(mix_method_ == 0 ? "Marginal cost" : "Requirement");
    ImGui::TableHeadersRow();
    for (int r = 0; r <= kRegions; ++r) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      double tot = 0;
      for (int k = 0; k < kTechs; ++k) tot += r < kRegions ? a.gen[r][k] : a.national[k];
      if (r < kRegions)
        ImGui::TextColored(kRegionColor[r], "%s", region_name(r));
      else
        ImGui::TextUnformatted("India");
      for (int k = 0; k < kTechs; ++k) {
        ImGui::TableNextColumn();
        const double g = r < kRegions ? a.gen[r][k] : a.national[k];
        if (g < 0.05)
          ImGui::TextDisabled("-");
        else
          ImGui::Text("%s (%.0f%%)", with_commas(g).c_str(), 100 * g / tot);
      }
      ImGui::TableNextColumn();
      ImGui::Text("%s", with_commas(tot).c_str());
      ImGui::TableNextColumn();
      if (r < kRegions) {
        if (mix_method_ == 0)
          ImGui::Text("%.1f USD/MWh", a.marginal_cost[r]);
        else
          ImGui::Text("%s", with_commas(s.requirement[r]).c_str());
      }
    }
    ImGui::EndTable();
  }
  if (mix_method_ == 0 && !a.corridor_flow.empty()) {
    ImGui::Text("Inter-regional flows (TWh, positive = first region to second):");
    for (size_t c = 0; c < a.corridor_flow.size(); ++c) {
      const Corridor& k = alloc.corridors[c];
      const double cap = k.mw_2032 * s.policy.transfer_scale * s.policy.transfer_util * 8.76e-3;
      ImGui::SameLine();
      ImGui::Text(" %s %+.0f/%.0f", k.name.c_str(), a.corridor_flow[c], cap);
    }
  }
  help(mix_method_ == 0
           ? "Least cost: minimises 2047 generation cost subject to each region's requirement, resource limits, firm "
             "generation floors, the non-fossil minimum, build-rate limits and corridor limits."
           : "Resource score: the same national totals by source, shared across regions in proportion to usable "
             "resource (solar, wind, hydro, bioenergy potential), existing fleets (coal, gas) and nuclear sites.");
}

void App::tab_models() {
  const Fitted& fit = pl_->fitted();
  static int origin = 1;
  ImGui::Text("Backtest origin:");
  for (int i = 0; i < static_cast<int>(fit.backtests.size()); ++i) {
    ImGui::SameLine();
    char b[16];
    std::snprintf(b, sizeof b, "%d", fit.backtests[i].origin);
    ImGui::RadioButton(b, &origin, i);
  }
  const BacktestResult& bt = fit.backtests[origin];
  const float ph = ImGui::GetContentRegionAvail().y * 0.5f;
  if (ImPlot::BeginPlot("Out-of-sample backtest: India generation (TWh)", ImVec2(ImGui::GetContentRegionAvail().x * 0.6f, ph))) {
    ImPlot::SetupAxes("Year", "TWh");
    ImPlot::SetupLegend(ImPlotLocation_NorthWest);
    std::vector<double> hx, hy;
    for (const auto& h : fit.history) {
      hx.push_back(h.year);
      hy.push_back(h.gen_twh);
    }
    ImPlot::SetNextLineStyle(ImVec4(0.95f, 0.95f, 0.95f, 1), 2.5f);
    ImPlot::PlotLine("Observed", hx.data(), hy.data(), static_cast<int>(hx.size()));
    std::vector<double> x, m[3];
    for (const auto& p : bt.points) {
      x.push_back(p.year);
      m[0].push_back(p.fe_quad_twh);
      m[1].push_back(p.fe_gompertz_twh);
      m[2].push_back(p.damped_twh);
    }
    const char* names[3] = {"FE-Quad", "FE-Gompertz", "Damped trend"};
    const ImVec4 cols[3] = {{0.95f, 0.65f, 0.30f, 1}, {0.45f, 0.85f, 0.55f, 1}, {0.85f, 0.45f, 0.65f, 1}};
    for (int k = 0; k < 3; ++k) {
      ImPlot::SetNextLineStyle(cols[k], 2.0f);
      ImPlot::PlotLine(names[k], x.data(), m[k].data(), static_cast<int>(x.size()));
    }
    const double ox = bt.origin;
    ImPlot::PlotInfLines("Fit ends", &ox, 1);
    ImPlot::EndPlot();
  }
  ImGui::SameLine();
  ImGui::BeginGroup();
  if (ImGui::BeginTable("bt", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
    for (const char* hname : {"Origin", "FE-Quad", "FE-Gompertz", "Damped"}) ImGui::TableSetupColumn(hname);
    ImGui::TableHeadersRow();
    for (const auto& b : fit.backtests) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("%d MAPE", b.origin);
      for (int k = 0; k < 3; ++k) {
        ImGui::TableNextColumn();
        ImGui::Text("%.1f%%", 100 * b.mape[k]);
      }
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("  2025 error");
      for (int k = 0; k < 3; ++k) {
        ImGui::TableNextColumn();
        const double e = 100 * b.ape_last[k];
        ImGui::TextColored(e <= 10 ? ImVec4(0.5f, 0.9f, 0.5f, 1) : ImVec4(1, 0.6f, 0.4f, 1), "%.1f%%", e);
      }
    }
    ImGui::EndTable();
  }
  ImGui::Text("Ensemble weights: %.2f / %.2f / %.2f", fit.backtest_weights[0], fit.backtest_weights[1],
              fit.backtest_weights[2]);
  help("Weights are proportional to 1 / MAPE from the 2010 origin (15-year horizon). Pass bar: 2025 error below 10%.");
  ImGui::EndGroup();

  // Parameter table with bootstrap intervals.
  auto ci = [](auto&& getter, size_t n) {
    std::vector<double> v(n);
    for (size_t i = 0; i < n; ++i) v[i] = getter(i);
    return std::array<double, 2>{quantile(v, 0.05), quantile(v, 0.95)};
  };
  const size_t nb = fit.quad_boot.size();
  if (ImGui::BeginTable("params", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
    for (const char* hname : {"Parameter", "Estimate", "Bootstrap 90% interval", "Meaning"}) ImGui::TableSetupColumn(hname);
    ImGui::TableHeadersRow();
    auto row = [&](const char* name, double est, std::array<double, 2> c, const char* meaning) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(name);
      ImGui::TableNextColumn();
      ImGui::Text("%.4f", est);
      ImGui::TableNextColumn();
      ImGui::Text("[%.4f, %.4f]", c[0], c[1]);
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(meaning);
    };
    row("FE-Quad b1", fit.quad.b1, ci([&](size_t i) { return fit.quad_boot[i].b1; }, nb), "Income elasticity at $10,000");
    row("FE-Quad b2", fit.quad.b2, ci([&](size_t i) { return fit.quad_boot[i].b2; }, nb), "Elasticity decline with income");
    row("FE-Quad g", fit.quad.g, ci([&](size_t i) { return fit.quad_boot[i].g; }, nb), "Urban share effect (log points per unit share)");
    row("FE-Quad d", fit.quad.d, ci([&](size_t i) { return fit.quad_boot[i].d; }, nb), "Trend per decade");
    row("FE-Gompertz a", fit.gomp.a, ci([&](size_t i) { return fit.gomp_boot[i].a; }, nb), "Saturation depth");
    row("FE-Gompertz b", fit.gomp.b, ci([&](size_t i) { return fit.gomp_boot[i].b; }, nb), "Saturation speed per $1,000");
    row("FE-Gompertz c", fit.gomp.c, ci([&](size_t i) { return fit.gomp_boot[i].c; }, nb), "Urban share effect");
    row("FE-Gompertz d", fit.gomp.d, ci([&](size_t i) { return fit.gomp_boot[i].d; }, nb), "Trend per decade");
    ImGui::EndTable();
  }
  help(("Panel: " + std::to_string(fit.panel.countries.size()) + " countries, " + std::to_string(fit.panel.n_obs()) +
        " country-years (Ember generation, World Bank drivers). " + std::to_string(nb) +
        " country-cluster bootstrap replicates per model, fitted in parallel in " + with_commas(fit.bootstrap_ms) + " ms.")
           .c_str());
}

void App::tab_sensitivity() {
  const ForecastResult& f = cur_->forecast;
  const StrategyResult& s = cur_->strategy;
  const auto& keep = f.mc.keep;
  const size_t nk = keep.size() / kKeepFeatures;
  const float ph = ImGui::GetContentRegionAvail().y * 0.48f;
  // Variance attribution for 2047 national generation from kept draws.
  if (nk > 100) {
    std::vector<double> y(nk);
    for (size_t i = 0; i < nk; ++i) y[i] = keep[i * kKeepFeatures + 7];
    // Rank (Spearman) measures: robust to the heavy tail of the statistical trend model.
    auto ranks = [](const std::vector<double>& v) {
      std::vector<size_t> idx(v.size());
      std::iota(idx.begin(), idx.end(), 0);
      std::sort(idx.begin(), idx.end(), [&](size_t a, size_t b) { return v[a] < v[b]; });
      std::vector<double> r(v.size());
      for (size_t i = 0; i < idx.size(); ++i) r[idx[i]] = static_cast<double>(i);
      return r;
    };
    const std::vector<double> yr = ranks(y);
    const double my = mean(yr), sy = stdev(yr);
    const char* names[5] = {"Population path", "Urban share path", "GDP growth (persistent)", "Income convergence",
                            "New loads"};
    const int idx[5] = {0, 1, 2, 3, 6};
    double r2[6];
    for (int k = 0; k < 5; ++k) {
      std::vector<double> xv(nk);
      for (size_t i = 0; i < nk; ++i) xv[i] = keep[i * kKeepFeatures + idx[k]];
      const double sxraw = stdev(xv);
      if (sxraw <= 0) {
        r2[k] = 0;
        continue;
      }
      const std::vector<double> xr = ranks(xv);
      const double mx = mean(xr), sx = stdev(xr);
      double c = 0;
      for (size_t i = 0; i < nk; ++i) c += (xr[i] - mx) * (yr[i] - my);
      c /= (nk - 1);
      r2[k] = (sx > 0 && sy > 0) ? std::pow(c / (sx * sy), 2) : 0.0;
    }
    // Model choice: between-model share of rank variance (correlation ratio).
    {
      double sum[3] = {0, 0, 0}, cnt[3] = {0, 0, 0};
      for (size_t i = 0; i < nk; ++i) {
        const int m = static_cast<int>(keep[i * kKeepFeatures + 4]);
        sum[m] += yr[i];
        cnt[m] += 1;
      }
      double between = 0;
      for (int m = 0; m < 3; ++m)
        if (cnt[m] > 0) between += cnt[m] * std::pow(sum[m] / cnt[m] - my, 2);
      r2[5] = between / ((nk - 1) * sy * sy);
    }
    static const char* labels[6] = {"Population path", "Urban share path", "GDP growth (persistent)",
                                    "Income convergence", "New loads", "Model choice"};
    if (ImPlot::BeginPlot("What drives 2047 uncertainty (rank R-squared, one input at a time)", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, ph))) {
      const double ticks[6] = {0, 1, 2, 3, 4, 5};
      ImPlot::SetupAxes("Share of variance", nullptr, ImPlotAxisFlags_AutoFit, 0);
      ImPlot::SetupAxisTicks(ImAxis_Y1, ticks, 6, labels);
      double pos[6] = {0, 1, 2, 3, 4, 5};
      ImPlot::SetNextFillStyle(kAccent);
      // Horizontal bars: first array = bar lengths, second = positions.
      ImPlot::PlotBars("##r2", r2, pos, 6, 0.6, ImPlotBarsFlags_Horizontal);
      ImPlot::EndPlot();
    }
    (void)names;
    ImGui::SameLine();
    if (ImPlot::BeginPlot("Distribution of 2047 national generation", ImVec2(-1, ph))) {
      ImPlot::SetupAxes("TWh", "Scenarios", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
      ImPlot::SetNextFillStyle(kAccent, 0.7f);
      const ImPlotRange range(quantile(y, 0.005), quantile(y, 0.995));
      ImPlot::PlotHistogram("Scenarios (P0.5-P99.5 shown)", y.data(), static_cast<int>(nk), 60, 1.0, range);
      ImPlot::EndPlot();
    }
  }
  // Policy tornado on CO2 and non-fossil share.
  if (!s.tornado.empty() && s.least_cost.ok) {
    const int nt = static_cast<int>(s.tornado.size());
    std::vector<const char*> labels(nt);
    std::vector<double> lo(nt), hi(nt), pos(nt);
    std::vector<std::string> names(nt);
    for (int i = 0; i < nt; ++i) {
      const auto& tr = s.tornado[i];
      names[i] = tr.name + ((!tr.low.ok || !tr.high.ok) ? " *" : "");
      labels[i] = names[i].c_str();
      lo[i] = tr.low.ok ? tr.low.co2_mt - s.least_cost.co2_mt : 0;
      hi[i] = tr.high.ok ? tr.high.co2_mt - s.least_cost.co2_mt : 0;
      pos[i] = i;
    }
    if (ImPlot::BeginPlot("Policy sensitivity: change in 2047 CO2 (Mt) vs base", ImVec2(ImGui::GetContentRegionAvail().x * 0.62f, -1))) {
      ImPlot::SetupAxes("Mt CO2", nullptr, ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
      ImPlot::SetupAxisTicks(ImAxis_Y1, pos.data(), nt, labels.data());
      ImPlot::SetupLegend(ImPlotLocation_SouthEast);
      ImPlot::SetNextFillStyle(ImVec4(0.40f, 0.75f, 0.95f, 1));
      ImPlot::PlotBars("Low setting", lo.data(), pos.data(), nt, 0.6, ImPlotBarsFlags_Horizontal);
      ImPlot::SetNextFillStyle(ImVec4(0.95f, 0.55f, 0.35f, 1));
      ImPlot::PlotBars("High setting", hi.data(), pos.data(), nt, 0.6, ImPlotBarsFlags_Horizontal);
      ImPlot::EndPlot();
    }
    ImGui::SameLine();
    ImGui::BeginGroup();
    if (ImGui::BeginTable("tornado", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
      for (const char* hname : {"Setting (low / high)", "Coal share", "Non-fossil", "Cost bn USD"}) ImGui::TableSetupColumn(hname);
      ImGui::TableHeadersRow();
      auto cell = [](const AllocationResult& a, auto fn, const char* fmt) {
        char b[32];
        if (!a.ok) return std::string("infeasible");
        std::snprintf(b, sizeof b, fmt, fn(a));
        return std::string(b);
      };
      for (const auto& t : s.tornado) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("%s (%s / %s)", t.name.c_str(), t.low_label.c_str(), t.high_label.c_str());
        auto coal = [](const AllocationResult& a) { return 100 * a.share(kCoal); };
        auto nf = [](const AllocationResult& a) { return 100 * a.nonfossil_share(); };
        auto cost = [](const AllocationResult& a) { return a.cost_busd; };
        ImGui::TableNextColumn();
        ImGui::Text("%s / %s", cell(t.low, coal, "%.0f%%").c_str(), cell(t.high, coal, "%.0f%%").c_str());
        ImGui::TableNextColumn();
        ImGui::Text("%s / %s", cell(t.low, nf, "%.0f%%").c_str(), cell(t.high, nf, "%.0f%%").c_str());
        ImGui::TableNextColumn();
        ImGui::Text("%s / %s", cell(t.low, cost, "%.0f").c_str(), cell(t.high, cost, "%.0f").c_str());
      }
      ImGui::EndTable();
    }
    help(("Each row moves one setting to a low and a high value and re-solves the least-cost plan; " +
          std::to_string(2 * s.tornado.size()) + " optimisations ran in parallel in " + with_commas(s.tornado_ms, 1) +
          " ms. * = one setting cannot be met together with the other constraints (infeasible).")
             .c_str());
    ImGui::EndGroup();
  }
}

void App::tab_data() {
  const AllocationInputs& alloc = pl_->allocation_inputs();
  const DriverData& d = pl_->drivers();
  ImGui::TextWrapped(
      "Compliance: forecasting and comparison use energy generated only. Installed capacity and consumption are never "
      "forecasting inputs. Resource potentials published in GW are converted to generation potential (TWh/yr) with "
      "sourced capacity factors and used only as upper bounds. Capacity-based policy targets are context only.");
  ImGui::Spacing();
  if (ImGui::BeginTable("sources", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
    for (const char* hname : {"Dataset", "Source", "Used for"}) ImGui::TableSetupColumn(hname);
    ImGui::TableHeadersRow();
    const char* rows[][3] = {
        {"Electricity generation, 153 countries, 1990-2025", "Ember yearly electricity data", "Cross-country models, India history"},
        {"GDP per capita PPP, population, urban share", "World Bank WDI", "Model drivers"},
        {"India regional and state generation by source", "Ember India; CEA General Review", "2024 base mix, existing fleets"},
        {"Population projections 2026-2047", "UN WPP 2024 (medium, low, high)", "National population paths"},
        {"State population projections 2011-2036", "Technical Group on Population Projections (2020)", "Regional requirement split"},
        {"Urban share projections", "UN WUP 2018; Technical Group", "Urbanisation paths, state urban gaps"},
        {"State GSDP and population 2006-2024", "Organiser dataset (capacity columns not used)", "Income convergence, relative incomes"},
        {"Solar, wind, hydro, bioenergy potential", "NISE, NIWE, CEA reassessment, MNRE", "Upper bounds on generation"},
        {"Capacity factors", "CEA NEP 2022-32; Ember observed", "Potential to generation conversion"},
        {"Costs 2047", "IEA WEO 2025; SECI auctions; CERC", "Least-cost optimisation"},
        {"Emission factors", "CEA CO2 Baseline Database v21", "CO2 of each plan"},
        {"Inter-regional corridors", "CEA / NEP Vol II (2032 plan)", "Transfer limits and losses"},
        {"Nuclear pipeline", "DAE / NPCIL", "Nuclear siting key"},
        {"Generation projections 2026-27, 2031-32", "CEA NEP 2022-32", "Benchmark only"},
    };
    for (const auto& r : rows) {
      ImGui::TableNextRow();
      for (int c = 0; c < 3; ++c) {
        ImGui::TableNextColumn();
        ImGui::TextWrapped("%s", r[c]);
      }
    }
    ImGui::EndTable();
  }
  ImGui::Spacing();
  ImGui::Text("Allocation inputs by region (base 2024 TWh / usable potential TWh / cost USD per MWh):");
  if (ImGui::BeginTable("ainputs", kTechs + 1, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
    ImGui::TableSetupColumn("Region");
    for (int k = 0; k < kTechs; ++k) ImGui::TableSetupColumn(tech_label(k));
    ImGui::TableHeadersRow();
    for (int r = 0; r < kRegions; ++r) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::TextColored(kRegionColor[r], "%s", region_name(r));
      for (int k = 0; k < kTechs; ++k) {
        ImGui::TableNextColumn();
        ImGui::Text("%.0f / %.0f / %.0f", alloc.base_twh[r][k], alloc.potential_twh[r][k], alloc.cost[r][k]);
      }
    }
    ImGui::EndTable();
  }
  ImGui::Spacing();
  ImGui::Text("Derivation notes:");
  for (const auto& n : alloc.notes) ImGui::BulletText("%s", n.c_str());
  ImGui::BulletText("GDP per capita growth %d-%d: mean %.2f%%, s.d. %.2f%%; income convergence %.3f/yr (s.e. %.3f).",
                    d.g_hist_from, d.g_hist_to, 100 * d.g_hist_mean, 100 * d.g_hist_sd,
                    pl_->fitted().convergence.lambda, pl_->fitted().convergence.lambda_se);
}

}  // namespace e47
