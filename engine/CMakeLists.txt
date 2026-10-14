cmake_minimum_required(VERSION 3.16)
project(retroshelf_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(retroshelf-engine src/main.cpp src/cpu.cpp)
target_include_directories(retroshelf-engine PRIVATE include)
target_compile_options(retroshelf-engine PRIVATE -Wall -Wextra -Wpedantic)

enable_testing()
add_executable(cpu-tests tests/cpu_tests.cpp src/cpu.cpp)
target_include_directories(cpu-tests PRIVATE include)
target_compile_options(cpu-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME cpu-tests COMMAND cpu-tests)
