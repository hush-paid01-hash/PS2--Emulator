cmake_minimum_required(VERSION 3.16)
project(retroshelf_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(retroshelf-engine src/main.cpp src/cpu.cpp src/system_bus.cpp
    src/ee_gif_dma.cpp src/gs_packet_processor.cpp src/gs_register_decoder.cpp
    src/gs_register_state.cpp src/iso9660_disc.cpp src/ps2_boot_config.cpp
    src/ps2_disc_boot_loader.cpp src/elf32_mips.cpp src/ps2_boot_image.cpp
    src/elf_memory_staging.cpp src/elf_ram_loader.cpp)
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

add_executable(iso9660-disc-tests tests/iso9660_disc_tests.cpp src/iso9660_disc.cpp)
target_include_directories(iso9660-disc-tests PRIVATE include)
target_compile_options(iso9660-disc-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME iso9660-disc-tests COMMAND iso9660-disc-tests)

add_executable(ps2-boot-config-tests tests/ps2_boot_config_tests.cpp src/ps2_boot_config.cpp)
target_include_directories(ps2-boot-config-tests PRIVATE include)
target_compile_options(ps2-boot-config-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME ps2-boot-config-tests COMMAND ps2-boot-config-tests)

add_executable(ps2-disc-boot-loader-tests tests/ps2_disc_boot_loader_tests.cpp
    src/iso9660_disc.cpp src/ps2_boot_config.cpp src/ps2_disc_boot_loader.cpp)
target_include_directories(ps2-disc-boot-loader-tests PRIVATE include)
target_compile_options(ps2-disc-boot-loader-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME ps2-disc-boot-loader-tests COMMAND ps2-disc-boot-loader-tests)

add_executable(elf32-mips-tests tests/elf32_mips_tests.cpp src/elf32_mips.cpp)
target_include_directories(elf32-mips-tests PRIVATE include)
target_compile_options(elf32-mips-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME elf32-mips-tests COMMAND elf32-mips-tests)

add_executable(ps2-boot-image-tests tests/ps2_boot_image_tests.cpp
    src/iso9660_disc.cpp src/ps2_boot_config.cpp src/ps2_disc_boot_loader.cpp
    src/elf32_mips.cpp src/ps2_boot_image.cpp
    src/elf_memory_staging.cpp src/elf_ram_loader.cpp src/cpu.cpp)
target_include_directories(ps2-boot-image-tests PRIVATE include)
target_compile_options(ps2-boot-image-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME ps2-boot-image-tests COMMAND ps2-boot-image-tests)

add_executable(elf-memory-staging-tests tests/elf_memory_staging_tests.cpp
    src/elf_memory_staging.cpp)
target_include_directories(elf-memory-staging-tests PRIVATE include)
target_compile_options(elf-memory-staging-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME elf-memory-staging-tests COMMAND elf-memory-staging-tests)

add_executable(elf-ram-loader-tests tests/elf_ram_loader_tests.cpp
    src/elf_memory_staging.cpp src/elf_ram_loader.cpp)
target_include_directories(elf-ram-loader-tests PRIVATE include)
target_compile_options(elf-ram-loader-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME elf-ram-loader-tests COMMAND elf-ram-loader-tests)

add_executable(cpu-ram-fetch-tests tests/cpu_ram_fetch_tests.cpp src/cpu.cpp)
target_include_directories(cpu-ram-fetch-tests PRIVATE include)
target_compile_options(cpu-ram-fetch-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME cpu-ram-fetch-tests COMMAND cpu-ram-fetch-tests)

add_executable(cpu-boot-execution-tests tests/cpu_boot_execution_tests.cpp
    src/cpu.cpp src/elf_memory_staging.cpp src/elf_ram_loader.cpp)
target_include_directories(cpu-boot-execution-tests PRIVATE include)
target_compile_options(cpu-boot-execution-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME cpu-boot-execution-tests COMMAND cpu-boot-execution-tests)
