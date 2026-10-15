cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

project(231_firmware_template C CXX ASM)
pico_sdk_init()

# ПУНКТ 4: Строку add_compile_options(-Wall) ставим ДО add_subdirectory
add_compile_options(-Wall)

# ПУНКТ 4: libs лежит вне папки проекта: второй аргумент задаёт каталог сборки
add_subdirectory(../libs ${CMAKE_BINARY_DIR}/libs)

# ПУНКТ 5: Рубильник профилирования (1 - включено, 0 - выключено)
target_compile_definitions(profiling PUBLIC
    PROFILING_ENABLED=0
)

# Список файлов главной программы
add_executable(${PROJECT_NAME}
    main.c
    api/api.c
    stdio-text-protocol/stdio-text-protocol.c
    led-task/led-task.c
    button-task/button-task.c
    pi-task/pi-task.c
)

# Пути поиска заголовков внутри самого проекта
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/stdio-text-protocol
    ${CMAKE_CURRENT_SOURCE_DIR}/api
    ${CMAKE_CURRENT_SOURCE_DIR}/led-task
    ${CMAKE_CURRENT_SOURCE_DIR}/button-task
    ${CMAKE_CURRENT_SOURCE_DIR}/pi-task
)

# ПУНКТ 4: Библиотеки подключаются по именам
target_link_libraries(${PROJECT_NAME} PRIVATE
    pico_stdlib
    device
    logging
    profiling
    systime
)

# Стандартные настройки платформы Raspberry Pi Pico
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)
pico_add_extra_outputs(${PROJECT_NAME})