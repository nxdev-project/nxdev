add_library(nxdev_core STATIC
    src/app.cpp
    src/result.cpp
)

add_library(NXDev::Core ALIAS nxdev_core)

target_include_directories(nxdev_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

nxdev_configure_target(nxdev_core)
