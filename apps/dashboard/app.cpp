#include "app.hpp"

#include <implot.h>

#include <algorithm>
#include <cctype>
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

const char* App::shot_slug(int k) {
  if (k < kTabCount) return tab_slug(k);
  return k == kTabCount ? "6b_data_requirement_evidence" : "6c_data_mix_evidence";
}

void App::force_shot(int k) {
  force_tab_ = std::min(k, kTabCount - 1);
  if (k >= kTabCount - 1) force_data_view_ = k - (kTabCount - 1);  // 0 sources, 1 requirement, 2 mix
}

App::App(std::filesystem::path root) : root_(std::move(root)) {
  // Open on the recommended plan (Robust policy); the Base policy is one click away.
  for (const auto& pp : policy_presets())
    if (pp.name == "Robust") pp.apply(pol_);
  request_core(0);
}

App::~App() {
  if (job_.valid()) job_.wait();
  for (auto& l : loading_)
    if (l.valid()) l.wait();
}

void App::request_core(int core) {
  // Switch at once if that core is loaded; otherwise keep showing the current core
  // until the new one is ready, so a failed load never leaves the page without controls.
  if (pls_[core]) {
    core_ = core;
    pending_core_ = -1;
    return;
  }
  if (!pl_) core_ = core;  // first load: nothing to keep showing
  pending_core_ = core;
  if (loading_[core].valid()) return;
  const std::string r = root_.string();
  loading_[core] = std::async(std::launch::async,
                              [r, core] { return std::make_unique<Pipeline>(r, static_cast<CoreModel>(core)); });
}

void App::poll() {
  for (int k = 0; k < 2; ++k) {
    if (!pls_[k] && loading_[k].valid() && loading_[k].wait_for(0s) == std::future_status::ready) {
      try {
        pls_[k] = loading_[k].get();
        if (pending_core_ == k) {
          core_ = k;
          error_.clear();
        }
      } catch (const std::exception& e) {
        error_ = std::string(core_name(static_cast<CoreModel>(k))) + " could not be loaded: " + e.what();
      }
      if (pending_core_ == k) pending_core_ = -1;
    }
  }
  if (pls_[core_].get() != pl_) {
    // Core switched (or finished loading): results of the other core no longer apply.
    pl_ = pls_[core_].get();
    cur_.reset();
    dirty_ = true;
  }
  if (busy_ && job_.wait_for(0s) == std::future_status::ready) {
    try {
      Job j = job_.get();
      if (j.core == core_) cur_ = std::make_shared<Job>(std::move(j));
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
  const Pipeline* pl = pl_;
  const ScenarioControls c = sc_;
  const PolicySettings p = pol_;
  const int core = core_;
  job_ = std::async(std::launch::async, [pl, c, p, core] {
    Timer t;
    Job j;
    j.core = core;
    j.forecast = pl->forecast(c);
    j.strategy = pl->strategy(j.forecast, c, p);
    // Evidence tables (data tab). The decomposition and the state table belong to the India
    // core. The P90 requirement (the national P90 split by the regional medians, as
    // Pipeline::strategy does) is kept for the mix sensitivity, which data_mix() runs only
    // while its view is open.
    const int last = static_cast<int>(j.forecast.years.size()) - 1;
    j.india = pl->core() == CoreModel::kIndia;
    try {
      if (j.india) {
        j.growth = pl->decompose_central(c, j.forecast.years[last]);
        j.states = pl->state_table(c, j.forecast.years[last]);
      }
      double planned = 0;
      for (double x : j.strategy.requirement) planned += x;
      const double p90 = j.forecast.mc.quantile(j.forecast.inputs, 0, last, 0.9);
      j.requirement_p90 = j.strategy.requirement;
      for (double& x : j.requirement_p90) x *= planned > 0 ? p90 / planned : 0.0;
      j.bounds = make_bounds(pl->allocation_inputs(), p, j.strategy.requirement);
      for (int s = 0; s < kTechs; ++s)
        j.national_hi[s] = national_upper_bound_twh(pl->allocation_inputs(), p, j.strategy.requirement, s);
    } catch (const std::exception& e) {
      j.evidence_error = e.what();
    }
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
    ImGui::Text("%s: %s", core_name(static_cast<CoreModel>(core_)),
                !pl_ ? "loading data and fitting models..."
                     : "running the scenario ensemble...");
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
  ImGui::Text("%s. Energy generated only (TWh)%s. %s scenarios on %s: ensemble %.1f ms, full update %.0f ms.%s",
              core_name(pl_->core()),
              pl_->core() == CoreModel::kIndia ? "; years are fiscal years by start year (2047 = FY2047-48)" : "",
              with_commas(static_cast<double>(j.forecast.mc.draws)).c_str(), j.forecast.mc.device.c_str(),
              j.forecast.mc.ms, j.total_ms, busy_ ? "  Recomputing..." : "");
  ImGui::PopStyleColor();
  ImGui::Spacing();
}

void App::sidebar() {
  if (font_head_) ImGui::PushFont(font_head_);
  ImGui::TextUnformatted("Forecast core");
  if (font_head_) ImGui::PopFont();
  {
    int c = pending_core_ >= 0 ? pending_core_ : core_;
    ImGui::RadioButton("India-only (primary)", &c, 0);
    ImGui::SetItemTooltip("NPP generation and the organiser state data (the two challenge datasets). Urban share and "
                          "population paths are external drivers.");
    ImGui::SameLine();
    ImGui::RadioButton("Cross-country (check)", &c, 1);
    ImGui::SetItemTooltip("Independent check used for justification, not for estimation: 153-country Ember / World "
                          "Bank panel. Calendar years, total generation beyond NPP's utility series.");
    if (c != (pending_core_ >= 0 ? pending_core_ : core_)) request_core(c);
    if (pending_core_ >= 0) help("Loading the other forecast core...");
  }
  const bool india = pl_->core() == CoreModel::kIndia;
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
    // Base plus the published new loads absent from the historical series: the add-on
    // comes from data/external/new_loads.csv, so it is read from the loaded pipeline.
    if (new_loads_for_ != pl_) {
      new_loads_for_ = pl_;
      try {
        new_loads_ = pl_->new_loads();
        new_loads_desc_ = new_loads_description(new_loads_);
      } catch (const std::exception& e) {
        new_loads_.clear();
        new_loads_desc_ = std::string("Not available: ") + e.what();
      }
    }
    ImGui::BeginDisabled(new_loads_.empty());
    if (ImGui::Button(kNewLoadsPresetName)) {
      const uint64_t draws = sc_.draws;
      const bool gpu = sc_.use_gpu;
      sc_ = ScenarioControls{};
      sc_.draws = draws;
      sc_.use_gpu = gpu;
      apply_new_loads(sc_, new_loads_);
      dirty_ = true;
    }
    ImGui::EndDisabled();
    ImGui::SetItemTooltip("%s", new_loads_desc_.c_str());
  }
  const DriverData& d = pl_->drivers();
  double g0 = (sc_.g_start >= 0 ? sc_.g_start : d.g_hist_mean) + sc_.g_start_shift;
  ImGui::TextUnformatted(india ? "Income growth per person, to 2030" : "GDP per capita growth, to 2030");
  if (slider("##g0", &g0, 0.0, 0.08, "%.2f%% a year", 100,
             india ? "Mean annual growth of real GSDP per person (organiser data). Default: FY2012-13 to FY2023-24 "
                     "average, pandemic years left out."
                   : "Mean annual growth of real GDP per capita (PPP). Default: India's 2005-2025 average."))
    sc_.g_start = g0, sc_.g_start_shift = 0, dirty_ = true;
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
  dirty_ |= slider("##alo", &sc_.addon_lo, 0.0, 1000.0, "low %.0f", 1, "Loads absent from the historical series, such as green hydrogen electrolysis; end-use TWh, outside the headline.");
  dirty_ |= slider("##ahi", &sc_.addon_hi, 0.0, 1500.0, "high %.0f", 1, nullptr);
  if (india) {
    ImGui::TextUnformatted("Elasticity decline with income");
    dirty_ |= slider("##edec", &sc_.elasticity_decline, 0.0, 0.3, "%.2f per ln income", 1,
                     "0 = the estimated constant elasticity. A positive value lets the income elasticity fall as "
                     "income rises (saturation), keeping its base-year value.");
  }
  {
    const auto& names = pl_->fitted().model_names;
    ImGui::Text("Model weights (%s / %s / %s)", india ? "urban prior" : names[0].c_str(),
                india ? "no urban term" : names[1].c_str(), india ? "trend" : names[2].c_str());
    const auto& bw = pl_->fitted().backtest_weights;
    float w[3];
    for (int m = 0; m < 3; ++m) w[m] = static_cast<float>(sc_.w_model[m] >= 0 ? sc_.w_model[m] : bw[m]);
    if (ImGui::SliderFloat3("##w", w, 0.0f, 1.0f, "%.2f")) {
      for (int m = 0; m < 3; ++m) sc_.w_model[m] = w[m];
      dirty_ = true;
    }
    ImGui::SetItemTooltip(india ? "Default 1 / 0 / 0: the driver model with the literature urbanisation prior. "
                                  "0 / 1 / 0 is the two-dataset-only ablation; 0 / 0 / 1 the benchmark without drivers."
                                : "Defaults are proportional to 1 / backtest error (2010 origin).");
    if (ImGui::SmallButton("Reset weights")) {
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
  ImGui::TextUnformatted("Coal floor / cap (x FY2023-24 coal)");
  dirty_ |= slider("##cf", &pol_.coal_floor, 0.0, 1.0, "floor %.2f", 1, "Existing young coal fleet still generating in 2047.");
  dirty_ |= slider("##cc", &pol_.coal_cap, 0.5, 1.5, "cap %.2f", 1, "No coal generation beyond the existing and under-construction fleet.");
  ImGui::TextUnformatted("Firm generation: national / regional");
  dirty_ |= slider("##fn", &pol_.firm_national, 0.2, 0.7, "national %.0f%%", 100, "Coal, gas, nuclear, hydro, bioenergy and firmed RE.");
  dirty_ |= slider("##fr", &pol_.firm_regional, 0.0, 0.6, "regional %.0f%%", 100, "Each region's own firm generation.");
  ImGui::TextUnformatted("Maximum gas / nuclear share");
  dirty_ |= slider("##gm", &pol_.gas_max, 0.0, 0.15, "gas %.0f%%", 100, nullptr);
  dirty_ |= slider("##nm", &pol_.nuclear_max, 0.0, 0.25, "nuclear %.0f%%", 100, nullptr);
  ImGui::TextUnformatted("Gas kept for peaking (share of fleet floor)");
  dirty_ |= slider("##gk", &pol_.gas_min_twh_share, 0.0, 1.0, "%.0f%%", 100,
                   "National gas generation >= this share of the existing gas fleet's best NPP year (FY2018-19 to "
                   "FY2025-26), kept for peaking and as a bridge for renewables. No gas additions are published, so "
                   "the gas cap is the fleet floor and each region's gas is at most its FY2023-24 share of it; at "
                   "100% every region is held at that share, below 100% the optimiser chooses where.");
  ImGui::TextUnformatted("Usable resource (solar / wind / hydro)");
  dirty_ |= slider("##sl", &pol_.solar_land, 0.05, 0.6, "solar %.0f%%", 100, "Fraction of solar generation potential usable by 2047.");
  dirty_ |= slider("##wl", &pol_.wind_land, 0.05, 0.6, "wind %.0f%%", 100, nullptr);
  dirty_ |= slider("##hf", &pol_.hydro_frac, 0.2, 1.0, "hydro %.0f%%", 100, nullptr);
  ImGui::TextUnformatted("Corridor capacity (x 2032 plan)");
  dirty_ |= slider("##ts", &pol_.transfer_scale, 0.5, 4.0, "%.1fx", 1, nullptr);
  ImGui::TextUnformatted("Build rate (published limits: case / multiplier)");
  for (int k = 0; k < kBuildCases; ++k) {
    if (k) ImGui::SameLine();
    dirty_ |= ImGui::RadioButton((std::string(build_case_name(k)) + "##bc").c_str(), &pol_.build_case, k);
    ImGui::SetItemTooltip("Low, central or high published additions per year (data/external/techno_economic/"
                          "build_limits.csv: CEA NEP 2022-32, NITI Aayog, DAE, MoP).");
  }
  // Lowest setting 0.75x: below about 0.71x the low case cannot reach the default 70% non-fossil
  // minimum next to the coal floor, and that row is hard (test default_policy_low_case_feasible).
  dirty_ |= slider("##ba", &pol_.build_accel, 0.75, 2.0, "%.2fx published limit", 1,
                   "2047 generation of each source <= max(existing-fleet floor, FY2023-24) + this x the published "
                   "additions to 2047 (GW per year x CUF x 8.76). The floor of coal, gas, nuclear and hydro is their "
                   "best NPP year FY2018-19 to FY2025-26. Solar, onshore wind and firmed RE share one limit.");
  ImGui::TextUnformatted("Carbon price");
  dirty_ |= slider("##cp", &pol_.carbon_price, 0.0, 100.0, "%.0f USD/t", 1, nullptr);
  if (ImGui::Button("Reset policy")) {
    pol_ = PolicySettings{};
    dirty_ = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("Reset scenario")) {
    sc_ = ScenarioControls{};
    draws_log2_ = kDefaultDrawsLog2;
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
  const float w = (ImGui::GetContentRegionAvail().x - 3 * ImGui::GetStyle().ItemSpacing.x) / 4;
  stat_card("2047 requirement (median)", with_commas(p50) + " TWh", "Energy generated, P50 of the ensemble", w);
  ImGui::SameLine();
  stat_card("90% range", with_commas(q[last][0]) + " - " + with_commas(q[last][6]), "P5 - P95, TWh", w);
  ImGui::SameLine();
  const bool india = pl_->core() == CoreModel::kIndia;
  char buf[320];
  std::snprintf(buf, sizeof buf, "%.2fx  (%.1f%%/yr)", p50 / h0.gen_twh,
                100 * (std::pow(p50 / h0.gen_twh, 1.0 / (f.years.back() - h0.year)) - 1));
  const std::string base_label = india ? "FY" + std::to_string(h0.year) + "-" + std::to_string((h0.year + 1) % 100)
                                       : std::to_string(h0.year);
  stat_card(("Growth vs " + base_label).c_str(), buf, ("Multiple of " + base_label + " generation, compound rate").c_str(),
            w);
  ImGui::SameLine();
  // The headline is the country's total in TWh, so the last card is the increase in TWh.
  std::snprintf(buf, sizeof buf, "%s%s TWh", p50 >= h0.gen_twh ? "+" : "", with_commas(p50 - h0.gen_twh).c_str());
  stat_card(("Increase over " + base_label).c_str(), buf,
            ("2047 median minus " + with_commas(h0.gen_twh) + " TWh generated in " + base_label).c_str(), w);
  if (india) help("Country total of utility generation, the scope of the National Power Portal (NPP) series.");

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
  if (ImPlot::BeginPlot(india ? "##national_india" : "##national_cc", ImVec2(-1, ImGui::GetContentRegionAvail().y * 0.80f))) {
    ImPlot::SetupAxes("Year", "Electricity generation (TWh)");
    ImPlot::SetupAxisLimits(ImAxis_X1, india ? 2004 : 1990, 2048, ImPlotCond_Once);
    ImPlot::SetupAxisLimits(ImAxis_Y1, 0, q[last][6] * 1.08, ImPlotCond_Always);
    ImPlot::SetupLegend(ImPlotLocation_NorthWest);
    ImPlot::SetNextFillStyle(kAccent, 0.18f);
    ImPlot::PlotShaded("90% range (P5-P95)", x.data(), p[0].data(), p[6].data(), static_cast<int>(x.size()));
    ImPlot::SetNextFillStyle(kAccent, 0.35f);
    ImPlot::PlotShaded("50% range (P25-P75)", x.data(), p[2].data(), p[4].data(), static_cast<int>(x.size()));
    ImPlot::SetNextLineStyle(kAccent, 3.0f);
    ImPlot::PlotLine("Median", x.data(), p[3].data(), static_cast<int>(x.size()));
    ImPlot::SetNextLineStyle(ImVec4(0.95f, 0.95f, 0.95f, 1), 2.5f);
    ImPlot::PlotLine(india ? "Observed (NPP)" : "Observed (Ember)", hx.data(), hy.data(), static_cast<int>(hx.size()));
    if (show_models_) {
      const ImVec4 cols[3] = {{0.95f, 0.65f, 0.30f, 1}, {0.45f, 0.85f, 0.55f, 1}, {0.85f, 0.45f, 0.65f, 1}};
      for (int m = 0; m < 3; ++m) {
        std::vector<double> cy(n + 1);
        cy[0] = h0.gen_twh;
        for (size_t t = 0; t < n; ++t) cy[t + 1] = f.central[m][t];
        ImPlot::SetNextLineStyle(cols[m], 1.5f);
        ImPlot::PlotLine((fit.model_names[m] + " (central)").c_str(), x.data(), cy.data(), static_cast<int>(cy.size()));
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
  if (india)
    help("Each scenario combines a population path (UN WPP 2024), an urban-share path (UN WUP 2018), an income path "
         "(real GSDP per person) with persistent and annual shocks, an income elasticity from a block bootstrap of "
         "India's own NPP and organiser data (two estimation windows), a literature urbanisation coefficient, and "
         "optional new loads. Starts from FY2023-24; a model-error term starts at that year's gap to the fitted line and fades out (see Models).");
  else
    help("Each scenario combines a population path (UN WPP 2024), an urban-share path (UN WUP 2018), a GDP-per-capita "
         "path with persistent and annual shocks, one of three models with bootstrap parameters, and optional new loads. "
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
  // Requirement share 2047 vs generation share in the base year (FY2023-24).
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
      static const char* items[2] = {"Generation FY2023-24", "Requirement 2047"};
      ImPlot::PlotBarGroups(items, vals.data(), 2, kRegions, 0.7);
      ImPlot::EndPlot();
    }
  }
  if (ImGui::BeginTable("regtable", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
    for (const char* hname : {"Region", "2047 P5", "2047 P50", "2047 P95", "Planned requirement", "Share 2047",
                              "Generation FY2023-24", "Growth"})
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
  help("The national total is split by each state's population and income (challenge dataset: population growing at "
       "the state's own 2019-2024 rate relative to India's; GSDP per person converging) through the same income "
       "response as the national model; states are placed in the grid regions NPP reports them under. It is not "
       "where power is generated: today the Western and Eastern regions generate more than their share and export "
       "to the North.");
}

void App::tab_mix() {
  const StrategyResult& s = cur_->strategy;
  const AllocationInputs& alloc = pl_->allocation_inputs();
  ImGui::RadioButton("Least-cost optimisation (LP)", &mix_method_, 0);
  ImGui::SameLine();
  ImGui::RadioButton("Resource-score allocation", &mix_method_, 1);
  const AllocationResult& a = mix_method_ == 0 ? s.least_cost : s.score;
  if (!s.least_cost.ok) {
    // No plan meets every hard constraint: say so instead of showing an earlier plan or zeros.
    // The score allocation shares the LP's national totals, so it has none either.
    if (font_big_) ImGui::PushFont(font_big_);
    ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Infeasible under these settings");
    if (font_big_) ImGui::PopFont();
    ImGui::TextWrapped("No 2047 generation mix meets every constraint at once, so no mix, cost or CO2 is shown "
                       "(solver status: %s).",
                       s.least_cost.status.c_str());
    help("Unmet requirement can relax only the regional balances; the non-fossil minimum, the coal floor, the gas "
         "kept for peaking, the firm generation floors and the build limits are hard. Raise the build-rate case or "
         "multiplier or the nuclear maximum, or lower the non-fossil minimum, the coal floor or the firm shares, or "
         "lower 'Gas kept for peaking' / raise the maximum gas share. At a low requirement (for example the Low "
         "growth P05) the coal floor and the gas kept for peaking, both fixed in TWh, can exceed the fossil share "
         "the non-fossil minimum allows.");
    return;
  }
  if (!a.ok) {
    ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Resource-score allocation not available: %s", a.status.c_str());
    return;
  }
  double unserved = 0;
  for (double u : a.unserved) unserved += u;
  const float w = (ImGui::GetContentRegionAvail().x - 3 * ImGui::GetStyle().ItemSpacing.x) / 4;
  char buf[64];
  std::snprintf(buf, sizeof buf, "%.1f%%", 100 * a.nonfossil_share());
  stat_card("Non-fossil share 2047", buf, "Of national generation", w);
  ImGui::SameLine();
  char ef[128];
  std::snprintf(ef, sizeof ef, "CEA FY%s: coal %.3f, gas %.3f t/MWh", alloc.emission_period.c_str(),
                alloc.emission_t_per_mwh[kCoal], alloc.emission_t_per_mwh[kGas]);
  stat_card("CO2 from generation", with_commas(a.co2_mt) + " Mt", ef, w);
  ImGui::SameLine();
  stat_card("Generation cost", with_commas(a.cost_busd) + " bn USD/yr", "2047 levelised costs (IEA WEO 2025 basis)", w);
  ImGui::SameLine();
  stat_card("Unmet requirement", with_commas(unserved, 1) + " TWh", unserved > 0.1 ? "Constraints too tight: relax a limit" : "System is adequate", w);

  const float ph = ImGui::GetContentRegionAvail().y * 0.48f;
  std::vector<const char*> labels(kTechs);
  for (int k = 0; k < kTechs; ++k) labels[k] = tech_label(k);
  // Stacked bars: national FY2023-24 (base year), national 2047, then five regions in 2047.
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
    static const char* glabels[groups] = {"India 2023-24", "India 2047", "North", "West", "South", "East", "NE"};
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

  // Storage and land back-calculated from the least-cost plan (recomputed with every strategy update).
  if (mix_method_ != 0) return;
  ImGui::SeparatorText("Storage and land this plan needs (hourly back-calculation)");
  const StorageLandResult& sl = s.storage;
  if (!sl.ok) {
    help(("Not computed: " + sl.status).c_str());
    return;
  }
  const StorageSizing& z = sl.total;
  const StorageInputs& si = pl_->storage_inputs();
  if (ImGui::BeginTable("storage", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchSame)) {
    for (const char* hname : {"Storage power", "Storage energy (daily cycle)", "Duration", "Curtailed solar and wind",
                              "Shortfall hours before storage", "Solar and wind land", "Share of India's area"})
      ImGui::TableSetupColumn(hname);
    ImGui::TableHeadersRow();
    ImGui::TableNextRow();
    char v[64], tip[512];
    auto cell = [&] {
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(v);
      ImGui::SetItemTooltip("%s", tip);
    };
    std::snprintf(v, sizeof v, "%s GW", with_commas(z.power_gw).c_str());
    std::snprintf(tip, sizeof tip,
                  "Charge and discharge rating. The largest hourly shortfall after hydro, coal and gas follow the load "
                  "is %s GW; the rating is raised above it for midday charging while each extra GW still serves an "
                  "hour a day of shortfall. Firmed RE is counted as solar and wind with its storage; with firmed RE as "
                  "round-the-clock supply the rest of the system needs %s GW.",
                  with_commas(z.discharge_gw).c_str(), with_commas(sl.system.power_gw).c_str());
    cell();
    std::snprintf(v, sizeof v, "%s GWh", with_commas(z.energy_day_gwh).c_str());
    std::snprintf(tip, sizeof tip,
                  "Smallest store that covers every shortfall the same day's surplus can cover, on the design day (%s); "
                  "the median day needs %s GWh. Carry-over between days adds energy only while each extra GWh cycles "
                  "at least weekly: %s GWh. CEA's 2029-30 optimal mix: %.1f GW, %.0f GWh.",
                  z.design_day.c_str(), with_commas(z.energy_day_median_gwh).c_str(),
                  with_commas(z.energy_multi_gwh).c_str(), si.bench_power_gw, si.bench_energy_gwh);
    cell();
    std::snprintf(v, sizeof v, "%.1f h", z.duration_h);
    std::snprintf(tip, sizeof tip, "Storage energy / storage power. About %.0f full cycles a year.", z.cycles);
    cell();
    std::snprintf(v, sizeof v, "%.1f%%", z.curtailed_day_pct());
    std::snprintf(tip, sizeof tip,
                  "Surplus solar and wind the daily store cannot take: %s of %s TWh. Still short hour by hour after "
                  "storage: %s TWh, of which storage losses %s TWh; the plan needs that much more generation.",
                  with_commas(z.curtailed_day_twh).c_str(), with_commas(z.vre_twh).c_str(),
                  with_commas(z.unmet_day_twh).c_str(), with_commas(z.losses_twh).c_str());
    cell();
    std::snprintf(v, sizeof v, "%s h/yr", with_commas(z.deficit_hours).c_str());
    std::snprintf(tip, sizeof tip, "Hours a year with load above supply after firm dispatch, before storage (%s TWh).",
                  with_commas(z.deficit_twh).c_str());
    cell();
    const LandUse& L = sl.land;
    std::snprintf(v, sizeof v, "%s km2", with_commas(L.total_km2()).c_str());
    std::snprintf(tip, sizeof tip,
                  "Solar %s km2 (4 acres/MW, MNRE, at the regional capacity factor; %s km2 at NREL's 3.4 acres per "
                  "GWh/yr) plus the wind turbine footprint %s km2. Wind farms also spread over %s km2 that mostly stays "
                  "in farming. Offshore wind uses no land. %.1f%% of India's wasteland.",
                  with_commas(L.solar_total()).c_str(), with_commas(L.solar_km2_high).c_str(),
                  with_commas(L.total_km2() - L.solar_total()).c_str(), with_commas(L.wind_total_area()).c_str(),
                  sl.pct_wasteland);
    cell();
    std::snprintf(v, sizeof v, "%.2f%%", sl.pct_india);
    std::snprintf(tip, sizeof tip,
                  "Of India's geographical area (ISFR 2023). By region: North %.2f%%, West %.2f%%, South %.2f%%, East "
                  "%.2f%%, North-East %.2f%%. Solar uses %.0f%% of the NISE ground-mounted generation potential.",
                  sl.pct_region[0], sl.pct_region[1], sl.pct_region[2], sl.pct_region[3], sl.pct_region[4],
                  100 * sl.solar_share_of_potential);
    cell();
    ImGui::EndTable();
  }
  char note[400];
  std::snprintf(note, sizeof note,
                "The plan is dispatched hour by hour over a year built from NPP Merit hourly generation (%zu complete "
                "days, %s to %s). Load follows the shape of total generation; nuclear and bioenergy run flat, hydro "
                "within each day's observed range, coal between %.0f%% and 100%% of its peak (%.2fx its mean), gas "
                "peaking. Round trip %.0f%%. Hover a value for details.",
                si.days.size(), si.days.front().date.c_str(), si.days.back().date.c_str(), 100 * si.coal_min_level,
                si.coal_peak_ratio, 100 * si.round_trip);
  help(note);
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
  origin = std::min(origin, static_cast<int>(fit.backtests.size()) - 1);
  const bool india = pl_->core() == CoreModel::kIndia;
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
    const ImVec4 cols[3] = {{0.95f, 0.65f, 0.30f, 1}, {0.45f, 0.85f, 0.55f, 1}, {0.85f, 0.45f, 0.65f, 1}};
    for (int k = 0; k < 3; ++k) {
      ImPlot::SetNextLineStyle(cols[k], 2.0f);
      ImPlot::PlotLine(fit.model_names[k].c_str(), x.data(), m[k].data(), static_cast<int>(x.size()));
    }
    const double ox = bt.origin;
    ImPlot::PlotInfLines("Fit ends", &ox, 1);
    ImPlot::EndPlot();
  }
  ImGui::SameLine();
  ImGui::BeginGroup();
  if (ImGui::BeginTable("bt", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
    ImGui::TableSetupColumn("Origin");
    for (int k = 0; k < 3; ++k) ImGui::TableSetupColumn(fit.model_names[k].c_str());
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
      ImGui::Text("  %d error", fit.history.back().year);
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
  help(india ? "Rolling origins FY2012-13 to FY2018-19; each fit uses FY2006-07 up to the origin and the observed "
               "drivers after it. Weights are not taken from the backtest (too few points); the damped trend is a "
               "benchmark."
             : "Weights are proportional to 1 / MAPE from the 2010 origin (15-year horizon). Pass bar: 2025 error below "
               "10%.");
  ImGui::EndGroup();
  if (india) {
    // India-only core: estimation table.
    const IndiaFit& a = fit.india_fit;
    const IndiaFit& b = fit.india_fit_full;
    const auto& bb = fit.india_beta_boot;
    if (ImGui::BeginTable("iparams", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
      for (const char* hname : {"Quantity", "Value", "Meaning"}) ImGui::TableSetupColumn(hname);
      ImGui::TableHeadersRow();
      auto row = [](const char* k, const std::string& v, const char* m) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(k);
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(v.c_str());
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(m);
      };
      char v[128];
      std::snprintf(v, sizeof v, "%.3f (s.e. %.3f)", a.beta, a.beta_se);
      row("Elasticity, FY2011-12 to FY2023-24", v, "ln(NPP generation per person) on ln(real GSDP per person), FY2020-21 dummy");
      std::snprintf(v, sizeof v, "%.3f (s.e. %.3f)", b.beta, b.beta_se);
      row("Elasticity, FY2006-07 to FY2023-24", v, "Same, across the 2011 GSDP price-base splice");
      std::snprintf(v, sizeof v, "%.3f / %.3f", a.fd_beta, b.fd_beta);
      row("First-difference elasticity", v, "Growth on growth, pandemic years left out (checks a spurious trend fit)");
      std::snprintf(v, sizeof v, "%.3f [%.3f, %.3f]", quantile(bb, 0.5), quantile(bb, 0.05), quantile(bb, 0.95));
      row("Elasticity used (median, 90%)", v, "Moving-block bootstrap (block 3), half from each window");
      std::snprintf(v, sizeof v, "%+.3f", a.covid);
      row("FY2020-21 effect", v, "Pandemic year, log points");
      std::snprintf(v, sizeof v, "%.3f / %.2f", a.resid_sd, a.dw);
      row("Residual s.d. / Durbin-Watson", v, "Serial correlation is why the bootstrap resamples blocks");
      std::snprintf(v, sizeof v, "%.3f", a.kappa);
      row("Urban share per unit ln income", v, "Historical co-movement absorbed by the elasticity");
      std::snprintf(v, sizeof v, "%.2f (s.d. %.2f)", fit.gamma_mean, fit.gamma_sd);
      row("Urbanisation coefficient (prior)", v, "Applies only to urbanisation beyond the income-implied path; literature");
      std::snprintf(v, sizeof v, "%+.3f / %.2f", a.resid.back(), a.resid_rho);
      row("Base-year residual / persistence", v, "Model error: AR(1) residual that starts at FY2023-24 and reverts to the fitted line");
      ImGui::EndTable();
    }
    help(("Urbanisation prior: " + fit.gamma_source + ". " + std::to_string(bb.size()) + " bootstrap replicates in " +
          with_commas(fit.bootstrap_ms, 1) + " ms.")
             .c_str());
    return;
  }

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
  } else if (!s.least_cost.ok) {
    ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1),
                       "Policy sensitivity not shown: the least-cost plan is infeasible under these settings (%s).",
                       s.least_cost.status.c_str());
  }
}

void App::tab_data() {
  // Three views: the sources and derivation notes, then the tables behind the midpoint panel's
  // questions on the requirement (drivers, states, new loads) and on the mix (published figures,
  // committed builds), computed for the current settings.
  if (ImGui::BeginTabBar("datatabs")) {
    static const char* names[3] = {"Sources and notes", "Requirement evidence", "Mix evidence"};
    for (int v = 0; v < 3; ++v) {
      const ImGuiTabItemFlags fl = force_data_view_ == v ? ImGuiTabItemFlags_SetSelected : 0;
      if (ImGui::BeginTabItem(names[v], nullptr, fl)) {
        ImGui::BeginChild("dataview");
        if (v == 0)
          data_sources();
        else if (v == 1)
          data_requirement();
        else
          data_mix();
        ImGui::EndChild();
        ImGui::EndTabItem();
      }
    }
    force_data_view_ = -1;
    ImGui::EndTabBar();
  }
}

void App::data_sources() {
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
        {"All-India generation by source, FY1946-47 to FY2023-24", "National Power Portal (challenge dataset)",
         "Primary forecast: history and elasticity; record annual increase (build-limit fallback and cross-check)"},
        {"State population and real GSDP, 2006-2024", "Renewable India dataset (challenge dataset; capacity columns not used)",
         "Primary forecast: income and population; regional split: state population paths and income convergence"},
        {"States by grid region", "National Power Portal state generation reports (dataset Zone where NPP lists no state)",
         "Regional split: state-to-region mapping"},
        {"Region / state conventional generation, monthly, Jul-2018 to Sep-2026", "National Power Portal (CEA 18-column reports)",
         "Holdout check FY2024-26; regional supply today; existing-fleet floors of the 2047 build caps (best year "
         "FY2018-19 to FY2025-26 of coal, gas, nuclear, hydro); state panel check FY2018-19 to FY2023-24 "
         "(diagnostic, not used)"},
        {"Hourly generation by source, Sep-2025 to Nov-2026", "National Power Portal (Merit India real-time)",
         "Solar and wind profiles, storage sizing"},
        {"Urbanisation coefficient", "Liddle 2013; Shahbaz et al. 2016; Holtedahl & Joutz 2004", "Literature prior"},
        {"Electricity generation, 153 countries, 1990-2025", "Ember yearly electricity data",
         "Independent check used for justification, not for estimation"},
        {"GDP per capita PPP, population, urban share", "World Bank WDI",
         "Independent check used for justification, not for estimation"},
        {"Region conventional generation by source, FY2023-24",
         "National Power Portal (CEA 18-column reports: region totals; station sums by state for coal / gas)",
         "Base mix and existing fleets: coal, gas, nuclear, hydro"},
        {"State renewable generation by type, FY2023-24", "CEA General Review 2025, Table 4.7 (secondary)",
         "Base mix: solar, wind, small hydro, bioenergy"},
        {"Regional generation by source", "Ember India; CEA General Review 2025, Table 4.4 (secondary)",
         "Cross-checks of the base mix only"},
        {"Population projections 2026-2047", "UN WPP 2024 (medium, low, high)",
         "National population paths; India-level growth on which the state paths are built"},
        {"State population projections 2011-2036", "Technical Group on Population Projections (2020)",
         "Reference column in the population consistency table only; not used"},
        {"Urban share projections", "UN WUP 2018", "National urbanisation path"},
        {"Solar, wind, hydro, bioenergy potential", "NISE 2025, NIWE 150 m, CEA reassessment, MNRE",
         "Upper bounds on generation; NISE 2014 and NIWE 120 m as mix sensitivities"},
        {"Capacity factors", "CEA NEP 2022-32; Ember observed", "Potential to generation conversion"},
        {"Costs 2047", "IEA WEO 2025; SECI auctions; CERC", "Least-cost optimisation"},
        {"CEA cost basis", "CEA NEP 2022-32 capital costs; CERC RE tariff order 2026 (WACC); CEA and CERC fuel; MoSPI GDP "
         "deflator, RBI 2024 rate and RBI CPI target (secondary)",
         "Mix sensitivity only, in real 2024 USD (cost_basis_cea; _pi2 and _pi6 at 2% and 6% inflation)"},
        {"Emission factors", "CEA CO2 Baseline Database v22 (FY2025-26; v21 kept as history)", "CO2 of each plan"},
        {"Inter-regional corridors", "CEA / NEP Vol II (2032 plan)", "Transfer limits and losses"},
        {"Nuclear pipeline", "DAE / NPCIL",
         "Nuclear siting key; committed floors (units under construction or sanctioned x CUF x 8.76)"},
        {"Large hydro under construction", "CEA hydro status (resource_potential.csv)",
         "Committed hydro floors (MW x CUF x 8.76)"},
        {"New loads in 2050 (vehicles, green hydrogen, data centres, cooling, cooking)",
         "NITI Aayog 2026; TERI 2024; CEEW 2025; BloombergNEF 2026; IEA 2018 (secondary)",
         "New-loads scenario outside the headline; only loads absent from the history are added"},
        {"Build limits by source, GW per year, FY2026-27 to FY2047-48 (low / central / high)",
         "CEA NEP 2022-32; CEA (Dec 2024); NITI Aayog (2026); DAE; MoP Annual Report 2025-26 (secondary)",
         "2047 build caps: GW per year x CUF x 8.76, summed from FY2024-25"},
        {"Generation projections 2026-27, 2031-32", "CEA NEP 2022-32", "Benchmark only"},
        {"Land-use intensity of solar and wind", "MNRE (PIB 2023); NREL (Ong et al. 2013; Denholm et al. 2009)",
         "Land back-calculation"},
        {"Storage round trip; coal minimum level", "NREL ATB 2025; CEA flexible operation regulations 2023",
         "Storage sizing (CEA 2029-30 optimal mix for comparison)"},
        {"Geographical area, wasteland", "ISFR 2023; Wastelands Atlas 2019", "Land as a share of India and its regions"},
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
  ImGui::Text("Allocation inputs by region (base FY2023-24 TWh / usable potential TWh / cost USD per MWh):");
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
  for (const auto& n : alloc.base_source_notes) ImGui::BulletText("%s", n.c_str());
  ImGui::BulletText("GDP per capita growth %d-%d: mean %.2f%%, s.d. %.2f%%; income convergence %.3f/yr (s.e. %.3f).",
                    d.g_hist_from, d.g_hist_to, 100 * d.g_hist_mean, 100 * d.g_hist_sd,
                    pl_->fitted().convergence.lambda, pl_->fitted().convergence.lambda_se);
}

namespace {

constexpr ImGuiTableFlags kTableFlags =
    ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;

bool begin_table(const char* id, std::initializer_list<const char*> heads, float height = 0) {
  const ImGuiTableFlags fl = kTableFlags | (height > 0 ? ImGuiTableFlags_ScrollY : 0);
  if (!ImGui::BeginTable(id, static_cast<int>(heads.size()), fl, ImVec2(0, height))) return false;
  if (height > 0) ImGui::TableSetupScrollFreeze(0, 1);
  for (const char* h : heads) ImGui::TableSetupColumn(h);
  ImGui::TableHeadersRow();
  return true;
}

void cell(const char* fmt, double v) {
  ImGui::TableNextColumn();
  if (std::isfinite(v))
    ImGui::Text(fmt, v);
  else
    ImGui::TextUnformatted("-");
}

void cell_text(const std::string& s) {
  ImGui::TableNextColumn();
  ImGui::TextWrapped("%s", s.c_str());
}

std::string fy(int y) {
  char b[16];
  std::snprintf(b, sizeof b, "FY%d-%02d", y, (y + 1) % 100);
  return b;
}

// Normalised state names ("andaman and nicobar islands") for display ("Andaman and Nicobar Islands").
std::string state_label(std::string s) {
  for (size_t i = 0; i < s.size(); ++i)
    if ((i == 0 || s[i - 1] == ' ') && s.compare(i, 4, "and ") != 0)
      s[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[i])));
  return s;
}

void heading(ImFont* f, const char* text) {
  ImGui::Spacing();
  if (f) ImGui::PushFont(f);
  ImGui::TextUnformatted(text);
  if (f) ImGui::PopFont();
}

}  // namespace

// The panel's questions on the requirement: what besides income drives it, how the national
// elasticity is applied state by state, why it is not estimated on states, and new loads.
void App::data_requirement() {
  const Job& j = *cur_;
  if (!j.evidence_error.empty())
    ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Some tables are not available: %s", j.evidence_error.c_str());
  if (!j.india) {
    help("The growth decomposition and the state table belong to the India-only core (the primary forecast). "
         "Switch the forecast core to India-only to see them.");
  }

  // 1. Growth decomposition of the central path.
  const GrowthDecomposition& g = j.growth;
  if (j.india && std::isfinite(g.total) && std::isfinite(g.income)) {
    heading(font_head_, "What drives the requirement besides income: growth of the central path by driver");
    help(("Central path " + fy(g.base_year) + " to " + fy(g.target_year) + ": " + with_commas(g.gen_base_twh) + " to " +
          with_commas(g.gen_target_twh) +
          " TWh under the current scenario settings (medium drivers, mean income growth). The parts are log points and "
          "add up to the total; shares are of the log growth of the central path, not of the median.")
             .c_str());
    if (begin_table("growth", {"Driver", "Log points", "Share of growth", "What it is"})) {
      const struct {
        const char* name;
        double v;
        const char* what;
      } rows[] = {
          {"Population", g.population, "UN WPP 2024 medium path from the base-year population"},
          {"Income", g.income, "income elasticity x change of ln GSDP per person (less the elasticity decline, if set)"},
          {"Urbanisation beyond income", g.urbanisation,
           "gamma x (change of urban share - kappa x change of ln income): the part income does not already imply"},
          {"Model error", g.model_error, "the high base-year residual fading (AR(1)); negative because FY2023-24 was high"},
          {"Total", g.total, "ln(central path in the target year / base year)"},
      };
      for (const auto& r : rows) {
        ImGui::TableNextRow();
        cell_text(r.name);
        cell("%+.3f", r.v);
        cell("%.1f%%", 100 * r.v / g.total);
        cell_text(r.what);
      }
      ImGui::EndTable();
    }
  }

  // 2. The national income response applied state by state.
  if (j.india && !j.states.empty()) {
    const DriverData& d = pl_->drivers();
    const ForecastResult& f = j.forecast;
    const size_t last = f.years.size() - 1;
    heading(font_head_, "State-wise: the national income response applied to each state's own income");
    char b[256];
    std::snprintf(b, sizeof b,
                  "%zu states and union territories of the Renewable India dataset; income relative to India from GSDP "
                  "and population (%d), converging at %.4f a year; the kernel sums the states into NPP's grid regions.",
                  j.states.size(), d.state_pop_year, pl_->fitted().convergence.lambda);
    help(b);
    std::array<double, kRegions> table{}, med{};
    double med_sum = 0;
    for (int r = 0; r < kRegions; ++r) {
      med[r] = f.quant.q[1 + r][last][3];
      med_sum += med[r];
    }
    const std::string ty = std::to_string(f.years[last]), by = std::to_string(d.state_pop_year);
    const std::string h_pop = "Population share " + by, h_ri0 = "Income vs India " + by, h_ri1 = "Income vs India " + ty,
                      h_idx = "Generation per person " + ty + " (India = 1)", h_req = "Requirement share " + ty;
    if (begin_table("states", {"State", "Grid region", h_pop.c_str(), h_ri0.c_str(), h_ri1.c_str(), h_idx.c_str(),
                               h_req.c_str()},
                    ImGui::GetTextLineHeightWithSpacing() * 13)) {
      for (const StateRow& s : j.states) {
        ImGui::TableNextRow();
        cell_text(state_label(s.state));
        cell_text(s.region + (s.region_from == "NPP" ? "" : " (dataset zone)"));
        cell("%.2f%%", 100 * s.pop_share_base);
        cell("%.2fx", std::exp(s.ln_rel_income_base));
        cell("%.2fx", std::exp(s.ln_rel_income_target));
        cell("%.2f", s.per_person_index_target);
        cell("%.2f%%", 100 * s.requirement_share_target);
        for (int r = 0; r < kRegions; ++r)
          if (s.region == region_name(r)) table[r] += s.requirement_share_target;
      }
      ImGui::EndTable();
    }
    std::string sums = "Regional sums of the state table | Monte Carlo regional medians:";
    for (int r = 0; r < kRegions; ++r) {
      std::snprintf(b, sizeof b, " %s %.1f%% | %.1f%%;", region_name(r), 100 * table[r],
                    med_sum > 0 ? 100 * med[r] / med_sum : 0.0);
      sums += b;
    }
    help(sums.c_str());
  }

  // 3. State panel check (read once; a diagnostic, never used).
  if (!panel_read_) {
    panel_read_ = true;
    try {
      panel_ = state_panel_data(root_.string(), &pl_->gsdp_basis());  // GSDP on constant 2011-12 prices
      panel_fit_ = fe_slope(panel_.obs);
    } catch (const std::exception& e) {
      panel_error_ = e.what();
    }
  }
  heading(font_head_, "Why not a state-wise elasticity: the state panel check (diagnostic, not used)");
  if (!panel_error_.empty()) {
    ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Not available: %s", panel_error_.c_str());
  } else {
    char b[512];
    std::string skipped;
    for (const std::string& s : panel_.skipped) skipped += (skipped.empty() ? "" : ", ") + state_label(s);
    std::snprintf(b, sizeof b,
                  "ln(NPP state generation per person) on ln(GSDP per person) with state fixed effects, %s to %s: "
                  "slope %.3f (s.e. %.3f), %d state-years, %d states, %d skipped (%s); the national elasticity is "
                  "%.3f. %.0f%% of the variance of ln generation per person lies between states. NPP records "
                  "generation where the plant stands (conventional stations only), so a state's figure measures where "
                  "plants are, not its demand: from %.2fx to %.2fx the national %.0f kWh per person.",
                  fy(panel_.first_year).c_str(), fy(panel_.last_year).c_str(), panel_fit_.slope, panel_fit_.se,
                  panel_fit_.n_obs, panel_fit_.n_states, panel_fit_.n_skipped, skipped.empty() ? "none" : skipped.c_str(),
                  pl_->fitted().india_fit.beta, 100 * panel_.between_share_ln_gen,
                  [&] {
                    double lo = 1e300;
                    for (const auto& s : panel_.spread)
                      if (s.ratio_to_national > 0) lo = std::min(lo, s.ratio_to_national);
                    return lo;
                  }(),
                  [&] {
                    double hi = 0;
                    for (const auto& s : panel_.spread) hi = std::max(hi, s.ratio_to_national);
                    return hi;
                  }(),
                  panel_.national_gen_pc_kwh);
    help(b);
    if (begin_table("panel", {"State", "Grid region (NPP)", "Years", "kWh per person", "x national"},
                    ImGui::GetTextLineHeightWithSpacing() * 9)) {
      for (const StateSpread& s : panel_.spread) {
        ImGui::TableNextRow();
        cell_text(state_label(s.state));
        cell_text(s.region);
        cell("%.0f", s.n_years);
        cell("%.0f", s.gen_pc_kwh);
        cell("%.2f", s.ratio_to_national);
      }
      ImGui::EndTable();
    }
  }

  // 4. Published new loads: a scenario outside the headline.
  heading(font_head_, "New loads (published estimates, outside the headline)");
  if (new_loads_.empty()) {
    help(new_loads_desc_.c_str());
  } else {
    help(new_loads_desc_.c_str());
    const int ty = new_loads_.front().target_year, py = new_loads_.front().year;
    const std::string lo = std::to_string(py) + " low / central / high", hi = std::to_string(ty) + " low / central / high";
    if (begin_table("loads", {"Load", "Overlaps the history", "Added", lo.c_str(), hi.c_str()})) {
      for (const NewLoad& l : new_loads_) {
        ImGui::TableNextRow();
        std::string name = l.load;
        std::replace(name.begin(), name.end(), '_', ' ');
        cell_text(name);
        cell_text(l.overlaps);
        cell_text(l.overlaps == "no" ? "yes" : "no (reported only)");
        char b[96];
        std::snprintf(b, sizeof b, "%.0f / %.0f / %.0f TWh", l.low_published, l.central_published, l.high_published);
        cell_text(b);
        std::snprintf(b, sizeof b, "%.0f / %.0f / %.0f TWh", l.low, l.central, l.high);
        cell_text(b);
      }
      for (const bool all : {false, true}) {
        const NewLoadTotals t = new_loads_total(new_loads_, all);
        ImGui::TableNextRow();
        cell_text(all ? "All loads (reference only, never added)" : "Add-on of the scenario");
        cell_text(all ? "any" : "no");
        cell_text(all ? "no" : "yes");
        cell_text("");
        char b[96];
        std::snprintf(b, sizeof b, "%.0f / %.0f / %.0f TWh", t.low, t.central, t.high);
        cell_text(b);
      }
      ImGui::EndTable();
    }
    help("End-use electricity, not grossed up for losses; the add-on assumes grid supply. Use the 'New loads' scenario "
         "button to run it.");
  }
}

// The panel's questions on the mix: how the published figures behind it move the plan, and
// which plants already being built set floors.
void App::data_mix() {
  const Job& j = *cur_;
  const AllocationInputs& alloc = pl_->allocation_inputs();
  const StrategyResult& s = j.strategy;
  if (!j.evidence_error.empty())
    ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Some tables are not available: %s", j.evidence_error.c_str());
  double planned = 0, p90 = 0;
  for (int r = 0; r < kRegions; ++r) {
    planned += s.requirement[r];
    p90 += j.requirement_p90[r];
  }

  // 1. Mix sensitivity to the published figures.
  heading(font_head_, "How the published figures move the mix: one change at a time");
  char b[1024];
  std::snprintf(b, sizeof b,
                "Each case is the current policy with one published figure changed, solved at the planned requirement "
                "(P%.0f, %s TWh) and at P90 (%s TWh). Costs are in each case's own basis. A case that leaves the "
                "requirement unmet describes less generation, so its CO2 and cost are not comparable.",
                100 * sc_.requirement_quantile, with_commas(planned).c_str(), with_commas(p90).c_str());
  help(b);
  // Solved only while this view is open, on a worker thread, and again only when the policy or
  // the requirement changes (CachedTask): the other views never wait for these 16 solves.
  const MixSensitivityKey key{&alloc, s.policy, s.requirement, j.requirement_p90};
  using MixState = CachedTask<MixSensitivityKey, MixEvidence>::State;
  const MixState ms = mix_.request(key, [key] {
    Timer t;
    MixEvidence e;
    e.rows = mix_sensitivity(*key.inputs, key.policy, key.requirement, key.requirement_p90);
    e.ms = t.ms();
    return e;
  });
  if (ms == MixState::kRunning)
    help("Solving the cases (two least-cost plans each, at the planned requirement and at P90)...");
  else if (ms == MixState::kFailed)
    ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Mix sensitivity not available: %s", mix_.error().c_str());
  const std::vector<MixSensitivityRow> no_rows;
  const std::vector<MixSensitivityRow>& mix_rows = ms == MixState::kReady ? mix_.value().rows : no_rows;
  if (!mix_rows.empty() &&
      begin_table("mixsens", {"Case", "Non-fossil", "Coal", "CO2 (Mt)", "Cost (bn USD/yr)", "Unmet, planned (TWh)",
                              "Unmet at P90 (TWh)", "What changes"})) {
    for (const MixSensitivityRow& m : mix_rows) {
      const AllocationResult& a = m.plan;
      ImGui::TableNextRow();
      cell_text(m.name);
      if (a.ok) {
        cell("%.1f%%", 100 * a.nonfossil_share());
        cell("%.1f%%", 100 * a.share(kCoal));
        cell("%.0f", a.co2_mt);
        cell("%.1f", a.cost_busd);
        cell("%.0f", a.unserved_total());
      } else {
        for (int k = 0; k < 5; ++k) cell_text(k == 0 ? "infeasible" : "");
      }
      if (m.plan_p90.ok)
        cell("%.0f", m.plan_p90.unserved_total());
      else
        cell_text("infeasible");
      cell_text(m.description);
    }
    ImGui::EndTable();
    help((std::to_string(2 * mix_rows.size()) + " least-cost plans solved in " + with_commas(mix_.value().ms, 0) +
          " ms when this view opened or its inputs last changed.")
             .c_str());
  }

  // 2. Committed builds and the gas kept for peaking.
  heading(font_head_, "Committed builds and gas kept for peaking (floors in TWh)");
  std::snprintf(b, sizeof b,
                "Floor = %s generation + committed MW x CUF x 8.76: nuclear units under construction, sanctioned or "
                "commissioned after %s (CUF %.2f); large hydro under construction (CUF %.2f). Committed TWh are cut "
                "to the national build cap and each region's upper bound. Gas: national minimum %.1f TWh (%.0f%% of "
                "the %.1f TWh fleet floor, %s), each region at most its %s share of the gas cap (held there when the "
                "minimum equals the cap). India's upper bound "
                "is the national limit: the build cap (nuclear: also the share limit) or the sum of the regional "
                "bounds, whichever is lower.",
                fy(alloc.base_year).c_str(), fy(alloc.base_year).c_str(), alloc.committed_nuclear_cuf,
                alloc.committed_hydro_cuf, gas_min_twh(alloc, s.policy), 100 * s.policy.gas_min_twh_share,
                alloc.fleet_floor_twh[kGas], fy(alloc.fleet_floor_year[kGas]).c_str(), fy(alloc.base_year).c_str());
  help(b);
  if (begin_table("committed", {"Source", "Region", "Base TWh", "Committed MW", "Committed TWh", "Floor TWh",
                                "Cut TWh", "Upper bound TWh", "Plan TWh"})) {
    const Bounds& bd = j.bounds;
    const AllocationResult& a = s.least_cost;
    for (int t : {kNuclear, kHydro, kGas}) {
      const auto* mw = t == kNuclear ? &alloc.committed_nuclear_mw : t == kHydro ? &alloc.committed_hydro_mw : nullptr;
      std::array<double, 7> sum{};
      for (int r = 0; r <= kRegions; ++r) {
        const bool total = r == kRegions;
        const std::array<double, 7> v =
            total ? sum
                  : std::array<double, 7>{alloc.base_twh[r][t], mw ? (*mw)[r] : 0.0, bd.committed[r][t], bd.lo[r][t],
                                          bd.committed_cut[r][t], bd.hi[r][t], a.ok ? a.gen[r][t] : kNaN};
        if (!total)
          for (size_t k = 0; k < v.size(); ++k) sum[k] += v[k];
        ImGui::TableNextRow();
        cell_text(r == 0 || total ? tech_label(t) : "");
        ImGui::TableNextColumn();
        if (total)
          ImGui::TextUnformatted("India");
        else
          ImGui::TextColored(kRegionColor[r], "%s", region_name(r));
        cell("%.1f", v[0]);
        cell("%.0f", v[1]);
        for (size_t k = 2; k < v.size(); ++k) {
          // India: the national upper bound, not the sum of the regional ones (nuclear's regional
          // bounds add up to more than its national limits allow).
          if (total && k == 5) {
            if (std::isfinite(j.national_hi[t]))
              cell("%.1f", j.national_hi[t]);
            else
              cell_text("-");
          } else {
            cell("%.1f", v[k]);
          }
        }
      }
    }
    ImGui::EndTable();
  }
  if (s.storage.ok && s.storage.total.gas_twh > 0) {
    const StorageSizing& z = s.storage.total;
    std::snprintf(b, sizeof b,
                  "Hourly check of this plan: gas runs %.0f hours a year, up to %.1f GW, with %.1f of its %.1f TWh "
                  "(%.0f%%) in the evening peak, 18:00-24:00 IST.",
                  z.gas_hours, z.gas_peak_gw, z.gas_evening_twh, z.gas_twh, 100 * z.gas_evening_twh / z.gas_twh);
    help(b);
  }
}

}  // namespace e47
