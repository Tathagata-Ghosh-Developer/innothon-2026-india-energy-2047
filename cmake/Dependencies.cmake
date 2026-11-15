# Third-party libraries, fetched at configure time and built from source.
include(FetchContent)
set(FETCHCONTENT_QUIET ON)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Eigen: header-only linear algebra. SOURCE_SUBDIR points at a missing folder so
# Eigen's own CMake project (tests, docs) is not added to the build.
FetchContent_Declare(eigen
  URL https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz
  SOURCE_SUBDIR do-not-configure
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(eigen)
add_library(e47_eigen INTERFACE)
target_include_directories(e47_eigen SYSTEM INTERFACE ${eigen_SOURCE_DIR})
target_compile_definitions(e47_eigen INTERFACE EIGEN_MPL2_ONLY)

# HiGHS: linear programming solver for the least-cost allocation cross-check.
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FAST_BUILD ON CACHE BOOL "" FORCE)
set(ZLIB OFF CACHE BOOL "" FORCE)
if(EMSCRIPTEN)
  # Single-threaded solver in the browser (each extra thread would be a web worker),
  # and no stand-alone highs executable.
  set(HIGHS_NO_DEFAULT_THREADS ON CACHE BOOL "" FORCE)
  set(BUILD_CXX_EXE OFF CACHE BOOL "" FORCE)
else()
  set(HIGHS_NO_DEFAULT_THREADS OFF CACHE BOOL "" FORCE)
endif()
FetchContent_Declare(highs
  URL https://github.com/ERGO-Code/HiGHS/archive/refs/tags/v1.11.0.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(highs)

if(BUILD_DASHBOARD)
  FetchContent_Declare(imgui
    URL https://github.com/ocornut/imgui/archive/refs/tags/v1.91.9.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
  FetchContent_Declare(implot
    URL https://github.com/epezent/implot/archive/refs/tags/v0.16.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
  FetchContent_MakeAvailable(imgui implot)
  if(NOT EMSCRIPTEN)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(glfw
      URL https://github.com/glfw/glfw/archive/refs/tags/3.4.tar.gz
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(glfw)
    find_package(OpenGL REQUIRED)
  endif()

  add_library(e47_imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${implot_SOURCE_DIR}/implot.cpp
    ${implot_SOURCE_DIR}/implot_items.cpp)
  target_include_directories(e47_imgui SYSTEM PUBLIC
    ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends ${implot_SOURCE_DIR})
  if(EMSCRIPTEN)
    # Emscripten's built-in GLFW 3 on a WebGL 2 context; the ImGui OpenGL3
    # backend then uses the GLES 3 API ("#version 300 es" shaders).
    target_compile_definitions(e47_imgui PUBLIC IMGUI_IMPL_OPENGL_ES3 GLFW_INCLUDE_ES3)
    target_link_options(e47_imgui PUBLIC -sUSE_GLFW=3 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2)
  else()
    target_link_libraries(e47_imgui PUBLIC glfw OpenGL::GL)
  endif()
endif()
