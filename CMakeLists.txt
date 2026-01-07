cmake_minimum_required(VERSION 3.20)
project(EGoTouchRev VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- SIMD options ---
# This project targets a specific platform; enable NEON by default.
option(HIMAX_ENABLE_NEON "Enable ARM NEON intrinsics when available" ON)

# --- Common Library ---
set(COMMON_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/Common")
file(GLOB COMMON_SOURCES "${COMMON_ROOT}/*.cpp")
file(GLOB COMMON_HEADERS "${COMMON_ROOT}/*.h")

add_library(Common STATIC
    ${COMMON_SOURCES}
    ${COMMON_HEADERS}
)

target_include_directories(Common PUBLIC
    "${COMMON_ROOT}"
)

# --- HimaxChipCore (Static Library) ---
# 定义源文件和头文件路径
set(HIMAX_CORE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/HimaxChipCore")
file(GLOB HIMAX_CORE_SOURCES "${HIMAX_CORE_ROOT}/source/*.cpp")
file(GLOB HIMAX_CORE_HEADERS "${HIMAX_CORE_ROOT}/header/*.h")
file(GLOB HIMAX_CORE_HEADERS_HPP "${HIMAX_CORE_ROOT}/header/*.hpp")

# 创建静态库
add_library(HimaxChipCore STATIC
    ${HIMAX_CORE_SOURCES}
    ${HIMAX_CORE_HEADERS}
    ${HIMAX_CORE_HEADERS_HPP}
)

# Define the switch for consumers; the header will still gate on actual arch macros.
target_compile_definitions(HimaxChipCore PUBLIC
    $<$<BOOL:${HIMAX_ENABLE_NEON}>:HIMAX_ENABLE_NEON=1>
    $<$<NOT:$<BOOL:${HIMAX_ENABLE_NEON}>>:HIMAX_ENABLE_NEON=0>
)

# 设置包含目录，PUBLIC 属性使得链接此库的目标也能看到这些头文件
target_include_directories(HimaxChipCore PUBLIC 
    "${HIMAX_CORE_ROOT}/header"
    "${COMMON_ROOT}"
)

# 链接 Common 库
target_link_libraries(HimaxChipCore PUBLIC Common)

# --- EGoTouchService (Main Service Executable) ---
# 定义源文件和头文件路径
set(SERVICE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/EGoTouchService")
file(GLOB SERVICE_SOURCES "${SERVICE_ROOT}/source/*.cpp")
file(GLOB SERVICE_HEADERS "${SERVICE_ROOT}/header/*.h")

# 创建可执行文件
add_executable(EGoTouchService
    ${SERVICE_SOURCES}
    ${SERVICE_HEADERS}
)

# 设置私有包含目录
target_include_directories(EGoTouchService PRIVATE
    "${SERVICE_ROOT}/header"
)

# 链接静态库
target_link_libraries(EGoTouchService PRIVATE HimaxChipCore)

