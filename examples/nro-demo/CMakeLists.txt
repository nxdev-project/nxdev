cmake_minimum_required(VERSION 3.20)

project(nro-demo LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT TARGET NXDev::Core)
    find_package(NXDev QUIET)
endif()

if(COMMAND nxdev_add_application)
    nxdev_add_application(nro-demo
        SOURCES
            src/main.cpp
        LIBRARIES
            NXDev::Core
            NXDev::Filesystem
            NXDev::Input
            NXDev::Time
    )
else()
    add_executable(nro-demo
        src/main.cpp
    )
    target_link_libraries(nro-demo PRIVATE
        NXDev::Core
        NXDev::Filesystem
        NXDev::Input
        NXDev::Time
    )
    if(COMMAND nxdev_configure_target)
        nxdev_configure_target(nro-demo)
    endif()
endif()
