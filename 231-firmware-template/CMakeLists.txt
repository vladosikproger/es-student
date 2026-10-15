cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

project(231_firmware_template)
pico_sdk_init()

add_compile_options(-Wall)

# Подключаем внешнюю папку с библиотеками
add_subdirectory(../libs ${CMAKE_BINARY_DIR}/libs)

# Выключатель профилирования (1 - включено, 0 - выключено)
target_compile_definitions(profiling PUBLIC
    PROFILING_ENABLED=0
)

# Список файлов главной программы (остались только задачи самого прибора!)
add_executable(${PROJECT_NAME}
    main.c
    api/api.c
    led-task/led-task.c
    button-task/button-task.c
    pi-task/pi-task.c
)

# Пути поиска заголовков внутри самого проекта
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/api
    ${CMAKE_CURRENT_SOURCE_DIR}/led-task
    ${CMAKE_CURRENT_SOURCE_DIR}/button-task
    ${CMAKE_CURRENT_SOURCE_DIR}/pi-task
)

# Подключаем все 5 библиотек по именам
target_link_libraries(${PROJECT_NAME} PRIVATE
    pico_stdlib
    device
    logging
    profiling
    systime
    stdio-text-protocol
)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)
pico_add_extra_outputs(${PROJECT_NAME})