add_library(profiling STATIC
    src/profiling.c
)

target_include_directories(profiling PUBLIC
    include
)

# Use the course's systime abstraction instead of calling Pico SDK timer APIs here.
target_link_libraries(profiling PRIVATE
    systime
)
