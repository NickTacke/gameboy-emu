cmake_minimum_required(VERSION 3.10)
project(gameboy-emu)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(gameboy-emu src/main.cpp)