# NXDevCompilerOptions.cmake
# Standardizes compilation flags, warning levels, and language standards

function(nxdev_apply_compiler_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE
            /W4
            /permissive-
            /utf-8
        )
    else()
        target_compile_options(${target_name} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wnon-virtual-dtor
            -Wcast-align
            -Wunused
            -Woverloaded-virtual
            -Wnull-dereference
            -Wformat=2
        )
    endif()
endfunction()

function(nxdev_configure_target target_name)
    target_compile_features(${target_name} PUBLIC cxx_std_20)
    nxdev_apply_compiler_warnings(${target_name})
    target_compile_definitions(${target_name} PUBLIC
        NXDEV_VERSION="${NXDEV_VERSION_STRING}"
        NXDEV_VERSION_MAJOR=${NXDEV_VERSION_MAJOR}
        NXDEV_VERSION_MINOR=${NXDEV_VERSION_MINOR}
        NXDEV_VERSION_PATCH=${NXDEV_VERSION_PATCH}
    )
endfunction()
