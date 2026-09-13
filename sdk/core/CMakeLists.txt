add_library(nxdev_core STATIC
    src/app.cpp
    src/result.cpp
    src/log.cpp
    src/system.cpp
    src/version.cpp
)

add_library(NXDev::Core ALIAS nxdev_core)

target_include_directories(nxdev_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(TARGET NXDev::Libnx)
    target_link_libraries(nxdev_core PUBLIC NXDev::Libnx)
endif()

nxdev_configure_target(nxdev_core)
