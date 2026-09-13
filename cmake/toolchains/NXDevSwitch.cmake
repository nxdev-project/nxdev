# NXDevSwitch.cmake
# Nintendo Switch Cross-Compilation CMake Toolchain File
# Unofficial devkitA64 / libnx toolchain integration for NXDev

cmake_minimum_required(VERSION 3.20)

# Set Target Platform & Architecture
set(CMAKE_SYSTEM_NAME "Generic" CACHE STRING "Cross-compiling target system")
set(CMAKE_SYSTEM_PROCESSOR "aarch64" CACHE STRING "Target architecture")
set(CMAKE_SYSTEM_VERSION 1)
set(NXDEV_TARGET_SWITCH TRUE CACHE BOOL "Targeting Nintendo Switch")

# Resolve devkitPro root path
if(DEFINED NXDEV_DEVKITPRO AND NOT NXDEV_DEVKITPRO STREQUAL "")
    set(DEVKITPRO "${NXDEV_DEVKITPRO}" CACHE PATH "Path to devkitPro installation" FORCE)
elseif(DEFINED ENV{DEVKITPRO} AND NOT "$ENV{DEVKITPRO}" STREQUAL "")
    set(DEVKITPRO "$ENV{DEVKITPRO}" CACHE PATH "Path to devkitPro installation" FORCE)
elseif(NOT DEFINED DEVKITPRO OR DEVKITPRO STREQUAL "")
    if(EXISTS "/opt/devkitpro")
        set(DEVKITPRO "/opt/devkitpro" CACHE PATH "Path to devkitPro installation" FORCE)
    endif()
endif()

# Resolve devkitA64 root path
if(DEFINED NXDEV_DEVKITA64 AND NOT NXDEV_DEVKITA64 STREQUAL "")
    set(DEVKITA64 "${NXDEV_DEVKITA64}" CACHE PATH "Path to devkitA64 toolchain" FORCE)
elseif(DEFINED ENV{DEVKITA64} AND NOT "$ENV{DEVKITA64}" STREQUAL "")
    set(DEVKITA64 "$ENV{DEVKITA64}" CACHE PATH "Path to devkitA64 toolchain" FORCE)
elseif(DEFINED DEVKITPRO AND EXISTS "${DEVKITPRO}/devkitA64")
    set(DEVKITA64 "${DEVKITPRO}/devkitA64" CACHE PATH "Path to devkitA64 toolchain" FORCE)
endif()

if(NOT DEFINED DEVKITA64 OR NOT EXISTS "${DEVKITA64}")
    message(FATAL_ERROR "NXDev: devkitA64 not found! Set DEVKITA64 environment variable or NXDEV_DEVKITA64 CMake variable.")
endif()

# Locate Cross-Compiler Binaries
set(NXDEV_TOOLCHAIN_PREFIX "aarch64-none-elf-")
set(NXDEV_BIN_DIR "${DEVKITA64}/bin")

find_program(CMAKE_C_COMPILER   NAMES ${NXDEV_TOOLCHAIN_PREFIX}gcc     HINTS "${NXDEV_BIN_DIR}" NO_DEFAULT_PATH)
find_program(CMAKE_CXX_COMPILER NAMES ${NXDEV_TOOLCHAIN_PREFIX}g++     HINTS "${NXDEV_BIN_DIR}" NO_DEFAULT_PATH)
find_program(CMAKE_ASM_COMPILER NAMES ${NXDEV_TOOLCHAIN_PREFIX}gcc     HINTS "${NXDEV_BIN_DIR}" NO_DEFAULT_PATH)
find_program(CMAKE_AR           NAMES ${NXDEV_TOOLCHAIN_PREFIX}gcc-ar ${NXDEV_TOOLCHAIN_PREFIX}ar HINTS "${NXDEV_BIN_DIR}" NO_DEFAULT_PATH)
find_program(CMAKE_RANLIB       NAMES ${NXDEV_TOOLCHAIN_PREFIX}gcc-ranlib ${NXDEV_TOOLCHAIN_PREFIX}ranlib HINTS "${NXDEV_BIN_DIR}" NO_DEFAULT_PATH)
find_program(CMAKE_OBJCOPY      NAMES ${NXDEV_TOOLCHAIN_PREFIX}objcopy HINTS "${NXDEV_BIN_DIR}" NO_DEFAULT_PATH)
find_program(CMAKE_STRIP        NAMES ${NXDEV_TOOLCHAIN_PREFIX}strip   HINTS "${NXDEV_BIN_DIR}" NO_DEFAULT_PATH)

if(NOT CMAKE_C_COMPILER OR NOT CMAKE_CXX_COMPILER)
    message(FATAL_ERROR "NXDev: AArch64 GCC cross-compilers (${NXDEV_TOOLCHAIN_PREFIX}gcc / g++) not found in '${NXDEV_BIN_DIR}'!")
endif()

# Prevent CMake test compilation linking failures on bare-metal / newlib toolchains
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Target Architecture Flags (Nintendo Switch Tegra X1 Cortex-A57)
set(NXDEV_ARCH_FLAGS "-march=armv8-a+crc+crypto -mtune=cortex-a57 -mcpu=cortex-a57 -fPIE -ftls-model=local-exec")

set(CMAKE_C_FLAGS_INIT "${NXDEV_ARCH_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${NXDEV_ARCH_FLAGS}")
set(CMAKE_ASM_FLAGS_INIT "${NXDEV_ARCH_FLAGS}")

# Search Path Configuration
set(CMAKE_FIND_ROOT_PATH "${DEVKITPRO}" "${DEVKITA64}" "${DEVKITPRO}/libnx" "${DEVKITPRO}/portlibs/switch")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

message(STATUS "NXDev Switch Toolchain loaded: Target=AArch64 (devkitA64 at ${DEVKITA64})")
