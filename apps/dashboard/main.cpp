// India Energy 2047 dashboard: Dear ImGui + ImPlot on GLFW / OpenGL 3.
//
// Usage: e47_dashboard [repo_root]
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include "app.hpp"

namespace {

void glfw_error(int code, const char* desc) { std::fprintf(stderr, "glfw error %d: %s\n", code, desc); }

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
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
  GLFWwindow* window = glfwCreateWindow(1600, 960, "India Energy 2047", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return 1;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  float xs = 1.0f, ys = 1.0f;
  glfwGetWindowContentScale(window, &xs, &ys);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImPlot::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  // Segoe UI when available (Windows), otherwise the built-in font.
  ImFont* body = nullptr;
  ImFont* head = nullptr;
  ImFont* big = nullptr;
  const char* regular = "C:/Windows/Fonts/segoeui.ttf";
  const char* bold = "C:/Windows/Fonts/segoeuib.ttf";
  if (std::filesystem::exists(regular) && std::filesystem::exists(bold)) {
    body = io.Fonts->AddFontFromFileTTF(regular, 17.0f * xs);
    head = io.Fonts->AddFontFromFileTTF(bold, 21.0f * xs);
    big = io.Fonts->AddFontFromFileTTF(bold, 28.0f * xs);
  } else {
    ImFontConfig fc;
    fc.SizePixels = 15.0f * xs;
    body = io.Fonts->AddFontDefault(&fc);
  }
  apply_theme(xs);
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  e47::App app(root);
  app.set_fonts(body, head, big);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    app.frame();

    ImGui::Render();
    int w = 0, h = 0;
    glfwGetFramebufferSize(window, &w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0.06f, 0.07f, 0.09f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    if (!shot_dir.empty() && app.ready()) {
      if (shot_tab < 0) {
        shot_tab = 0;
        app.force_tab(0);
      } else if (++settle == 8) {
        save_bmp(shot_dir / (std::string(e47::App::tab_slug(shot_tab)) + ".bmp"), w, h);
        settle = 0;
        if (++shot_tab == e47::App::kTabCount) glfwSetWindowShouldClose(window, GLFW_TRUE);
        else app.force_tab(shot_tab);
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
}
