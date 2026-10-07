cmake_minimum_required(VERSION 4.1.2)
project(tinydict LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wall -Wextra -Wpedantic -O3)
endif()

add_library(tinydict
        src/dict.c
        src/hash.c
        src/exception.c
)

target_include_directories(tinydict PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
