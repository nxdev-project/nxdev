# NXDevVersion.cmake
# Authoritatively extracts the project version from the root VERSION file

function(nxdev_extract_version)
    set(VERSION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
    if(NOT EXISTS "${VERSION_FILE}")
        message(FATAL_ERROR "VERSION file not found at: ${VERSION_FILE}")
    endif()

    file(STRINGS "${VERSION_FILE}" NXDEV_RAW_VERSION)
    string(STRIP "${NXDEV_RAW_VERSION}" NXDEV_RAW_VERSION)

    set(NXDEV_VERSION_STRING "${NXDEV_RAW_VERSION}" PARENT_SCOPE)

    # Parse semantic version (e.g. 0.1.0-dev or 0.1.0)
    string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)(-(.*))?$" _ "${NXDEV_RAW_VERSION}")
    set(NXDEV_VERSION_MAJOR "${CMAKE_MATCH_1}" PARENT_SCOPE)
    set(NXDEV_VERSION_MINOR "${CMAKE_MATCH_2}" PARENT_SCOPE)
    set(NXDEV_VERSION_PATCH "${CMAKE_MATCH_3}" PARENT_SCOPE)
    set(NXDEV_VERSION_PRERELEASE "${CMAKE_MATCH_5}" PARENT_SCOPE)
    set(NXDEV_VERSION_PLAIN "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}" PARENT_SCOPE)

    message(STATUS "NXDev Version: ${NXDEV_RAW_VERSION} (${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3})")
endfunction()
