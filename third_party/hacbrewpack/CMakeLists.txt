# hacBrewPack (gayhearts/hacBrewPack) CMakeLists.txt

file(GLOB HACBREWPACK_SOURCES
    "src/*.c"
)

file(GLOB MBEDTLS_SOURCES
    "src/mbedtls/library/*.c"
)

add_executable(nxdev_hacbrewpack
    ${HACBREWPACK_SOURCES}
    ${MBEDTLS_SOURCES}
)

target_include_directories(nxdev_hacbrewpack PRIVATE
    src
    src/mbedtls/include
)

target_compile_definitions(nxdev_hacbrewpack PRIVATE
    _BSD_SOURCE
    _POSIX_SOURCE
    _POSIX_C_SOURCE=200809L
    _DEFAULT_SOURCE
    _FILE_OFFSET_BITS=64
    __USE_MINGW_ANSI_STDIO=1
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(nxdev_hacbrewpack PRIVATE
        -std=gnu11
        -Wno-unused-result
        -Wno-implicit-fallthrough
        -Wno-enum-conversion
        -Wno-stringop-overflow
    )
endif()

set_target_properties(nxdev_hacbrewpack PROPERTIES
    OUTPUT_NAME hacbrewpack
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_executable(NXDevThirdParty::hacBrewPack ALIAS nxdev_hacbrewpack)
