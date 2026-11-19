#pragma once
// Dashboard state and views. Heavy work (pipeline setup, Monte Carlo, LP) runs
// on a background thread; the UI always draws the latest finished result.
#include <imgui.h>

#include <chrono>
#include <filesystem>
#include <future>
#include <memory>
#include <string>
#include <vector>

#include "core/cached_task.hpp"
#include "forecast/state_panel.hpp"
#include "pipeline/pipeline.hpp"
#include "pipeline/presets.hpp"

namespace e47 {

class App {
 public:
  explicit App(std::filesystem::path root);
  ~App();
  void frame();
  bool ready() const { return pl_ && cur_ && !busy_ && !dirty_ && !mix_.busy(); }
  void force_tab(int t) { force_tab_ = t; }
  static constexpr int kTabCount = 6;
  static const char* tab_slug(int t);
  // Screenshots (--shot): every tab, then the data tab's requirement and mix evidence views.
  static constexpr int kShotCount = kTabCount + 2;
  static const char* shot_slug(int k);
  void force_shot(int k);
  void set_fonts(ImFont* body, ImFont* heading, ImFont* big) {
    font_body_ = body;
    font_head_ = heading;
    font_big_ = big;
  }

 private:
  struct Job {
    int core = 0;
    ForecastResult forecast;
    StrategyResult strategy;
    // Evidence tables of the data tab, for the same settings (India-only core for the first two).
    bool india = false;
    GrowthDecomposition growth;             // central path to the target year, by driver
    std::vector<StateRow> states;           // the regional split state by state, target year
    std::array<double, kRegions> requirement_p90{};
    Bounds bounds;                          // committed nuclear and hydro floors, gas bounds
    std::array<double, kTechs> national_hi{};  // national upper bound by source (national_upper_bound_twh)
    std::string evidence_error;             // why a table above is missing
    double total_ms = 0;
  };
  // The mix sensitivity (12 LP solves): computed only while the mix evidence view is open, on a
  // worker thread, and again only when the inputs change.
  struct MixEvidence {
    std::vector<MixSensitivityRow> rows;  // the current policy with one published figure changed
    double ms = 0;
  };

  void poll();
  void launch();
  void request_core(int core);
  void sidebar();
  void header();
  void tab_forecast();
  void tab_regions();
  void tab_mix();
  void tab_models();
  void tab_sensitivity();
  void tab_data();
  void data_sources();
  void data_requirement();
  void data_mix();
  void stat_card(const char* label, const std::string& value, const char* note, float width);

  std::filesystem::path root_;
  // One pipeline per forecast core: 0 = India-only (primary), 1 = cross-country (check).
  std::future<std::unique_ptr<Pipeline>> loading_[2];
  std::unique_ptr<Pipeline> pls_[2];
  Pipeline* pl_ = nullptr;  // pipeline of the active core (null while it loads)
  int core_ = 0;
  int pending_core_ = -1;   // core being loaded after a switch (-1 = none)
  std::string error_;

  ScenarioControls sc_;
  PolicySettings pol_;
  // Published new loads of the active pipeline, for the new-loads preset (read once per pipeline).
  const Pipeline* new_loads_for_ = nullptr;
  std::vector<NewLoad> new_loads_;
  std::string new_loads_desc_;
  bool dirty_ = true;
  std::future<Job> job_;
  std::shared_ptr<Job> cur_;
  bool busy_ = false;
  CachedTask<MixSensitivityKey, MixEvidence> mix_;  // reads a pipeline's inputs: declared after pls_

  int draws_log2_ = kDefaultDrawsLog2;
  int mix_method_ = 0;     // 0 = least cost, 1 = resource score
  bool show_models_ = true;
  bool show_nep_ = true;
  int region_view_ = 0;
  int force_tab_ = -1;
  int force_data_view_ = -1;  // data tab: 0 sources and notes, 1 requirement evidence, 2 mix evidence
  // State panel check (a diagnostic on the two NPP and dataset files), read once.
  bool panel_read_ = false;
  StatePanelData panel_;
  PanelFit panel_fit_;
  std::string panel_error_;
  ImFont* font_body_ = nullptr;
  ImFont* font_head_ = nullptr;
  ImFont* font_big_ = nullptr;
};

}  // namespace e47
