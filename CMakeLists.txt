cmake_minimum_required(VERSION 3.15)

# Some distros ship the binary as `ninja` (not `ninja-build`). If CMake picked a
# non-existent program, fix it up automatically.
if(CMAKE_GENERATOR MATCHES "Ninja")
    if(CMAKE_MAKE_PROGRAM AND NOT EXISTS "${CMAKE_MAKE_PROGRAM}")
        find_program(NINJA_EXE NAMES ninja ninja-build)
        if(NINJA_EXE)
            set(CMAKE_MAKE_PROGRAM "${NINJA_EXE}" CACHE FILEPATH "Ninja build program" FORCE)
        endif()
    endif()
endif()

# QTFEdit (Qt) is the primary GUI going forward. The legacy WinForms GUI and
# Visual Studio solutions live under `legacy/` and are frozen.
project(QTFEdit VERSION 3.5.0 LANGUAGES C CXX)

option(VTFLIB_USE_COMPRESSONATOR "Link against AMD Compressonator if available" ON)
option(BUILD_VTFCMD "Build VTFCmd CLI" ON)
option(BUILD_VTFEDIT_QT "Build cross-platform Qt GUI (QTFEdit)" OFF)

enable_testing()

add_subdirectory(VTFLib)
if(BUILD_VTFCMD)
    add_subdirectory(VTFCmd)
endif()
if(BUILD_VTFEDIT_QT)
    add_subdirectory(VTFEdit)
endif()

# Linux shell integration: ship a freedesktop thumbnailer + MIME-type XML when installing.
# These are harmless on other platforms because `UNIX AND NOT APPLE` gates the install rule.
if(UNIX AND NOT APPLE)
    install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/vtf.thumbnailer
        DESTINATION share/thumbnailers
        COMPONENT shell_integration
        OPTIONAL
    )
    install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/vtf-mime.xml
        DESTINATION share/mime/packages
        COMPONENT shell_integration
        OPTIONAL
    )
endif()
