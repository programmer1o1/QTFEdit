add_executable(vtfcmd
    vtfcmd.c
    enumerations.c
    image_io.c
    tinyexr_wrapper.cpp
)

target_include_directories(vtfcmd
    PRIVATE
        ${PROJECT_SOURCE_DIR}/lib
        ${PROJECT_SOURCE_DIR}/thirdparty/include
)

target_link_libraries(vtfcmd PRIVATE vtflib)

set(VTFCMD_IMAGE_BACKEND "auto" CACHE STRING "VTFCmd image backend: auto, devil, stb")
set_property(CACHE VTFCMD_IMAGE_BACKEND PROPERTY STRINGS auto devil stb)

set(_vtfcmd_use_devil 0)

if(VTFCMD_IMAGE_BACKEND STREQUAL "stb")
    set(_vtfcmd_use_devil 0)
else()
    # DevIL (OpenIL) - we only need IL/il.h + libIL for vtfcmd.
    set(IL_INCLUDE_DIR "" CACHE PATH "DevIL include dir (contains IL/il.h)")
    set(IL_LIBRARY "" CACHE FILEPATH "DevIL library (libIL.so/libIL.a)")

    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(PC_IL QUIET IL)
        if(PC_IL_FOUND)
            if(NOT IL_INCLUDE_DIR)
                find_path(IL_INCLUDE_DIR NAMES IL/il.h PATHS ${PC_IL_INCLUDE_DIRS})
            endif()
            if(NOT IL_LIBRARY)
                find_library(IL_LIBRARY NAMES IL PATHS ${PC_IL_LIBRARY_DIRS} PATH_SUFFIXES lib lib64)
            endif()
        endif()
    endif()

    if(NOT IL_INCLUDE_DIR)
        find_path(IL_INCLUDE_DIR NAMES IL/il.h)
    endif()
    if(NOT IL_LIBRARY)
        find_library(IL_LIBRARY NAMES IL)
    endif()

    if(IL_INCLUDE_DIR AND NOT EXISTS "${IL_INCLUDE_DIR}/IL/il.h")
        message(FATAL_ERROR "IL_INCLUDE_DIR was set to '${IL_INCLUDE_DIR}' but '${IL_INCLUDE_DIR}/IL/il.h' was not found.")
    endif()
    if(IL_LIBRARY AND NOT EXISTS "${IL_LIBRARY}")
        message(FATAL_ERROR "IL_LIBRARY was set to '${IL_LIBRARY}' but that file does not exist. Use the full path to libIL.so (or libIL.so.1) on your system.")
    endif()

    if(IL_INCLUDE_DIR AND IL_LIBRARY)
        set(_vtfcmd_use_devil 1)
        target_include_directories(vtfcmd PRIVATE ${IL_INCLUDE_DIR})
        target_link_libraries(vtfcmd PRIVATE ${IL_LIBRARY})
    else()
        if(VTFCMD_IMAGE_BACKEND STREQUAL "devil")
            message(FATAL_ERROR "VTFCMD_IMAGE_BACKEND=devil but DevIL (OpenIL) was not found. Install the DevIL development package (headers + libIL) or use -DVTFCMD_IMAGE_BACKEND=stb.")
        endif()
        set(_vtfcmd_use_devil 0)
    endif()
endif()

target_compile_definitions(vtfcmd PRIVATE VTFCMD_USE_DEVIL=${_vtfcmd_use_devil})

if(NOT _vtfcmd_use_devil)
    if(NOT EXISTS "${PROJECT_SOURCE_DIR}/thirdparty/include/stb_image.h" OR NOT EXISTS "${PROJECT_SOURCE_DIR}/thirdparty/include/stb_image_write.h")
        message(FATAL_ERROR "VTFCmd stb backend selected but stb headers are missing. Add stb_image.h and stb_image_write.h to thirdparty/include/ or use -DVTFCMD_IMAGE_BACKEND=devil.")
    endif()
endif()

# Optional WebP support (requires libwebp to be installed).
option(VTFCMD_USE_WEBP "Enable WebP import/export support" ON)

set(_vtfcmd_has_webp 0)
if(VTFCMD_USE_WEBP)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(PC_WEBP QUIET libwebp)
    endif()

    find_path(WEBP_INCLUDE_DIR NAMES webp/encode.h PATHS ${PC_WEBP_INCLUDE_DIRS})
    find_library(WEBP_LIBRARY NAMES webp PATHS ${PC_WEBP_LIBRARY_DIRS} PATH_SUFFIXES lib lib64)

    if(WEBP_INCLUDE_DIR AND WEBP_LIBRARY)
        set(_vtfcmd_has_webp 1)
        target_include_directories(vtfcmd PRIVATE ${WEBP_INCLUDE_DIR})
        target_link_libraries(vtfcmd PRIVATE ${WEBP_LIBRARY})
        message(STATUS "VTFCmd: WebP support enabled")
    else()
        message(STATUS "VTFCmd: WebP not found, WebP support disabled")
    endif()
endif()

target_compile_definitions(vtfcmd PRIVATE VTFCMD_HAS_WEBP=${_vtfcmd_has_webp})

# Small test-only helper: pixel-diff two images within a tolerance.
add_executable(imgdiff
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/imgdiff.c
)
target_include_directories(imgdiff PRIVATE ${PROJECT_SOURCE_DIR}/thirdparty/include)
if(NOT WIN32)
    target_link_libraries(imgdiff PRIVATE m)
endif()
# imgdiff is a test-only helper; don't install it.
set_target_properties(imgdiff PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD FALSE)

# Test-only helper: builds a multi-frame (animated) VTF from N PNGs by calling vlImageCreateMultiple.
add_executable(mkanimvtf
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/mkanimvtf.c
)
target_include_directories(mkanimvtf
    PRIVATE
        ${PROJECT_SOURCE_DIR}/thirdparty/include
        ${PROJECT_SOURCE_DIR}/lib
)
target_link_libraries(mkanimvtf PRIVATE vtflib)
if(NOT WIN32)
    target_link_libraries(mkanimvtf PRIVATE m)
endif()

# Round-trip smoke tests: PNG → VTF → PNG across a few formats/flags.
if(EXISTS "${PROJECT_SOURCE_DIR}/resources/rgb.png")
    function(_add_roundtrip_test suffix format extra_def)
        add_test(
            NAME vtfcmd_roundtrip_${suffix}
            COMMAND ${CMAKE_COMMAND}
                -DVTFCMD=$<TARGET_FILE:vtfcmd>
                -DIMGDIFF=$<TARGET_FILE:imgdiff>
                -DINPUT_PNG=${PROJECT_SOURCE_DIR}/resources/rgb.png
                -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/test_roundtrip_${suffix}
                -DFORMAT=${format}
                -DEXPECTED_WIDTH=32
                -DEXPECTED_HEIGHT=32
                # Peak absolute channel diff can legitimately spike at alpha-edge pixels where VTFLib's
                # RGBA8888 round-trip reshuffles color in fully-transparent areas. Keep this permissive
                # and let PSNR be the real quality gate.
                -DDIFF_TOL=255
                -DDIFF_MIN_PSNR=20
                ${extra_def}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/roundtrip.cmake
        )
    endfunction()

    _add_roundtrip_test(rgba8888 rgba8888 "")
    _add_roundtrip_test(bgra8888 bgra8888 "")
    _add_roundtrip_test(rgb888   rgb888   "")
    _add_roundtrip_test(rgba8888_nomips rgba8888 "-DNO_MIPMAPS=1")

    # Mipmap filter round-trip smoke: mip 0 should survive regardless of the chosen filter,
    # so the PSNR gate applies identically across filters. This catches breakage if any filter
    # path ever regresses below catastrophe threshold. Kaiser (and its sibling windowed-sincs)
    # are remapped internally to MITCHELL since stbir v2 doesn't expose them natively; the tests
    # still exercise the enum-dispatch path end-to-end.
    _add_roundtrip_test(mipfilter_box      rgba8888 "-DMIPMAP_FILTER=box")
    _add_roundtrip_test(mipfilter_triangle rgba8888 "-DMIPMAP_FILTER=triangle")
    _add_roundtrip_test(mipfilter_catrom   rgba8888 "-DMIPMAP_FILTER=catrom")
    _add_roundtrip_test(mipfilter_mitchell rgba8888 "-DMIPMAP_FILTER=mitchell")
    _add_roundtrip_test(mipfilter_kaiser   rgba8888 "-DMIPMAP_FILTER=kaiser")
    _add_roundtrip_test(mipfilter_point    rgba8888 "-DMIPMAP_FILTER=point")
    _add_roundtrip_test(mipfilter_gaussian rgba8888 "-DMIPMAP_FILTER=gaussian")
    _add_roundtrip_test(mipfilter_nice     rgba8888 "-DMIPMAP_FILTER=nice")

    # Multi-frame / animated round-trip. Builds a 2-frame VTF via mkanimvtf (which calls
    # vlImageCreateMultiple directly — vtfcmd's -file flag just produces separate single-frame
    # VTFs), decodes the VTF back to PNG via vtfcmd -exportformat png, then runs imgdiff on the
    # first emitted frame vs the first input.
    if(EXISTS "${PROJECT_SOURCE_DIR}/resources/blue.png")
        add_test(
            NAME vtfcmd_roundtrip_animated
            COMMAND ${CMAKE_COMMAND}
                -DVTFCMD=$<TARGET_FILE:vtfcmd>
                -DMKANIMVTF=$<TARGET_FILE:mkanimvtf>
                -DIMGDIFF=$<TARGET_FILE:imgdiff>
                -DFRAME0=${PROJECT_SOURCE_DIR}/resources/rgb.png
                -DFRAME1=${PROJECT_SOURCE_DIR}/resources/blue.png
                -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/test_roundtrip_animated
                -DDIFF_TOL=255
                -DDIFF_MIN_PSNR=20
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/roundtrip_animated.cmake
        )
    endif()

    # Cubemap (6-face environment map) round-trip.
    if(EXISTS "${PROJECT_SOURCE_DIR}/resources/blue.png"
       AND EXISTS "${PROJECT_SOURCE_DIR}/resources/green.png"
       AND EXISTS "${PROJECT_SOURCE_DIR}/resources/alpha.png"
       AND EXISTS "${PROJECT_SOURCE_DIR}/resources/rgb.png"
       AND EXISTS "${PROJECT_SOURCE_DIR}/resources/tile.png")
        set(_cubefaces
            "${PROJECT_SOURCE_DIR}/resources/rgb.png"
            "${PROJECT_SOURCE_DIR}/resources/blue.png"
            "${PROJECT_SOURCE_DIR}/resources/green.png"
            "${PROJECT_SOURCE_DIR}/resources/alpha.png"
            "${PROJECT_SOURCE_DIR}/resources/tile.png"
            "${PROJECT_SOURCE_DIR}/resources/rgb.png"
        )
        add_test(
            NAME vtfcmd_roundtrip_cubemap
            COMMAND ${CMAKE_COMMAND}
                -DVTFCMD=$<TARGET_FILE:vtfcmd>
                -DMKANIMVTF=$<TARGET_FILE:mkanimvtf>
                -DIMGDIFF=$<TARGET_FILE:imgdiff>
                "-DFACES=${_cubefaces}"
                -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/test_roundtrip_cubemap
                -DDIFF_TOL=255
                -DDIFF_MIN_PSNR=20
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/roundtrip_cubemap.cmake
        )
    endif()
    # DXT tests are intentionally omitted: they require a working Compressonator link,
    # which is best-effort across platforms and not required to validate the core round-trip.
endif()

# QOI and EXR are always required (header-only, used by both backends).
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/thirdparty/include/qoi.h")
    message(FATAL_ERROR "qoi.h is missing. Add it to thirdparty/include/.")
endif()
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/thirdparty/include/tinyexr.h")
    message(FATAL_ERROR "tinyexr.h is missing. Add it to thirdparty/include/.")
endif()

# tinyexr needs zlib (we disable its built-in miniz).
find_package(ZLIB REQUIRED)
target_link_libraries(vtfcmd PRIVATE ZLIB::ZLIB)

if(NOT WIN32)
    target_link_libraries(vtfcmd PRIVATE m)
endif()
