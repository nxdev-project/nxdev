cmake_minimum_required(VERSION 3.20)
project({{PROJECT_ID}} C)

find_package(NXDev REQUIRED)

add_executable({{PROJECT_ID}} src/main.c)
target_link_libraries({{PROJECT_ID}} PRIVATE NXDev::Libnx)
