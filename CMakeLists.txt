cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
set(CMAKE_C_COMPILER clang CACHE STRING "C compiler" FORCE)
set(CMAKE_CXX_COMPILER clang++ CACHE STRING "C++ compiler" FORCE)
project(gameboy-emu VERSION 0.1.0 DESCRIPTION "A GameBoy Emulator" LANGUAGES CXX)
# Add SDL2 package
find_package(SDL2 REQUIRED)

option(WITH_SANITIZERS "Enable Address and Undefined Behavior Sanitizers" OFF)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Export compile commands for IDEs and tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enforce C++20 and disable compiler extensions
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Warning flags per compiler
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Werror)
elseif(MSVC)
  add_compile_options(/W4 /WX)
endif()

# Collect all source files under src/
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.hpp"
)

# Declare executable target and link options
add_executable(gameboy-emu ${SOURCES})

target_include_directories(gameboy-emu PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
)

target_compile_features(gameboy-emu PRIVATE cxx_std_20)

# Optional sanitizers for debug purposes
if(WITH_SANITIZERS AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
  target_compile_options(gameboy-emu PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_libraries(gameboy-emu PRIVATE -fsanitize=address,undefined)
endif()

# Link SDL2 to gameboy-emu target
target_link_libraries(gameboy-emu PRIVATE SDL2::SDL2)