# NXDevToolchainSwitch.cmake
# Helper module for detecting Nintendo Switch build environment when cross-compiling.
# Does NOT fail if devkitA64 / libnx is not present during host builds.

if(DEFINED ENV{DEVKITPRO})
    set(DEVKITPRO "$ENV{DEVKITPRO}" CACHE PATH "Path to devkitPro installation")
elseif(NOT DEFINED DEVKITPRO)
    if(EXISTS "/opt/devkitpro")
        set(DEVKITPRO "/opt/devkitpro" CACHE PATH "Path to devkitPro installation")
    endif()
endif()

# Check if we are targeting the Nintendo Switch
if(CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch" OR DEFINED __SWITCH__ OR TARGET_SWITCH)
    set(NXDEV_IS_SWITCH_TARGET TRUE CACHE INTERNAL "Building for Nintendo Switch")
    message(STATUS "Configuring NXDev for Nintendo Switch target platform")
else()
    set(NXDEV_IS_SWITCH_TARGET FALSE CACHE INTERNAL "Building for Host platform")
    message(STATUS "Configuring NXDev for Host platform (${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR})")
endif()
