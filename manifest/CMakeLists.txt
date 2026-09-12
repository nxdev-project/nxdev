add_library(nxdev_manifest_parser STATIC
    parser/src/manifest.cpp
    parser/src/parser.cpp
    parser/src/validator.cpp
    parser/src/diagnostics.cpp
    parser/src/yaml.cpp
)

add_library(NXDevAppManifest::Parser ALIAS nxdev_manifest_parser)

target_include_directories(nxdev_manifest_parser PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/parser/include>
    $<INSTALL_INTERFACE:include>
)

nxdev_configure_target(nxdev_manifest_parser)
