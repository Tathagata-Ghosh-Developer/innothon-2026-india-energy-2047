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

#include "pipeline/pipeline.hpp"
#include "pipeline/presets.hpp"

namespace e47 {

class App {
 public:
  explicit App(std::filesystem::path root);
  ~App();
  void frame();
  bool ready() const { return pl_ && cur_ && !busy_ && !dirty_; }
  void force_tab(int t) { force_tab_ = t; }
  static constexpr int kTabCount = 6;
  static const char* tab_slug(int t);
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
    double total_ms = 0;
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
  void stat_card(const char* label, const std::string& value, const char* note, float width);

  std::filesystem::path root_;
  // One pipeline per forecast core: 0 = India-only (primary), 1 = cross-country (check).
  std::future<std::unique_ptr<Pipeline>> loading_[2];
  std::unique_ptr<Pipeline> pls_[2];
  Pipeline* pl_ = nullptr;  // pipeline of the active core (null while it loads)
  int core_ = 0;
  std::string error_;

  ScenarioControls sc_;
  PolicySettings pol_;
  bool dirty_ = true;
  std::future<Job> job_;
  std::shared_ptr<Job> cur_;
  bool busy_ = false;

  int draws_log2_ = 22;
  int mix_method_ = 0;     // 0 = least cost, 1 = resource score
  bool show_models_ = true;
  bool show_nep_ = true;
  int region_view_ = 0;
  int force_tab_ = -1;
  ImFont* font_body_ = nullptr;
  ImFont* font_head_ = nullptr;
  ImFont* font_big_ = nullptr;
};

}  // namespace e47
