# NXDevSDK Modules

add_library(nxdev_input STATIC
    input/src/input.cpp
)
add_library(NXDev::Input ALIAS nxdev_input)
target_include_directories(nxdev_input PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/input/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(nxdev_input PUBLIC NXDev::Core)
nxdev_configure_target(nxdev_input)

add_library(nxdev_display STATIC
    display/src/display.cpp
)
add_library(NXDev::Display ALIAS nxdev_display)
target_include_directories(nxdev_display PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/display/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(nxdev_display PUBLIC NXDev::Core)
nxdev_configure_target(nxdev_display)
