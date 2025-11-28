if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_INSTALL_INCLUDEDIR
        "include/fast_hex-${PROJECT_VERSION}"
        CACHE STRING
        ""
    )
    set_property(CACHE CMAKE_INSTALL_INCLUDEDIR PROPERTY TYPE PATH)
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# find_package(<package>) call for consumers to find this project
set(package fast_hex)

# Install header-only library unconditionally
install(
    FILES include/fast_hex/fast_hex_inline.hpp
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/fast_hex"
    COMPONENT fast_hex_Development
)

# Install library targets and full headers only if building libraries
if(fast_hex_BUILD_LIBS)
    install(
        FILES include/fast_hex/fast_hex.hpp
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/fast_hex"
        COMPONENT fast_hex_Development
    )

    install(
        DIRECTORY "${PROJECT_BINARY_DIR}/export/"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        COMPONENT fast_hex_Development
    )

    install(
        TARGETS fast_hex_fast_hex
        EXPORT fast_hexTargets
        RUNTIME #
            COMPONENT fast_hex_Runtime
        LIBRARY #
            COMPONENT fast_hex_Runtime
            NAMELINK_COMPONENT fast_hex_Development
        ARCHIVE #
            COMPONENT fast_hex_Development
        INCLUDES #
        DESTINATION
            "${CMAKE_INSTALL_INCLUDEDIR}"
    )
endif()

if(fast_hex_BUILD_LIBS)
    write_basic_package_version_file(
        "${package}ConfigVersion.cmake"
        COMPATIBILITY SameMajorVersion
    )

    # Allow package maintainers to freely override the path for the configs
    set(fast_hex_INSTALL_CMAKEDIR
        "${CMAKE_INSTALL_LIBDIR}/cmake/${package}"
        CACHE STRING
        "CMake package config location relative to the install prefix"
    )
    set_property(CACHE fast_hex_INSTALL_CMAKEDIR PROPERTY TYPE PATH)
    mark_as_advanced(fast_hex_INSTALL_CMAKEDIR)

    install(
        FILES cmake/install-config.cmake
        DESTINATION "${fast_hex_INSTALL_CMAKEDIR}"
        RENAME "${package}Config.cmake"
        COMPONENT fast_hex_Development
    )

    install(
        FILES "${PROJECT_BINARY_DIR}/${package}ConfigVersion.cmake"
        DESTINATION "${fast_hex_INSTALL_CMAKEDIR}"
        COMPONENT fast_hex_Development
    )

    install(
        EXPORT fast_hexTargets
        NAMESPACE fast_hex::
        DESTINATION "${fast_hex_INSTALL_CMAKEDIR}"
        COMPONENT fast_hex_Development
    )
endif()

if(PROJECT_IS_TOP_LEVEL)
    include(CPack)
endif()
