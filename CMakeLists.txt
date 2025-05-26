cmake_minimum_required(VERSION 3.5)
project('sysprobe')
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDART 17)

# Вид сборки: Debug или Release
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Debug" FORCE)
endif()

set(COMPILE_FLAGS -std=c++17)
# Флаги компиляции для дебаг-сборки
if (CMAKE_BUILD_TYPE STREQUAL Debug)
    list(APPEND COMPILE_FLAGS -Wall -Wextra -Wpedantic 
    -g -fsanitize=address -fsanitize=leak)
    set(LINK_FLAGS -fsanitize=address) 
endif()

# Свойства сборки, которые будут передаваться всем остальным таргетам проекта
add_library(build_features INTERFACE)
target_compile_options(build_features INTERFACE ${COMPILE_FLAGS})
target_link_options(build_features INTERFACE ${LINK_FLAGS})
target_include_directories(build_features INTERFACE include)

include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)
target_link_libraries(build_features INTERFACE nlohmann_json::nlohmann_json)

add_library(probe_utilities STATIC ${CMAKE_SOURCE_DIR}/src/ProbeUtilities.cpp)

if (WIN32)
    target_sources(probe_utilities PUBLIC 
                  ${CMAKE_SOURCE_DIR}/src/ProbeUtilsImplWin.cpp)
elseif(UNIX)
    target_sources(probe_utilities PUBLIC 
                  ${CMAKE_SOURCE_DIR}/src/ProbeUtilsImplLinux.cpp)
else()
    message(FATAL_ERROR "Your platform isn't valid\n"
                        "List of supported platforms: \'Linux\', \'Windows\'")
endif()

target_link_libraries(probe_utilities PUBLIC build_features)

add_executable(main src/main.cpp)
target_link_libraries(main PUBLIC probe_utilities)
