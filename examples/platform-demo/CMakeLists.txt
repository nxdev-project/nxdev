cmake_minimum_required(VERSION 3.20)

project(platform-demo LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT TARGET NXDev::Core)
    find_package(NXDev REQUIRED)
endif()

if(COMMAND nxdev_add_application)
    nxdev_add_application(platform-demo
        SOURCES
            src/main.cpp
        LIBRARIES
            NXDev::Input
            NXDev::Filesystem
            NXDev::Account
            NXDev::Network
            NXDev::Time
            NXDev::Power
    )
else()
    add_executable(platform-demo
        src/main.cpp
    )
    target_link_libraries(platform-demo PRIVATE
        NXDev::Core
        NXDev::Input
        NXDev::Filesystem
        NXDev::Account
        NXDev::Network
        NXDev::Time
        NXDev::Power
    )
    if(COMMAND nxdev_configure_target)
        nxdev_configure_target(platform-demo)
    endif()
endif()
