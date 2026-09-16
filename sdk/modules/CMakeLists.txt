# NXDevSDK Modules CMakeLists.txt

# ------------------------------------------------------------------------------
# Target: NXDev::Input
# ------------------------------------------------------------------------------
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

# ------------------------------------------------------------------------------
# Target: NXDev::Filesystem
# ------------------------------------------------------------------------------
add_library(nxdev_filesystem STATIC
    filesystem/src/filesystem.cpp
)
add_library(NXDev::Filesystem ALIAS nxdev_filesystem)
target_include_directories(nxdev_filesystem PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/filesystem/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(nxdev_filesystem PUBLIC NXDev::Core)
nxdev_configure_target(nxdev_filesystem)

# ------------------------------------------------------------------------------
# Target: NXDev::Account
# ------------------------------------------------------------------------------
add_library(nxdev_account STATIC
    account/src/account.cpp
)
add_library(NXDev::Account ALIAS nxdev_account)
target_include_directories(nxdev_account PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/account/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(nxdev_account PUBLIC NXDev::Core)
nxdev_configure_target(nxdev_account)

# ------------------------------------------------------------------------------
# Target: NXDev::Network
# ------------------------------------------------------------------------------
add_library(nxdev_network STATIC
    network/src/network.cpp
)
add_library(NXDev::Network ALIAS nxdev_network)
target_include_directories(nxdev_network PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/network/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(nxdev_network PUBLIC NXDev::Core)
nxdev_configure_target(nxdev_network)

# ------------------------------------------------------------------------------
# Target: NXDev::Time
# ------------------------------------------------------------------------------
add_library(nxdev_time STATIC
    time/src/time.cpp
)
add_library(NXDev::Time ALIAS nxdev_time)
target_include_directories(nxdev_time PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/time/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(nxdev_time PUBLIC NXDev::Core)
nxdev_configure_target(nxdev_time)

# ------------------------------------------------------------------------------
# Target: NXDev::Power
# ------------------------------------------------------------------------------
add_library(nxdev_power STATIC
    power/src/power.cpp
)
add_library(NXDev::Power ALIAS nxdev_power)
target_include_directories(nxdev_power PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/power/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(nxdev_power PUBLIC NXDev::Core)
nxdev_configure_target(nxdev_power)

# ------------------------------------------------------------------------------
# Target: NXDev::Display
# ------------------------------------------------------------------------------
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

# ------------------------------------------------------------------------------
# Target: NXDev::Borealis
# ------------------------------------------------------------------------------
add_library(nxdev_borealis STATIC
    borealis/src/view.cpp
    borealis/src/container.cpp
    borealis/src/label.cpp
    borealis/src/button.cpp
    borealis/src/image.cpp
    borealis/src/scroll_view.cpp
    borealis/src/dialog.cpp
    borealis/src/list.cpp
    borealis/src/theme.cpp
    borealis/src/application.cpp
)
add_library(NXDev::Borealis ALIAS nxdev_borealis)
target_include_directories(nxdev_borealis PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/borealis/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(nxdev_borealis PUBLIC NXDev::Core)
nxdev_configure_target(nxdev_borealis)

