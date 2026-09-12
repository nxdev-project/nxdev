cmake_minimum_required(VERSION 3.20)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(NXDevVersion)
nxdev_extract_version()

project(NXDev
    VERSION ${NXDEV_VERSION_PLAIN}
    DESCRIPTION "Unofficial development ecosystem and SDK for Nintendo Switch homebrew"
    HOMEPAGE_URL "https://github.com/nxdev-project/nxdev"
    LANGUAGES C CXX
)

include(NXDevCompilerOptions)
include(NXDevToolchainSwitch)

# Component build options
option(NXDEV_BUILD_MANIFEST "Build NXDevAppManifest parser and schema library" ON)
option(NXDEV_BUILD_PACK "Build NXDevPack packaging toolkit" ON)
option(NXDEV_BUILD_SDK "Build NXDevSDK libraries" ON)
option(NXDEV_BUILD_CLI "Build NXDev unified command-line interface" ON)
option(NXDEV_BUILD_EXAMPLES "Build NXDev examples" ON)
option(NXDEV_BUILD_TESTS "Build NXDev test suite" ON)

# Configure output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Subdirectories
if(NXDEV_BUILD_MANIFEST)
    add_subdirectory(manifest)
endif()

if(NXDEV_BUILD_PACK)
    add_subdirectory(pack)
endif()

if(NXDEV_BUILD_SDK)
    add_subdirectory(sdk)
endif()

if(NXDEV_BUILD_CLI)
    add_subdirectory(cli)
endif()

if(NXDEV_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(NXDEV_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Summary
message(STATUS "--------------------------------------------------------")
message(STATUS "NXDev Configuration Summary:")
message(STATUS "  Version:            ${NXDEV_VERSION_STRING}")
message(STATUS "  Platform Target:    ${CMAKE_SYSTEM_NAME}")
message(STATUS "  C++ Standard:       C++20")
message(STATUS "  Build Manifest:     ${NXDEV_BUILD_MANIFEST}")
message(STATUS "  Build Pack:         ${NXDEV_BUILD_PACK}")
message(STATUS "  Build SDK:          ${NXDEV_BUILD_SDK}")
message(STATUS "  Build CLI:          ${NXDEV_BUILD_CLI}")
message(STATUS "  Build Examples:     ${NXDEV_BUILD_EXAMPLES}")
message(STATUS "  Build Tests:        ${NXDEV_BUILD_TESTS}")
message(STATUS "--------------------------------------------------------")
