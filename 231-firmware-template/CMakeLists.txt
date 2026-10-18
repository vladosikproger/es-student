cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

project(231_firmware_template)
pico_sdk_init()

add_compile_options(-Wall)

# Общие библиотеки курса
add_subdirectory(../libs ${CMAKE_BINARY_DIR}/libs)

# Профилирование включено в основной сборке приёмки.
# Для отдельной проверочной сборки достаточно заменить 1 на 0.
target_compile_definitions(profiling PUBLIC
    PROFILING_ENABLED=1
)

# Железо прибора: единственное место проекта с SDK
add_library(platform STATIC
    platform/platform.c
)

target_include_directories(platform PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(platform PRIVATE
    pico_stdlib
)

# Логика прибора: задачи и команды, без SDK
add_library(logic STATIC
    logic/api/api.c
    logic/api/api-callbacks.c
    logic/led-task/led-task.c
    logic/button-task/button-task.c
    logic/pi-task/pi-task.c
)

target_include_directories(logic PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/logic
)

target_link_libraries(logic PUBLIC
    stdio_text_protocol
)

target_link_libraries(logic PRIVATE
    platform
    device
    profiling
    systime
)

# Главная программа
add_executable(${PROJECT_NAME}
    main.c
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    pico_stdlib
    platform
    logic
    logging
    profiling
)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)
pico_add_extra_outputs(${PROJECT_NAME})