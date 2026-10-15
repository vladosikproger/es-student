add_library(device STATIC
    src/device.c
)

# ИМЕННО СЛОВО PUBLIC ПЕРЕДАЕТ ПУТЬ К ПАПКЕ INCLUDE В ГЛАВНЫЙ ПРОЕКТ
target_include_directories(device PUBLIC
    include
)

target_link_libraries(device PRIVATE
    pico_stdlib
    pico_unique_id
)

target_compile_definitions(device PRIVATE
    DEVICE_BOARD="pico"
)