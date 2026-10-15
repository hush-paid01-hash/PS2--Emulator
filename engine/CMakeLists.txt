cmake_minimum_required(VERSION 3.16)
project(retroshelf_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(retroshelf-engine src/main.cpp src/cpu.cpp src/system_bus.cpp
    src/ee_gif_dma.cpp src/gs_packet_processor.cpp src/gs_register_decoder.cpp
    src/gs_register_state.cpp src/gs_command_processor.cpp src/gs_framebuffer.cpp)
target_include_directories(retroshelf-engine PRIVATE include)
target_compile_options(retroshelf-engine PRIVATE -Wall -Wextra -Wpedantic)

enable_testing()
add_executable(cpu-tests tests/cpu_tests.cpp src/cpu.cpp)
target_include_directories(cpu-tests PRIVATE include)
target_compile_options(cpu-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME cpu-tests COMMAND cpu-tests)

add_executable(ee-gif-dma-tests tests/ee_gif_dma_tests.cpp src/system_bus.cpp
    src/ee_gif_dma.cpp src/gs_packet_processor.cpp)
target_include_directories(ee-gif-dma-tests PRIVATE include)
target_compile_options(ee-gif-dma-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME ee-gif-dma-tests COMMAND ee-gif-dma-tests)

add_executable(system-bus-tests tests/system_bus_tests.cpp src/system_bus.cpp)
target_include_directories(system-bus-tests PRIVATE include)
target_compile_options(system-bus-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME system-bus-tests COMMAND system-bus-tests)

add_executable(gs-packet-processor-tests tests/gs_packet_processor_tests.cpp
    src/gs_packet_processor.cpp)
target_include_directories(gs-packet-processor-tests PRIVATE include)
target_compile_options(gs-packet-processor-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME gs-packet-processor-tests COMMAND gs-packet-processor-tests)

add_executable(gs-register-decoder-tests tests/gs_register_decoder_tests.cpp
    src/gs_register_decoder.cpp)
target_include_directories(gs-register-decoder-tests PRIVATE include)
target_compile_options(gs-register-decoder-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME gs-register-decoder-tests COMMAND gs-register-decoder-tests)

add_executable(gs-register-state-tests tests/gs_register_state_tests.cpp
    src/gs_register_state.cpp)
target_include_directories(gs-register-state-tests PRIVATE include)
target_compile_options(gs-register-state-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME gs-register-state-tests COMMAND gs-register-state-tests)

add_executable(gs-command-processor-tests tests/gs_command_processor_tests.cpp
    src/gs_command_processor.cpp src/gs_register_decoder.cpp src/gs_register_state.cpp)
target_include_directories(gs-command-processor-tests PRIVATE include)
target_compile_options(gs-command-processor-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME gs-command-processor-tests COMMAND gs-command-processor-tests)

add_executable(gs-framebuffer-tests tests/gs_framebuffer_tests.cpp src/gs_framebuffer.cpp)
target_include_directories(gs-framebuffer-tests PRIVATE include)
target_compile_options(gs-framebuffer-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME gs-framebuffer-tests COMMAND gs-framebuffer-tests)
