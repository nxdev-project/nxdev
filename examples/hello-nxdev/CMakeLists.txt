cmake_minimum_required(VERSION 3.20)

project(hello-nxdev LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT TARGET NXDev::Core)
    find_package(NXDev REQUIRED)
endif()

if(COMMAND nxdev_add_application)
    nxdev_add_application(hello-nxdev
        SOURCES
            src/main.cpp
    )
else()
    add_executable(hello-nxdev
        src/main.cpp
    )
    target_link_libraries(hello-nxdev PRIVATE NXDev::Core)
    if(COMMAND nxdev_configure_target)
        nxdev_configure_target(hello-nxdev)
    endif()
endif()
