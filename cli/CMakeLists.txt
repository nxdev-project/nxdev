# NXDev CLI CMakeLists.txt

add_executable(nxdev
    src/main.cpp
    src/cli.cpp
    src/commands/version_cmd.cpp
    src/commands/doctor_cmd.cpp
    src/commands/build_cmd.cpp
    src/commands/pack_cmd.cpp
    src/commands/new_cmd.cpp
    src/commands/manifest_cmd.cpp
)

add_executable(NXDev::CLI ALIAS nxdev)

target_include_directories(nxdev PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(nxdev PRIVATE
    NXDevAppManifest::Parser
    NXDevPack::Core
)

nxdev_configure_target(nxdev)

# Install rule for CLI
install(TARGETS nxdev DESTINATION bin)
