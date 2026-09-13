# hacBrewPack CMakeLists.txt

add_executable(nxdev_hacbrewpack
    src/main.cpp
)

set_target_properties(nxdev_hacbrewpack PROPERTIES
    OUTPUT_NAME hacbrewpack
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_executable(NXDevThirdParty::hacBrewPack ALIAS nxdev_hacbrewpack)
