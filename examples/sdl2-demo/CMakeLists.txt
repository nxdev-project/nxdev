cmake_minimum_required(VERSION 3.20)
project(sdl2-demo LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(NXDev REQUIRED)

nxdev_add_application(sdl2-demo
    SOURCES
        src/main.cpp
    LIBRARIES
        NXDev::SDL2
)
