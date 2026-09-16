cmake_minimum_required(VERSION 3.20)

project(ui-demo LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT TARGET NXDev::Core)
    find_package(NXDev REQUIRED)
endif()

if(COMMAND nxdev_add_application)
    nxdev_add_application(ui-demo
        SOURCES
            src/main.cpp
        LIBRARIES
            NXDev::Borealis
    )
else()
    add_executable(ui-demo
        src/main.cpp
    )
    target_link_libraries(ui-demo PRIVATE
        NXDev::Core
        NXDev::Borealis
    )
    if(COMMAND nxdev_configure_target)
        nxdev_configure_target(ui-demo)
    endif()
endif()
