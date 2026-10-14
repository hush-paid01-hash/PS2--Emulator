cmake_minimum_required(VERSION 3.16)
project(retroshelf_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(retroshelf-engine src/main.cpp src/cpu.cpp)
target_include_directories(retroshelf-engine PRIVATE include)
target_compile_options(retroshelf-engine PRIVATE -Wall -Wextra -Wpedantic)

add_library(retroshelf-ee-core src/ee_cpu.cpp src/ee_bus.cpp src/ee_machine.cpp src/ee_scheduler.cpp src/ee_interrupts.cpp src/ee_timers.cpp src/ee_hardware.cpp src/ee_dma.cpp src/ee_system_bus.cpp src/disc_image.cpp src/ps2_elf.cpp src/gs_vram.cpp src/gs_gif.cpp src/gs_gif_stream.cpp)
target_include_directories(retroshelf-ee-core PUBLIC include)
target_compile_options(retroshelf-ee-core PRIVATE -Wall -Wextra -Wpedantic)

enable_testing()
add_executable(cpu-tests tests/cpu_tests.cpp src/cpu.cpp)
target_include_directories(cpu-tests PRIVATE include)
target_compile_options(cpu-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME cpu-tests COMMAND cpu-tests)

add_executable(ee-cpu-tests tests/ee_cpu_tests.cpp)
target_link_libraries(ee-cpu-tests PRIVATE retroshelf-ee-core)
target_compile_options(ee-cpu-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME ee-cpu-tests COMMAND ee-cpu-tests)

add_executable(ee-bus-tests tests/ee_bus_tests.cpp)
target_link_libraries(ee-bus-tests PRIVATE retroshelf-ee-core)
target_compile_options(ee-bus-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME ee-bus-tests COMMAND ee-bus-tests)

add_executable(ee-machine-tests tests/ee_machine_tests.cpp)
target_link_libraries(ee-machine-tests PRIVATE retroshelf-ee-core)
target_compile_options(ee-machine-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME ee-machine-tests COMMAND ee-machine-tests)

add_executable(ee-scheduler-tests tests/ee_scheduler_tests.cpp)
target_link_libraries(ee-scheduler-tests PRIVATE retroshelf-ee-core)
target_compile_options(ee-scheduler-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME ee-scheduler-tests COMMAND ee-scheduler-tests)

add_executable(ee-interrupts-tests tests/ee_interrupts_tests.cpp)
target_link_libraries(ee-interrupts-tests PRIVATE retroshelf-ee-core)
target_compile_options(ee-interrupts-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME ee-interrupts-tests COMMAND ee-interrupts-tests)

add_executable(ee-timers-tests tests/ee_timers_tests.cpp)
target_link_libraries(ee-timers-tests PRIVATE retroshelf-ee-core)
target_compile_options(ee-timers-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME ee-timers-tests COMMAND ee-timers-tests)

add_executable(ee-hardware-tests tests/ee_hardware_tests.cpp)
target_link_libraries(ee-hardware-tests PRIVATE retroshelf-ee-core)
target_compile_options(ee-hardware-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME ee-hardware-tests COMMAND ee-hardware-tests)

add_executable(ee-dma-tests tests/ee_dma_tests.cpp)
target_link_libraries(ee-dma-tests PRIVATE retroshelf-ee-core)
target_compile_options(ee-dma-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME ee-dma-tests COMMAND ee-dma-tests)

add_executable(ee-system-bus-tests tests/ee_system_bus_tests.cpp)
target_link_libraries(ee-system-bus-tests PRIVATE retroshelf-ee-core)
target_compile_options(ee-system-bus-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME ee-system-bus-tests COMMAND ee-system-bus-tests)

add_executable(disc-image-tests tests/disc_image_tests.cpp)
target_link_libraries(disc-image-tests PRIVATE retroshelf-ee-core)
target_compile_options(disc-image-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME disc-image-tests COMMAND disc-image-tests)

add_executable(ps2-elf-tests tests/ps2_elf_tests.cpp)
target_link_libraries(ps2-elf-tests PRIVATE retroshelf-ee-core)
target_compile_options(ps2-elf-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME ps2-elf-tests COMMAND ps2-elf-tests)

add_executable(gs-vram-tests tests/gs_vram_tests.cpp)
target_link_libraries(gs-vram-tests PRIVATE retroshelf-ee-core)
target_compile_options(gs-vram-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME gs-vram-tests COMMAND gs-vram-tests)

add_executable(gs-gif-tests tests/gs_gif_tests.cpp)
target_link_libraries(gs-gif-tests PRIVATE retroshelf-ee-core)
target_compile_options(gs-gif-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME gs-gif-tests COMMAND gs-gif-tests)

add_executable(gs-gif-stream-tests tests/gs_gif_stream_tests.cpp)
target_link_libraries(gs-gif-stream-tests PRIVATE retroshelf-ee-core)
target_compile_options(gs-gif-stream-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME gs-gif-stream-tests COMMAND gs-gif-stream-tests)
