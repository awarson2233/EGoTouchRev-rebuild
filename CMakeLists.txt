cmake_minimum_required(VERSION 3.20)
project(EGoTouchRev VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- SIMD options ---
# This project targets a specific platform; enable NEON by default.
option(HIMAX_ENABLE_NEON "Enable ARM NEON intrinsics when available" ON)

# --- Common Library ---
# Use the source directory as the root for subprojects/resources so paths
# resolve correctly whether building in-tree or out-of-tree.
set(COMMON_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/Common")
set(IMGUI_ROOT "${COMMON_ROOT}/imgui-docking")

# Collect ImGui source files
file(GLOB IMGUI_SOURCES 
    "${IMGUI_ROOT}/imgui.cpp"
    "${IMGUI_ROOT}/imgui_demo.cpp"
    "${IMGUI_ROOT}/imgui_draw.cpp"
    "${IMGUI_ROOT}/imgui_tables.cpp"
    "${IMGUI_ROOT}/imgui_widgets.cpp"
)

file(GLOB COMMON_SOURCES "${COMMON_ROOT}/source/*.cpp")
file(GLOB COMMON_HEADERS "${COMMON_ROOT}/include/*.h")

add_subdirectory("${COMMON_ROOT}/spdlog-1.17.0" "spdlog_build")

add_library(Common STATIC
    ${COMMON_SOURCES}
    ${COMMON_HEADERS}
    ${IMGUI_SOURCES}
)

target_include_directories(Common PUBLIC
    "${COMMON_ROOT}/include"
    "${IMGUI_ROOT}"
)
target_link_libraries(Common PUBLIC spdlog::spdlog)

# --- Device Module (Hardware Abstraction) ---
set(DEVICE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/Device")
file(GLOB DEVICE_SOURCES "${DEVICE_ROOT}/source/*.cpp")
file(GLOB DEVICE_HEADERS "${DEVICE_ROOT}/include/*.h" "${DEVICE_ROOT}/include/*.hpp")

add_library(Device STATIC
    ${DEVICE_SOURCES}
    ${DEVICE_HEADERS}
)

target_compile_definitions(Device PUBLIC
    $<$<BOOL:${HIMAX_ENABLE_NEON}>:HIMAX_ENABLE_NEON=1>
    $<$<NOT:$<BOOL:${HIMAX_ENABLE_NEON}>>:HIMAX_ENABLE_NEON=0>
)

target_include_directories(Device PUBLIC 
    "${DEVICE_ROOT}/include"
)
target_link_libraries(Device PUBLIC Common)

# --- Engine Module (Touch Algorithm) ---
set(ENGINE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/Engine")
file(GLOB ENGINE_SOURCES "${ENGINE_ROOT}/source/*.cpp")
file(GLOB ENGINE_HEADERS "${ENGINE_ROOT}/include/*.h")

add_library(Engine STATIC
    Engine/source/MasterFrameParser.cpp
    Engine/source/BaselineSubtraction.cpp
    Engine/source/FramePipeline.cpp
    Engine/source/DynamicDeadzoneFilter.cpp
    Engine/source/SignalConditioningFilter.cpp
    Engine/source/GaussianFilter.cpp
    Engine/source/SpatialSharpenFilter.cpp
    Engine/source/CentroidExtractor.cpp
    ${ENGINE_HEADERS}
)

# ARM NEON is part of the standard ARM64 instruction set on MSVC.
# We ensure the defines are passed if conditionally required.
target_compile_definitions(Engine PUBLIC
    $<$<BOOL:${HIMAX_ENABLE_NEON}>:HIMAX_ENABLE_NEON=1>
    $<$<NOT:$<BOOL:${HIMAX_ENABLE_NEON}>>:HIMAX_ENABLE_NEON=0>
)

target_include_directories(Engine PUBLIC "${ENGINE_ROOT}/include")
target_link_libraries(Engine PUBLIC Common)

# --- Host Module (System Integration) ---
set(HOST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/Host")
file(GLOB HOST_SOURCES "${HOST_ROOT}/source/*.cpp")
file(GLOB HOST_HEADERS "${HOST_ROOT}/include/*.h")

add_library(Host STATIC
    ${HOST_SOURCES}
    ${HOST_HEADERS}
)
target_include_directories(Host PUBLIC "${HOST_ROOT}/include")
target_link_libraries(Host PUBLIC Common)

# --- App Module (Main & ServiceHost) ---
set(APP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/App")
file(GLOB APP_SOURCES "${APP_ROOT}/source/*.cpp")
file(GLOB APP_HEADERS "${APP_ROOT}/include/*.h")

# ImGui Backend sources (DX11 + Win32)
set(IMGUI_BACKEND_SOURCES 
    "${IMGUI_ROOT}/backends/imgui_impl_win32.cpp"
    "${IMGUI_ROOT}/backends/imgui_impl_dx11.cpp"
)

add_executable(EGoTouchApp
    ${APP_SOURCES}
    ${APP_HEADERS}
    ${IMGUI_BACKEND_SOURCES}
)

target_include_directories(EGoTouchApp PRIVATE
    "${APP_ROOT}/include"
    "${IMGUI_ROOT}"
    "${IMGUI_ROOT}/backends"
)

# Link D3D11 for the ImGui DX11 backend
target_link_libraries(EGoTouchApp PRIVATE Device Engine Host Common d3d11 d3dcompiler dwmapi)


