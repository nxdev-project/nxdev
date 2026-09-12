cmake_minimum_required(VERSION 3.20)
project(MySwitchApp CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(NXDev REQUIRED)

add_executable(MySwitchApp src/main.cpp)
target_link_libraries(MySwitchApp PRIVATE NXDev::Core NXDev::Input NXDev::Display)
