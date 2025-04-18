cmake_minimum_required(VERSION 3.2)
project('sysprobe')
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDART 17)

# Может быть или Windows, или Linux
if (NOT DEFINED SYSPROBE_SYSTEM_NAME)
    set(SYSPROBE_SYSTEM_NAME Windows)
endif()
message("Generating build files for \'${SYSPROBE_SYSTEM_NAME}\'...")

# Вид сборки: Debug или Release
if (NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(COMPILE_FLAGS -std=c++17)
# Флаги компиляции для дебаг-сборки
if (CMAKE_BUILD_TYPE STREQUAL Debug)
    list(APPEND COMPILE_FLAGS -Wall -Wextra -Wpedantic 
    -g -fsanitize=address -fsanitize=leak)
endif()

# Свойства сборки, которые будут передаваться всем остальным таргетам проекта
add_library(build_features INTERFACE)
target_compile_options(build_features INTERFACE ${COMPILE_FLAGS})
target_include_directories(build_features INTERFACE include)

include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)
target_link_libraries(build_features INTERFACE nlohmann_json::nlohmann_json)

add_library(probe_utilities STATIC src/ProbeUtilities.cpp)

# Коды TARGET_SYSTEM для условной компиляции:
# 0 - Windows
# 1 - Linux
if (SYSPROBE_SYSTEM_NAME STREQUAL Windows)
    target_sources(probe_utilities PUBLIC src/ProbeUtilsImplWin.cpp)
    target_compile_definitions(probe_utilities PRIVATE TARGET_SYSTEM=0)
elseif(SYSPROBE_SYSTEM_NAME STREQUAL Linux)
    target_sources(probe_utilities PUBLIC src/ProbeUtilsImplLinux.cpp)
    target_compile_definitions(probe_utilities PRIVATE TARGET_SYSTEM=1)
else()
    message(FATAL_ERROR "Target platform ${SYSPROBE_SYSTEM_NAME} isn't valid\n"
                        "List of supported platforms: \'Linux\', \'Windows\'")
endif()

target_link_libraries(probe_utilities PUBLIC build_features)
