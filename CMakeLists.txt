cmake_minimum_required(VERSION 3.16)
project(compresskit VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_compile_options(-Wall -Wextra -Werror)

# Shared codec library
add_library(compresskit_codec STATIC
    algorithms/shared/cpp/src/buffer_api.cpp
    algorithms/shared/cpp/src/cli_launcher.cpp
    algorithms/shared/cpp/src/frequency_table.cpp
)
target_include_directories(compresskit_codec
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/algorithms/shared/cpp/include>
)

# Algorithm executables
function(add_compresskit_algo name src)
    add_executable(${name} ${src})
    target_link_libraries(${name} PRIVATE compresskit_codec)
endfunction()

add_compresskit_algo(huffman_cpp algorithms/huffman/cpp/main.cpp)
add_compresskit_algo(arithmetic_cpp algorithms/arithmetic/cpp/main.cpp)
add_compresskit_algo(rangecoder_cpp algorithms/range/cpp/main.cpp)
add_compresskit_algo(rle_cpp algorithms/rle/cpp/main.cpp)
add_compresskit_algo(lzss_cpp algorithms/lzss/cpp/main.cpp)

# Tests
enable_testing()
add_executable(test_lifecycle
    algorithms/shared/cpp/tests/test_lifecycle.cpp
    algorithms/huffman/cpp/main.cpp
    algorithms/arithmetic/cpp/main.cpp
    algorithms/range/cpp/main.cpp
    algorithms/rle/cpp/main.cpp
    algorithms/lzss/cpp/main.cpp
)
target_link_libraries(test_lifecycle PRIVATE compresskit_codec)
target_compile_definitions(test_lifecycle PRIVATE COMPRESSKIT_NO_MAIN)
add_test(NAME lifecycle COMMAND test_lifecycle)
