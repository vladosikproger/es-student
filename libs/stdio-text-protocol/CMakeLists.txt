add_library(stdio-text-protocol STATIC
    src/stdio-text-protocol.c
)

target_include_directories(stdio-text-protocol PUBLIC
    include
)

target_link_libraries(stdio-text-protocol PRIVATE
    pico_stdlib
)