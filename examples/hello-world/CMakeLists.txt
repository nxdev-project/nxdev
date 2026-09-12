add_executable(nxdev_example_hello_world
    src/main.cpp
)

target_link_libraries(nxdev_example_hello_world PRIVATE
    NXDev::Core
    NXDev::Input
    NXDev::Display
)

nxdev_configure_target(nxdev_example_hello_world)
