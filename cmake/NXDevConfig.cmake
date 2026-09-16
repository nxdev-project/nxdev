# NXDevConfig.cmake
# NXDev CMake Package Configuration for Nintendo Switch Applications

cmake_minimum_required(VERSION 3.20)

if(TARGET NXDev::Libnx AND TARGET NXDev::Core)
    return()
endif()

# Resolve devkitPro root path if not already set
if(NOT DEFINED DEVKITPRO OR DEVKITPRO STREQUAL "")
    if(DEFINED ENV{DEVKITPRO} AND NOT "$ENV{DEVKITPRO}" STREQUAL "")
        set(DEVKITPRO "$ENV{DEVKITPRO}")
    elseif(EXISTS "/opt/devkitpro")
        set(DEVKITPRO "/opt/devkitpro")
    else()
        message(FATAL_ERROR "NXDev: DEVKITPRO not found! Please set the DEVKITPRO environment variable.")
    endif()
endif()

set(NXDEV_LIBNX_DIR "${DEVKITPRO}/libnx")
if(NOT EXISTS "${NXDEV_LIBNX_DIR}")
    message(FATAL_ERROR "NXDev: libnx directory not found at '${NXDEV_LIBNX_DIR}'!")
endif()

# ------------------------------------------------------------------------------
# Target: NXDev::Libnx
# Encapsulates libnx headers, architecture flags, linker specs, and system libraries
# ------------------------------------------------------------------------------
if(NOT TARGET NXDev::Libnx)
    add_library(NXDev::Libnx INTERFACE IMPORTED GLOBAL)

    target_include_directories(NXDev::Libnx INTERFACE
        "${NXDEV_LIBNX_DIR}/include"
    )

    target_compile_definitions(NXDev::Libnx INTERFACE
        __SWITCH__
    )

    target_compile_options(NXDev::Libnx INTERFACE
        -march=armv8-a+crc+crypto
        -mtune=cortex-a57
        -mcpu=cortex-a57
        -fPIE
        -ftls-model=local-exec
        -Wall
        -Wextra
    )

    target_link_options(NXDev::Libnx INTERFACE
        -fPIE
        -pie
        -specs=${NXDEV_LIBNX_DIR}/switch.specs
    )

    target_link_directories(NXDev::Libnx INTERFACE
        "${NXDEV_LIBNX_DIR}/lib"
    )

    target_link_libraries(NXDev::Libnx INTERFACE
        nx
        m
    )
endif()

# ------------------------------------------------------------------------------
# Locate NXDevSDK Root (Installed or Source-Tree)
# Precedence:
# 1. Explicit CMake override (NXDEV_SDK_ROOT variable)
# 2. NXDEV_SDK_ROOT environment variable
# 3. Installed Layout relative to share/nxdev/cmake (relocatable)
# 4. Standard platform installation path (/opt/nxdev or C:/NXDevSDK)
# 5. Source-Tree fallback
# ------------------------------------------------------------------------------
set(NXDEV_SDK_CORE_DIR "")
set(NXDEV_SDK_MODULES_DIR "")
set(NXDEV_SDK_INC_DIR "")
set(NXDEV_SDK_SRC_DIR "")

# 1. Check explicit CMake variable override
if(DEFINED NXDEV_SDK_ROOT AND EXISTS "${NXDEV_SDK_ROOT}/include/nxdev/nxdev.hpp")
    set(NXDEV_SDK_INSTALL_ROOT "${NXDEV_SDK_ROOT}")
    set(NXDEV_SDK_INC_DIR "${NXDEV_SDK_INSTALL_ROOT}/include")
    set(NXDEV_SDK_SRC_DIR "${NXDEV_SDK_INSTALL_ROOT}/src")
# 2. Check NXDEV_SDK_ROOT environment override
elseif(DEFINED ENV{NXDEV_SDK_ROOT} AND EXISTS "$ENV{NXDEV_SDK_ROOT}/include/nxdev/nxdev.hpp")
    set(NXDEV_SDK_INSTALL_ROOT "$ENV{NXDEV_SDK_ROOT}")
    set(NXDEV_SDK_INC_DIR "${NXDEV_SDK_INSTALL_ROOT}/include")
    set(NXDEV_SDK_SRC_DIR "${NXDEV_SDK_INSTALL_ROOT}/src")
# 3. Check Installed Layout relative to share/nxdev/cmake (relocatable)
elseif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../../include/nxdev/nxdev.hpp")
    get_filename_component(NXDEV_SDK_INSTALL_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
    set(NXDEV_SDK_INC_DIR "${NXDEV_SDK_INSTALL_ROOT}/include")
    set(NXDEV_SDK_SRC_DIR "${NXDEV_SDK_INSTALL_ROOT}/src")
# 4. Check standard platform installation path
elseif(EXISTS "/opt/nxdev/include/nxdev/nxdev.hpp")
    set(NXDEV_SDK_INSTALL_ROOT "/opt/nxdev")
    set(NXDEV_SDK_INC_DIR "${NXDEV_SDK_INSTALL_ROOT}/include")
    set(NXDEV_SDK_SRC_DIR "${NXDEV_SDK_INSTALL_ROOT}/src")
elseif(EXISTS "C:/NXDevSDK/include/nxdev/nxdev.hpp")
    set(NXDEV_SDK_INSTALL_ROOT "C:/NXDevSDK")
    set(NXDEV_SDK_INC_DIR "${NXDEV_SDK_INSTALL_ROOT}/include")
    set(NXDEV_SDK_SRC_DIR "${NXDEV_SDK_INSTALL_ROOT}/src")
# 5. Check Source-Tree layout
elseif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../sdk/core/include/nxdev/nxdev.hpp")
    set(NXDEV_SDK_CORE_DIR "${CMAKE_CURRENT_LIST_DIR}/../sdk/core")
    set(NXDEV_SDK_MODULES_DIR "${CMAKE_CURRENT_LIST_DIR}/../sdk/modules")
endif()

# ------------------------------------------------------------------------------
# Target: NXDev::Core
# High-level C++ abstraction SDK on top of libnx
# ------------------------------------------------------------------------------
if(NOT TARGET nxdev_core AND NOT TARGET NXDev::Core)
    if(NXDEV_SDK_CORE_DIR AND EXISTS "${NXDEV_SDK_CORE_DIR}/include/nxdev/nxdev.hpp")
        add_library(nxdev_core STATIC
            "${NXDEV_SDK_CORE_DIR}/src/app.cpp"
            "${NXDEV_SDK_CORE_DIR}/src/result.cpp"
            "${NXDEV_SDK_CORE_DIR}/src/log.cpp"
            "${NXDEV_SDK_CORE_DIR}/src/system.cpp"
            "${NXDEV_SDK_CORE_DIR}/src/version.cpp"
        )
        add_library(NXDev::Core ALIAS nxdev_core)
        target_include_directories(nxdev_core PUBLIC "${NXDEV_SDK_CORE_DIR}/include")
        target_link_libraries(nxdev_core PUBLIC NXDev::Libnx)
        target_compile_definitions(nxdev_core PUBLIC NXDEV_PLATFORM_SWITCH)
    elseif(NXDEV_SDK_INC_DIR AND EXISTS "${NXDEV_SDK_SRC_DIR}/core/app.cpp")
        add_library(nxdev_core STATIC
            "${NXDEV_SDK_SRC_DIR}/core/app.cpp"
            "${NXDEV_SDK_SRC_DIR}/core/result.cpp"
            "${NXDEV_SDK_SRC_DIR}/core/log.cpp"
            "${NXDEV_SDK_SRC_DIR}/core/system.cpp"
            "${NXDEV_SDK_SRC_DIR}/core/version.cpp"
        )
        add_library(NXDev::Core ALIAS nxdev_core)
        target_include_directories(nxdev_core PUBLIC "${NXDEV_SDK_INC_DIR}")
        target_link_libraries(nxdev_core PUBLIC NXDev::Libnx)
        target_compile_definitions(nxdev_core PUBLIC NXDEV_PLATFORM_SWITCH)
    endif()
endif()

# Helper macro for module targets
macro(nxdev_define_module_target TGT_NAME MOD_NAME)
    if(NOT TARGET nxdev_${MOD_NAME} AND NOT TARGET ${TGT_NAME})
        if(NXDEV_SDK_MODULES_DIR AND EXISTS "${NXDEV_SDK_MODULES_DIR}/${MOD_NAME}/src/${MOD_NAME}.cpp")
            add_library(nxdev_${MOD_NAME} STATIC "${NXDEV_SDK_MODULES_DIR}/${MOD_NAME}/src/${MOD_NAME}.cpp")
            add_library(${TGT_NAME} ALIAS nxdev_${MOD_NAME})
            target_include_directories(nxdev_${MOD_NAME} PUBLIC "${NXDEV_SDK_MODULES_DIR}/${MOD_NAME}/include")
            target_link_libraries(nxdev_${MOD_NAME} PUBLIC NXDev::Core)
        elseif(NXDEV_SDK_INC_DIR AND EXISTS "${NXDEV_SDK_SRC_DIR}/modules/${MOD_NAME}.cpp")
            add_library(nxdev_${MOD_NAME} STATIC "${NXDEV_SDK_SRC_DIR}/modules/${MOD_NAME}.cpp")
            add_library(${TGT_NAME} ALIAS nxdev_${MOD_NAME})
            target_include_directories(nxdev_${MOD_NAME} PUBLIC "${NXDEV_SDK_INC_DIR}")
            target_link_libraries(nxdev_${MOD_NAME} PUBLIC NXDev::Core)
        endif()
    endif()
endmacro()

nxdev_define_module_target(NXDev::Input input)
nxdev_define_module_target(NXDev::Filesystem filesystem)
nxdev_define_module_target(NXDev::Account account)
nxdev_define_module_target(NXDev::Network network)
nxdev_define_module_target(NXDev::Time time)
nxdev_define_module_target(NXDev::Power power)
nxdev_define_module_target(NXDev::Display display)

# ------------------------------------------------------------------------------
# External devkitPro Portlibs Targets (Cross-Compilation Isolated)
# ------------------------------------------------------------------------------
set(NXDEV_PORTLIBS_DIR "${DEVKITPRO}/portlibs/switch")

if(NOT TARGET NXDev::Portlibs)
    add_library(NXDev::Portlibs INTERFACE IMPORTED GLOBAL)
    if(EXISTS "${NXDEV_PORTLIBS_DIR}/include")
        target_include_directories(NXDev::Portlibs INTERFACE "${NXDEV_PORTLIBS_DIR}/include")
    endif()
    if(EXISTS "${NXDEV_PORTLIBS_DIR}/lib")
        target_link_directories(NXDev::Portlibs INTERFACE "${NXDEV_PORTLIBS_DIR}/lib")
    endif()
    target_link_libraries(NXDev::Portlibs INTERFACE NXDev::Libnx)
endif()

# NXDev::Zlib
if(NOT TARGET NXDev::Zlib)
    add_library(NXDev::Zlib INTERFACE IMPORTED GLOBAL)
    target_link_libraries(NXDev::Zlib INTERFACE NXDev::Portlibs z)
endif()

# NXDev::MbedTLS
if(NOT TARGET NXDev::MbedTLS)
    add_library(NXDev::MbedTLS INTERFACE IMPORTED GLOBAL)
    target_link_libraries(NXDev::MbedTLS INTERFACE NXDev::Portlibs mbedtls mbedcrypto mbedx509)
endif()

# NXDev::Curl
if(NOT TARGET NXDev::Curl)
    add_library(NXDev::Curl INTERFACE IMPORTED GLOBAL)
    target_link_libraries(NXDev::Curl INTERFACE NXDev::Portlibs curl NXDev::MbedTLS NXDev::Zlib)
endif()

# NXDev::Png
if(NOT TARGET NXDev::Png)
    add_library(NXDev::Png INTERFACE IMPORTED GLOBAL)
    target_link_libraries(NXDev::Png INTERFACE NXDev::Portlibs png NXDev::Zlib)
endif()

# NXDev::JpegTurbo
if(NOT TARGET NXDev::JpegTurbo)
    add_library(NXDev::JpegTurbo INTERFACE IMPORTED GLOBAL)
    target_link_libraries(NXDev::JpegTurbo INTERFACE NXDev::Portlibs jpeg)
endif()

# NXDev::FreeType
if(NOT TARGET NXDev::FreeType)
    add_library(NXDev::FreeType INTERFACE IMPORTED GLOBAL)
    if(EXISTS "${NXDEV_PORTLIBS_DIR}/include/freetype2")
        target_include_directories(NXDev::FreeType INTERFACE "${NXDEV_PORTLIBS_DIR}/include/freetype2")
    endif()
    target_link_libraries(NXDev::FreeType INTERFACE NXDev::Portlibs freetype NXDev::Png NXDev::Zlib)
endif()

# NXDev::Ogg
if(NOT TARGET NXDev::Ogg)
    add_library(NXDev::Ogg INTERFACE IMPORTED GLOBAL)
    target_link_libraries(NXDev::Ogg INTERFACE NXDev::Portlibs ogg)
endif()

# NXDev::Vorbis
if(NOT TARGET NXDev::Vorbis)
    add_library(NXDev::Vorbis INTERFACE IMPORTED GLOBAL)
    target_link_libraries(NXDev::Vorbis INTERFACE NXDev::Portlibs vorbisfile vorbis NXDev::Ogg)
endif()

# NXDev::Opus
if(NOT TARGET NXDev::Opus)
    add_library(NXDev::Opus INTERFACE IMPORTED GLOBAL)
    if(EXISTS "${NXDEV_PORTLIBS_DIR}/include/opus")
        target_include_directories(NXDev::Opus INTERFACE "${NXDEV_PORTLIBS_DIR}/include/opus")
    endif()
    target_link_libraries(NXDev::Opus INTERFACE NXDev::Portlibs opusfile opus NXDev::Ogg)
endif()

# NXDev::PhysFS
if(NOT TARGET NXDev::PhysFS)
    add_library(NXDev::PhysFS INTERFACE IMPORTED GLOBAL)
    target_link_libraries(NXDev::PhysFS INTERFACE NXDev::Portlibs physfs)
endif()

# NXDev::Deko3D
if(NOT TARGET NXDev::Deko3D)
    add_library(NXDev::Deko3D INTERFACE IMPORTED GLOBAL)
    target_link_libraries(NXDev::Deko3D INTERFACE NXDev::Libnx deko3d)
endif()

# NXDev::SDL2
if(NOT TARGET NXDev::SDL2)
    add_library(NXDev::SDL2 INTERFACE IMPORTED GLOBAL)
    if(EXISTS "${NXDEV_PORTLIBS_DIR}/include/SDL2")
        target_include_directories(NXDev::SDL2 INTERFACE "${NXDEV_PORTLIBS_DIR}/include/SDL2")
    endif()
    # SDL2 on Switch links EGL, glapi, drm_nouveau, nx, m
    target_link_libraries(NXDev::SDL2 INTERFACE
        NXDev::Portlibs
        SDL2
        EGL
        glapi
        drm_nouveau
        nx
        m
    )
endif()

# NXDev::SDL2Image
if(NOT TARGET NXDev::SDL2Image)
    add_library(NXDev::SDL2Image INTERFACE IMPORTED GLOBAL)
    target_link_libraries(NXDev::SDL2Image INTERFACE NXDev::SDL2 SDL2_image)
endif()

# NXDev::SDL2Mixer
if(NOT TARGET NXDev::SDL2Mixer)
    add_library(NXDev::SDL2Mixer INTERFACE IMPORTED GLOBAL)
    target_link_libraries(NXDev::SDL2Mixer INTERFACE NXDev::SDL2 SDL2_mixer)
endif()

# NXDev::SDL2TTF
if(NOT TARGET NXDev::SDL2TTF)
    add_library(NXDev::SDL2TTF INTERFACE IMPORTED GLOBAL)
    target_link_libraries(NXDev::SDL2TTF INTERFACE NXDev::SDL2 NXDev::FreeType SDL2_ttf)
endif()

# NXDev::Borealis
if(NOT TARGET nxdev_borealis AND NOT TARGET NXDev::Borealis)
    if(NXDEV_SDK_MODULES_DIR AND EXISTS "${NXDEV_SDK_MODULES_DIR}/borealis/src/application.cpp")
        add_library(nxdev_borealis STATIC
            "${NXDEV_SDK_MODULES_DIR}/borealis/src/view.cpp"
            "${NXDEV_SDK_MODULES_DIR}/borealis/src/container.cpp"
            "${NXDEV_SDK_MODULES_DIR}/borealis/src/label.cpp"
            "${NXDEV_SDK_MODULES_DIR}/borealis/src/button.cpp"
            "${NXDEV_SDK_MODULES_DIR}/borealis/src/image.cpp"
            "${NXDEV_SDK_MODULES_DIR}/borealis/src/scroll_view.cpp"
            "${NXDEV_SDK_MODULES_DIR}/borealis/src/dialog.cpp"
            "${NXDEV_SDK_MODULES_DIR}/borealis/src/list.cpp"
            "${NXDEV_SDK_MODULES_DIR}/borealis/src/theme.cpp"
            "${NXDEV_SDK_MODULES_DIR}/borealis/src/application.cpp"
        )
        add_library(NXDev::Borealis ALIAS nxdev_borealis)
        target_include_directories(nxdev_borealis PUBLIC "${NXDEV_SDK_MODULES_DIR}/borealis/include")
        target_link_libraries(nxdev_borealis PUBLIC NXDev::Core NXDev::Deko3D)
    elseif(NXDEV_SDK_INC_DIR AND EXISTS "${NXDEV_SDK_SRC_DIR}/modules/borealis/application.cpp")
        file(GLOB BOREALIS_INST_SRCS "${NXDEV_SDK_SRC_DIR}/modules/borealis/*.cpp")
        add_library(nxdev_borealis STATIC ${BOREALIS_INST_SRCS})
        add_library(NXDev::Borealis ALIAS nxdev_borealis)
        target_include_directories(nxdev_borealis PUBLIC "${NXDEV_SDK_INC_DIR}")
        target_link_libraries(nxdev_borealis PUBLIC NXDev::Core NXDev::Deko3D)
    endif()
endif()




# ------------------------------------------------------------------------------
# Function: nxdev_add_application(target_name [SOURCES ...])
# Creates a Nintendo Switch homebrew ELF executable target linked with NXDev / libnx
# ------------------------------------------------------------------------------
function(nxdev_add_application TARGET_NAME)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "SOURCES;LIBRARIES")

    if(NOT ARG_SOURCES)
        message(FATAL_ERROR "nxdev_add_application requires at least one source file in SOURCES")
    endif()

    add_executable(${TARGET_NAME} ${ARG_SOURCES})

    if(TARGET NXDev::Core)
        target_link_libraries(${TARGET_NAME} PRIVATE
            NXDev::Core
            ${ARG_LIBRARIES}
        )
    else()
        target_link_libraries(${TARGET_NAME} PRIVATE
            NXDev::Libnx
            ${ARG_LIBRARIES}
        )
    endif()

    set_target_properties(${TARGET_NAME} PROPERTIES
        OUTPUT_NAME "${TARGET_NAME}"
        SUFFIX ".elf"
    )

    if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
        set_target_properties(${TARGET_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        )
    endif()

    message(STATUS "Configured NXDev Switch Application Target: ${TARGET_NAME} (Output: ${TARGET_NAME}.elf)")
endfunction()

set(NXDev_FOUND TRUE)
