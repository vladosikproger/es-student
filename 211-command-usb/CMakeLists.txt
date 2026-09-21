cmake_minimum_required(VERSION 3.13)

# Р СџР С•Р Т‘Р С”Р В»РЎР‹РЎвЂЎР В°Р ВµР С Pico SDK
include(pico_sdk_import.cmake)

project(211_command_usb)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/led/led.c
    ${CMAKE_CURRENT_SOURCE_DIR}/logging/log.c
    ${CMAKE_CURRENT_SOURCE_DIR}/device/device.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/led
    ${CMAKE_CURRENT_SOURCE_DIR}/logging
    ${CMAKE_CURRENT_SOURCE_DIR}/device
)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    pico_unique_id
)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    DEVICE_BOARD="${PICO_BOARD}"
)

pico_add_extra_outputs(${PROJECT_NAME})

pico_set_linker_script(${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/memmap_rp2040.ld
)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)