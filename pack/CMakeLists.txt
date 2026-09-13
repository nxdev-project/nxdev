# NXDevPack CMakeLists.txt

add_library(nxdev_pack_core STATIC
    core/src/pack.cpp
    core/src/process.cpp
    nro/src/nro_backend.cpp
    nsp/src/nsp_backend.cpp
)

add_library(NXDevPack::Core ALIAS nxdev_pack_core)
add_library(NXDevPack::NRO ALIAS nxdev_pack_core)
add_library(NXDevPack::NSP ALIAS nxdev_pack_core)

target_include_directories(nxdev_pack_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/core/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/nro/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/nsp/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(nxdev_pack_core PUBLIC
    NXDevAppManifest::Parser
)

nxdev_configure_target(nxdev_pack_core)
