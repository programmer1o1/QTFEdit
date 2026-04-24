# Cubemap round-trip test:
#   mkanimvtf --cube <face0..5> -> cube.vtf  (6 faces, 1 frame, envmap flag)
#   vtfcmd -file cube.vtf -exportformat png -> 6 PNGs
#   imgdiff face 0 reference vs emitted
#
# Expected args (passed with -D):
#   -DVTFCMD=<path>
#   -DMKANIMVTF=<path>
#   -DIMGDIFF=<path>
#   -DFACES=<semicolon-separated list of 6 PNG paths>
#   -DWORK_DIR=<scratch dir>
# Optional:
#   -DDIFF_TOL (default 255) / -DDIFF_MIN_PSNR (default 20)

foreach(var VTFCMD MKANIMVTF IMGDIFF FACES WORK_DIR)
    if(NOT ${var})
        message(FATAL_ERROR "roundtrip_cubemap.cmake: ${var} is required")
    endif()
endforeach()
foreach(var VTFCMD MKANIMVTF IMGDIFF)
    if(NOT EXISTS "${${var}}")
        message(FATAL_ERROR "roundtrip_cubemap.cmake: ${var}='${${var}}' does not exist")
    endif()
endforeach()
list(LENGTH FACES _nfaces)
if(NOT _nfaces EQUAL 6)
    message(FATAL_ERROR "roundtrip_cubemap.cmake: FACES must contain exactly 6 entries (got ${_nfaces})")
endif()
foreach(f ${FACES})
    if(NOT EXISTS "${f}")
        message(FATAL_ERROR "roundtrip_cubemap.cmake: face '${f}' does not exist")
    endif()
endforeach()
if(NOT DIFF_TOL)
    set(DIFF_TOL 255)
endif()
if(NOT DIFF_MIN_PSNR)
    set(DIFF_MIN_PSNR 20)
endif()

file(REMOVE_RECURSE "${WORK_DIR}")
file(MAKE_DIRECTORY "${WORK_DIR}")

set(_vtf "${WORK_DIR}/cube.vtf")
list(GET FACES 0 _face0)
set(_ref "${WORK_DIR}/face0_ref.png")
configure_file("${_face0}" "${_ref}" COPYONLY)

# 1) Build cubemap VTF.
execute_process(
    COMMAND "${MKANIMVTF}" --cube "${_vtf}" ${FACES}
    RESULT_VARIABLE _rc1
    OUTPUT_VARIABLE _out1
    ERROR_VARIABLE _err1
)
if(NOT _rc1 EQUAL 0)
    message(FATAL_ERROR "roundtrip_cubemap: mkanimvtf failed (rc=${_rc1})\nstdout:\n${_out1}\nstderr:\n${_err1}")
endif()

# Validate Faces=6 in the header. Layout per SVTFHeader_71:
#   Signature(4) Version[2](8) HeaderSize(4) Width(2) Height(2) Flags(4) Frames(2) StartFrame(2)
#   Padding0[4](4) Reflectivity[3](12) Padding1[4](4) BumpScale(4) ImageFormat(4) MipCount(1)
#   LowResImageFormat(4) LowResImageWidth(1) LowResImageHeight(1) Depth(2)      <- v7.1 adds Depth
# Faces field is *not* in the 7.0 header. vtfcmd/VTFLib derives faces from TEXTUREFLAGS_ENVMAP.
# So check the ENVMAP flag bit (at byte 20..23, u32 LE).
file(READ "${_vtf}" _hex LIMIT 32 HEX)
string(SUBSTRING "${_hex}" 40 2 _flags_0)
string(SUBSTRING "${_hex}" 42 2 _flags_1)
string(SUBSTRING "${_hex}" 44 2 _flags_2)
string(SUBSTRING "${_hex}" 46 2 _flags_3)
math(EXPR _flags "0x${_flags_3}${_flags_2}${_flags_1}${_flags_0}")
math(EXPR _envmap "${_flags} & 0x00004000")  # TEXTUREFLAGS_ENVMAP = 0x4000
if(NOT _envmap)
    message(FATAL_ERROR "roundtrip_cubemap: VTF header flags 0x${_flags_3}${_flags_2}${_flags_1}${_flags_0} missing TEXTUREFLAGS_ENVMAP")
endif()

# 2) Decode back to PNG.
execute_process(
    COMMAND "${VTFCMD}" -file "${_vtf}" -exportformat png -silent -output "${WORK_DIR}"
    RESULT_VARIABLE _rc2
    OUTPUT_VARIABLE _out2
    ERROR_VARIABLE _err2
)
if(NOT _rc2 EQUAL 0)
    message(FATAL_ERROR "roundtrip_cubemap: VTF→PNG failed (rc=${_rc2})\nstdout:\n${_out2}\nstderr:\n${_err2}")
endif()

file(GLOB _pngs "${WORK_DIR}/*.png")
list(REMOVE_ITEM _pngs "${_ref}")
if(NOT _pngs)
    message(FATAL_ERROR "roundtrip_cubemap: no decoded PNG found in ${WORK_DIR}")
endif()
list(SORT _pngs)
list(GET _pngs 0 _candidate)

message(STATUS "roundtrip_cubemap: flags=0x${_flags_3}${_flags_2}${_flags_1}${_flags_0} (ENVMAP set); diffing face 0: ${_candidate}")

execute_process(
    COMMAND "${IMGDIFF}" "${_ref}" "${_candidate}" ${DIFF_TOL} ${DIFF_MIN_PSNR}
    RESULT_VARIABLE _rc3
    OUTPUT_VARIABLE _out3
    ERROR_VARIABLE _err3
)
if(NOT _rc3 EQUAL 0)
    message(FATAL_ERROR "roundtrip_cubemap: imgdiff failed (rc=${_rc3})\nstdout:\n${_out3}\nstderr:\n${_err3}")
endif()
string(STRIP "${_out3}" _out3_stripped)
if(_out3_stripped)
    message(STATUS "roundtrip_cubemap: ${_out3_stripped}")
endif()
