cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

set(PICO_BOARD pico_w CACHE STRING "Board type")

include(pico_sdk_import.cmake)

project(MedAlerta C CXX ASM)

pico_sdk_init()

add_executable(MedAlerta main.c display/ssd1306_i2c.c cjson/cJSON.c b64/cendode.c)

pico_set_program_name(MedAlerta "MedAlerta")
pico_set_program_version(MedAlerta "0.1")

pico_enable_stdio_uart(MedAlerta 0)
pico_enable_stdio_usb(MedAlerta 1)

target_include_directories(MedAlerta PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(
    MedAlerta
    pico_stdlib hardware_i2c hardware_pwm hardware_clocks pico_cyw43_arch_lwip_threadsafe_background
)

pico_add_extra_outputs(MedAlerta)
