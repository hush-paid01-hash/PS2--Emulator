cmake_minimum_required(VERSION 3.16)
project(retroshelf_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(RETROSHELF_ENGINE_SOURCES
    src/cpu.cpp
    src/ee_execution_runner.cpp
    src/system_bus.cpp
    src/ee_gif_dma.cpp
    src/ee_gif_gs_runtime.cpp
    src/gs_packet_processor.cpp
    src/gs_register_decoder.cpp
    src/gs_register_state.cpp
    src/gs_framebuffer.cpp
    src/gs_software_rasterizer.cpp
    src/gs_packed_primitive_renderer.cpp
    src/gs_gif_pipeline.cpp
    src/iso9660_disc.cpp
    src/ps2_boot_config.cpp
    src/ps2_disc_boot_loader.cpp
    src/elf32_mips.cpp
    src/ps2_boot_image.cpp
    src/ps2_disc_execution.cpp
    src/elf_memory_staging.cpp
    src/elf_ram_loader.cpp
    src/ps2_ee_memory_map.cpp
    src/engine_c_api.cpp
    src/bios_image_validator.cpp
)

# Reusable C++ library for native frontends. The C ABI is intentionally small
# until the underlying engine can boot a BIOS and run a game end-to-end.
add_library(retroshelf_engine STATIC ${RETROSHELF_ENGINE_SOURCES})
target_include_directories(retroshelf_engine PUBLIC include)
target_compile_options(retroshelf_engine PRIVATE -Wall -Wextra -Wpedantic)

add_executable(retroshelf-engine src/main.cpp)
target_link_libraries(retroshelf-engine PRIVATE retroshelf_engine)
target_compile_options(retroshelf-engine PRIVATE -Wall -Wextra -Wpedantic)

enable_testing()

add_executable(engine-c-api-tests tests/engine_c_api_tests.cpp)
target_link_libraries(engine-c-api-tests PRIVATE retroshelf_engine)
target_compile_options(engine-c-api-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME engine-c-api-tests COMMAND engine-c-api-tests)

add_executable(cpu-tests tests/cpu_tests.cpp src/cpu.cpp src/ps2_ee_memory_map.cpp)
target_include_directories(cpu-tests PRIVATE include)
target_compile_options(cpu-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME cpu-tests COMMAND cpu-tests)

add_executable(ee-gif-dma-tests tests/ee_gif_dma_tests.cpp src/system_bus.cpp
    src/ee_gif_dma.cpp src/gs_packet_processor.cpp src/gs_gif_pipeline.cpp
    src/gs_register_decoder.cpp src/gs_register_state.cpp
    src/gs_packed_primitive_renderer.cpp src/gs_software_rasterizer.cpp
    src/gs_framebuffer.cpp)
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

add_executable(gs-gif-pipeline-tests tests/gs_gif_pipeline_tests.cpp
    src/gs_gif_pipeline.cpp src/gs_register_decoder.cpp src/gs_register_state.cpp
    src/gs_packed_primitive_renderer.cpp src/gs_software_rasterizer.cpp src/gs_framebuffer.cpp)
target_include_directories(gs-gif-pipeline-tests PRIVATE include)
target_compile_options(gs-gif-pipeline-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME gs-gif-pipeline-tests COMMAND gs-gif-pipeline-tests)

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
    src/elf32_mips.cpp src/ps2_boot_image.cpp src/ps2_disc_execution.cpp src/ee_execution_runner.cpp
    src/elf_memory_staging.cpp src/elf_ram_loader.cpp src/cpu.cpp src/ps2_ee_memory_map.cpp)
target_include_directories(ps2-boot-image-tests PRIVATE include)
target_compile_options(ps2-boot-image-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME ps2-boot-image-tests COMMAND ps2-boot-image-tests)

add_executable(elf-memory-staging-tests tests/elf_memory_staging_tests.cpp
    src/elf_memory_staging.cpp)
target_include_directories(elf-memory-staging-tests PRIVATE include)
target_compile_options(elf-memory-staging-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME elf-memory-staging-tests COMMAND elf-memory-staging-tests)

add_executable(elf-ram-loader-tests tests/elf_ram_loader_tests.cpp
    src/elf_memory_staging.cpp src/elf_ram_loader.cpp src/ps2_ee_memory_map.cpp)
target_include_directories(elf-ram-loader-tests PRIVATE include)
target_compile_options(elf-ram-loader-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME elf-ram-loader-tests COMMAND elf-ram-loader-tests)

add_executable(cpu-ram-fetch-tests tests/cpu_ram_fetch_tests.cpp src/cpu.cpp src/ps2_ee_memory_map.cpp)
target_include_directories(cpu-ram-fetch-tests PRIVATE include)
target_compile_options(cpu-ram-fetch-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME cpu-ram-fetch-tests COMMAND cpu-ram-fetch-tests)

add_executable(cpu-boot-execution-tests tests/cpu_boot_execution_tests.cpp
    src/cpu.cpp
    src/elf_memory_staging.cpp
    src/elf_ram_loader.cpp
    src/ps2_ee_memory_map.cpp)
target_include_directories(cpu-boot-execution-tests PRIVATE include)
target_compile_options(cpu-boot-execution-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME cpu-boot-execution-tests COMMAND cpu-boot-execution-tests)

add_executable(ps2-ee-memory-map-tests tests/ps2_ee_memory_map_tests.cpp src/ps2_ee_memory_map.cpp)
target_include_directories(ps2-ee-memory-map-tests PRIVATE include)
target_compile_options(ps2-ee-memory-map-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME ps2-ee-memory-map-tests COMMAND ps2-ee-memory-map-tests)

add_executable(elf-ee-memory-loader-tests tests/elf_ee_memory_loader_tests.cpp
    src/elf_memory_staging.cpp src/elf_ram_loader.cpp src/ps2_ee_memory_map.cpp)
target_include_directories(elf-ee-memory-loader-tests PRIVATE include)
target_compile_options(elf-ee-memory-loader-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME elf-ee-memory-loader-tests COMMAND elf-ee-memory-loader-tests)

add_executable(bios-image-validator-tests tests/bios_image_validator_tests.cpp
    src/bios_image_validator.cpp)
target_include_directories(bios-image-validator-tests PRIVATE include)
target_compile_options(bios-image-validator-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME bios-image-validator-tests COMMAND bios-image-validator-tests)

add_executable(ee-execution-runner-tests tests/ee_execution_runner_tests.cpp
    src/ee_execution_runner.cpp src/cpu.cpp src/ps2_ee_memory_map.cpp)
target_include_directories(ee-execution-runner-tests PRIVATE include)
target_compile_options(ee-execution-runner-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME ee-execution-runner-tests COMMAND ee-execution-runner-tests)

add_executable(gs-framebuffer-tests tests/gs_framebuffer_tests.cpp src/gs_framebuffer.cpp)
target_include_directories(gs-framebuffer-tests PRIVATE include)
target_compile_options(gs-framebuffer-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME gs-framebuffer-tests COMMAND gs-framebuffer-tests)

add_executable(gs-software-rasterizer-tests tests/gs_software_rasterizer_tests.cpp
    src/gs_software_rasterizer.cpp src/gs_framebuffer.cpp)
target_include_directories(gs-software-rasterizer-tests PRIVATE include)
target_compile_options(gs-software-rasterizer-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME gs-software-rasterizer-tests COMMAND gs-software-rasterizer-tests)

add_executable(gs-packed-primitive-renderer-tests tests/gs_packed_primitive_renderer_tests.cpp
    src/gs_packed_primitive_renderer.cpp src/gs_software_rasterizer.cpp src/gs_framebuffer.cpp)
target_include_directories(gs-packed-primitive-renderer-tests PRIVATE include)
target_compile_options(gs-packed-primitive-renderer-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME gs-packed-primitive-renderer-tests COMMAND gs-packed-primitive-renderer-tests)

add_executable(ee-gif-gs-runtime-tests tests/ee_gif_gs_runtime_tests.cpp
    src/system_bus.cpp src/ee_gif_dma.cpp src/ee_gif_gs_runtime.cpp
    src/ps2_ee_memory_map.cpp
    src/gs_packet_processor.cpp src/gs_register_decoder.cpp src/gs_register_state.cpp
    src/gs_framebuffer.cpp src/gs_software_rasterizer.cpp
    src/gs_packed_primitive_renderer.cpp src/gs_gif_pipeline.cpp)
target_include_directories(ee-gif-gs-runtime-tests PRIVATE include)
target_compile_options(ee-gif-gs-runtime-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME ee-gif-gs-runtime-tests COMMAND ee-gif-gs-runtime-tests)


add_executable(ee-gif-dma-mmio-tests tests/ee_gif_dma_mmio_tests.cpp)
target_link_libraries(ee-gif-dma-mmio-tests PRIVATE retroshelf_engine)
target_compile_options(ee-gif-dma-mmio-tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME ee-gif-dma-mmio-tests COMMAND ee-gif-dma-mmio-tests)
