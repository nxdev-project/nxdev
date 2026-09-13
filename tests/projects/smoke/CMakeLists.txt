cmake_minimum_required(VERSION 3.20)
project(smoke-app CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(NXDev REQUIRED)

add_executable(smoke-app src/main.cpp)
target_link_libraries(smoke-app PRIVATE NXDev::Core NXDev::Input NXDev::Filesystem)
