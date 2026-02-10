
cmake_minimum_required(VERSION 3.20)

project(tempo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(tempo_lib src/runner.cpp)
target_include_directories(tempo_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(tempo_bench examples/main.cpp)
target_link_libraries(tempo_bench PRIVATE tempo_lib)

target_compile_options(tempo_lib PRIVATE -O3 -Wall -Wextra -Wpedantic)
target_compile_options(tempo_bench PRIVATE -O3 -Wall -Wextra -Wpedantic)

option(LANE_ASAN "Enable AddressSanitizer" OFF)
option(LANE_TSAN "Enable ThreadSanitizer" OFF)

if (LANE_ASAN)
  target_compile_options(tempo_lib PRIVATE -fsanitize=address -fno-omit-frame-pointer)
  target_link_options(tempo_lib PRIVATE -fsanitize=address)
  target_compile_options(tempo_bench PRIVATE -fsanitize=address -fno-omit-frame-pointer)
  target_link_options(tempo_bench PRIVATE -fsanitize=address)
endif()

if (LANE_TSAN)
  target_compile_options(tempo_lib PRIVATE -fsanitize=thread -fno-omit-frame-pointer)
  target_link_options(tempo_lib PRIVATE -fsanitize=thread)
  target_compile_options(tempo_bench PRIVATE -fsanitize=thread -fno-omit-frame-pointer)
  target_link_options(tempo_bench PRIVATE -fsanitize=thread)
endif()
