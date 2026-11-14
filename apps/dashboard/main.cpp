// India Energy 2047 dashboard: Dear ImGui + ImPlot on GLFW / OpenGL 3.
//
// Usage: e47_dashboard [repo_root]
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>

#include <cstdio>
#include <filesystem>
#include <string>

#include "app.hpp"

namespace {

void glfw_error(int code, const char* desc) { std::fprintf(stderr, "glfw error %d: %s\n", code, desc); }

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
  const std::filesystem::path root = argc > 1 ? std::filesystem::path(argv[1]) : std::filesystem::current_path();

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
  ImFontConfig fc;
  fc.SizePixels = 15.0f * xs;
  io.Fonts->AddFontDefault(&fc);
  apply_theme(xs);
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  e47::App app(root);

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
