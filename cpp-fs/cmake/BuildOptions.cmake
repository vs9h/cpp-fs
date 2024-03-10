set(DEBUG_BUILD_OPTIONS
    -Wall
    -Wextra
    -Wshadow
    -Werror
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Wunused
    -Woverloaded-virtual
    -Wpedantic
    -Wconversion
    -Wsign-conversion
    -Wnull-dereference
    -Wdouble-promotion
    -Wformat=2
    -Wimplicit-fallthrough
    -Wmisleading-indentation
    -g
    -O0
)

add_compile_options("$<$<CONFIG:Debug>:${DEBUG_BUILD_OPTIONS}>")

set(RELEASE_BUILD_OPTIONS
    -O3
    -Wall
    -Wextra
    -Werror
    -Wpedantic
)

add_compile_options("$<$<CONFIG:Release>:${RELEASE_BUILD_OPTIONS}>")

set(ASAN_BUILD_OPTIONS
    -O0
    -Wall
    -Wextra
    -Werror
    -Wpedantic
    -fsanitize=address
    -fno-omit-frame-pointer
    -fno-optimize-sibling-calls
)

set(ASAN_LINK_OPTIONS
    -fsanitize=address
)

add_compile_options("$<$<CONFIG:ASAN>:${ASAN_BUILD_OPTIONS}>")
add_link_options("$<$<CONFIG:ASAN>:${ASAN_LINK_OPTIONS}>")
