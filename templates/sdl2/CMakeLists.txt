cmake_minimum_required(VERSION 3.20)
project({{PROJECT_ID}} CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(NXDev REQUIRED)

add_executable({{PROJECT_ID}} src/main.cpp)
target_link_libraries({{PROJECT_ID}} PRIVATE NXDev::Core NXDev::Sdl2)
