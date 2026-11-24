// India Energy 2047 dashboard: Dear ImGui + ImPlot on GLFW / OpenGL 3.
// The same file builds the WebAssembly version (Emscripten, WebGL 2); see
// docs/deployment.md.
//
// Usage: e47_dashboard [repo_root] [--shot output_dir]
#include <GLFW/glfw3.h>  // web build: GLFW_INCLUDE_ES3 (cmake/Dependencies.cmake) selects the GLES 3 header
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include "app.hpp"
#ifdef E47_WEBGPU
#include "forecast/mc_webgpu.hpp"

// Integer value of a query-string parameter of the page URL, or `fallback` when absent.
// ?gpu=0 keeps the Monte Carlo on the CPU; ?selftest=1 runs the WebGPU parity self-test.
EM_JS(int, e47_query_int, (const char* name, int fallback), {
  var v = new URLSearchParams(location.search).get(UTF8ToString(name));
  return v === null ? fallback : (parseInt(v, 10) || 0);
});
#endif

namespace {

void glfw_error(int code, const char* desc) { std::fprintf(stderr, "glfw error %d: %s\n", code, desc); }

#ifndef __EMSCRIPTEN__
// Writes the current back buffer as a 24-bit BMP (rows bottom-up, as OpenGL returns them).
void save_bmp(const std::filesystem::path& path, int w, int h) {
  std::vector<unsigned char> rgba(static_cast<size_t>(w) * h * 4);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
  const int row = (w * 3 + 3) & ~3;
  const uint32_t size = 54u + static_cast<uint32_t>(row) * h;
  unsigned char hdr[54] = {'B', 'M'};
  auto put32 = [&](int off, uint32_t v) {
    for (int i = 0; i < 4; ++i) hdr[off + i] = static_cast<unsigned char>(v >> (8 * i));
  };
  put32(2, size);
  put32(10, 54);
  put32(14, 40);
  put32(18, static_cast<uint32_t>(w));
  put32(22, static_cast<uint32_t>(h));
  hdr[26] = 1;
  hdr[28] = 24;
  put32(34, static_cast<uint32_t>(row) * h);
  std::FILE* f = std::fopen(path.string().c_str(), "wb");
  if (!f) return;
  std::fwrite(hdr, 1, 54, f);
  std::vector<unsigned char> line(row, 0);
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const unsigned char* p = &rgba[(static_cast<size_t>(y) * w + x) * 4];
      line[x * 3 + 0] = p[2];
      line[x * 3 + 1] = p[1];
      line[x * 3 + 2] = p[0];
    }
    std::fwrite(line.data(), 1, line.size(), f);
  }
  std::fclose(f);
}
#endif

void apply_theme(float scale) {
  ImGuiStyle& s = ImGui::GetStyle();
  ImGui::StyleColorsDark();
  s.WindowRounding = 6.0f;
  s.FrameRounding = 4.0f;
  s.GrabRounding = 4.0f;
  s.TabRounding = 4.0f;
  s.FramePadding = ImVec2(8, 5);
  s.ItemSpacing = ImVec2(8, 6);
  s.ScaleAllSizes(scale);
  ImVec4* c = s.Colors;
  c[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.09f, 0.11f, 1.0f);
  c[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.11f, 0.13f, 1.0f);
  c[ImGuiCol_Header] = ImVec4(0.18f, 0.32f, 0.48f, 0.8f);
  c[ImGuiCol_HeaderHovered] = ImVec4(0.22f, 0.40f, 0.60f, 0.9f);
  c[ImGuiCol_Tab] = ImVec4(0.14f, 0.16f, 0.20f, 1.0f);
  c[ImGuiCol_TabSelected] = ImVec4(0.18f, 0.36f, 0.56f, 1.0f);
  c[ImGuiCol_Button] = ImVec4(0.18f, 0.32f, 0.48f, 0.9f);
  c[ImGuiCol_SliderGrab] = ImVec4(0.35f, 0.62f, 0.90f, 1.0f);
  c[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.16f, 0.20f, 1.0f);
  ImPlot::StyleColorsDark();
  ImPlot::GetStyle().LineWeight = 2.0f;
}

// One frame: input, UI, draw. Returns the framebuffer size in w, h.
void draw_frame(GLFWwindow* window, e47::App& app, int& w, int& h) {
  glfwPollEvents();
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  app.frame();

  ImGui::Render();
  glfwGetFramebufferSize(window, &w, &h);
  glViewport(0, 0, w, h);
  glClearColor(0.06f, 0.07f, 0.09f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

#ifdef __EMSCRIPTEN__
// The browser drives the loop (one call per animation frame) and main() returns
// to the page, so the window and dashboard state live in globals.
GLFWwindow* g_window = nullptr;
e47::App* g_app = nullptr;

void web_frame() {
  int w = 0, h = 0;
  draw_frame(g_window, *g_app, w, h);
}
#endif

}  // namespace

int main(int argc, char** argv) {
  // Usage: e47_dashboard [repo_root] [--shot output_dir]
  std::filesystem::path root = std::filesystem::current_path();
  std::filesystem::path shot_dir;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a == "--shot" && i + 1 < argc)
      shot_dir = argv[++i];
    else
      root = a;
  }
  if (!shot_dir.empty()) std::filesystem::create_directories(shot_dir);
  int shot_tab = -1, settle = 0;

  glfwSetErrorCallback(glfw_error);
  if (!glfwInit()) return 1;
#ifdef __EMSCRIPTEN__
  // WebGL 2 (OpenGL ES 3.0). SCALE_TO_MONITOR: the framebuffer follows devicePixelRatio.
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
  const char* glsl_version = "#version 300 es";
#else
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
  const char* glsl_version = "#version 330";
#endif
  GLFWwindow* window = glfwCreateWindow(1600, 960, "India Energy 2047", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return 1;
  }
  glfwMakeContextCurrent(window);
#ifndef __EMSCRIPTEN__
  glfwSwapInterval(1);  // the browser paces frames with requestAnimationFrame
#endif

  float xs = 1.0f, ys = 1.0f;
  glfwGetWindowContentScale(window, &xs, &ys);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImPlot::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  ImFont* body = nullptr;
  ImFont* head = nullptr;
  ImFont* big = nullptr;
#ifdef __EMSCRIPTEN__
  // Layout is in CSS pixels and the framebuffer is xs (devicePixelRatio) times larger:
  // fonts are rasterised at device resolution and drawn at 1 / xs so text stays sharp.
  // Roboto (from the Dear ImGui sources) is packaged by cmake/Web.cmake.
  const float font_scale = xs, ui_scale = 1.0f;
  io.FontGlobalScale = 1.0f / xs;
  const char* regular = "/fonts/Roboto-Medium.ttf";
  const char* bold = regular;
#else
  // Segoe UI when available (Windows), otherwise the built-in font.
  const float font_scale = xs, ui_scale = xs;
  const char* regular = "C:/Windows/Fonts/segoeui.ttf";
  const char* bold = "C:/Windows/Fonts/segoeuib.ttf";
#endif
  if (std::filesystem::exists(regular) && std::filesystem::exists(bold)) {
    body = io.Fonts->AddFontFromFileTTF(regular, 17.0f * font_scale);
    head = io.Fonts->AddFontFromFileTTF(bold, 21.0f * font_scale);
    big = io.Fonts->AddFontFromFileTTF(bold, 28.0f * font_scale);
  } else {
    ImFontConfig fc;
    fc.SizePixels = 15.0f * font_scale;
    body = io.Fonts->AddFontDefault(&fc);
  }
  apply_theme(ui_scale);
  ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef __EMSCRIPTEN__
  // Resize the GLFW window with the page container (#wrap in web/shell.html) and
  // take mouse-wheel deltas from the browser.
  ImGui_ImplGlfw_InstallEmscriptenCallbacks(window, "#wrap");
#endif
  ImGui_ImplOpenGL3_Init(glsl_version);

#ifdef __EMSCRIPTEN__
  // No --shot capture in the browser. The page keeps running after main() returns.
  (void)shot_tab;
  (void)settle;
  g_window = window;
#ifdef E47_WEBGPU
  // The Monte Carlo runs on the visitor's GPU when the browser offers WebGPU; the set-up
  // is asynchronous and falls back to the CPU on any failure (forecast/mc_webgpu.hpp).
  if (e47_query_int("gpu", 1) != 0) e47::webgpu_init();
#endif
  g_app = new e47::App(root);  // root is "/" (the preloaded data package is mounted there)
  g_app->set_fonts(body, head, big);
#ifdef E47_WEBGPU
  g_app->set_selftest(e47_query_int("selftest", 0));
#endif
  emscripten_set_main_loop(web_frame, 0, true);
  return 0;
#else
  e47::App app(root);
  app.set_fonts(body, head, big);

  while (!glfwWindowShouldClose(window)) {
    int w = 0, h = 0;
    draw_frame(window, app, w, h);
    if (!shot_dir.empty() && app.ready()) {
      // Every tab, then the data tab's evidence views (App::kShotCount, App::shot_slug).
      if (shot_tab < 0) {
        shot_tab = 0;
        app.force_shot(0);
      } else if (++settle == 8) {
        save_bmp(shot_dir / (std::string(e47::App::shot_slug(shot_tab)) + ".bmp"), w, h);
        settle = 0;
        if (++shot_tab == e47::App::kShotCount) glfwSetWindowShouldClose(window, GLFW_TRUE);
        else app.force_shot(shot_tab);
      }
    }
    glfwSwapBuffers(window);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImPlot::DestroyContext();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
#endif
}
