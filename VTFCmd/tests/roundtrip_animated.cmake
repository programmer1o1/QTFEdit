# Animated round-trip test:
#   mkanimvtf <frame0> <frame1> -> animated.vtf
#   vtfcmd -file animated.vtf -exportformat png -> frames emitted per frame
#   imgdiff frame0_ref vs emitted-frame-0 (PSNR gate)
#
# Expected args (passed with -D):
#   -DVTFCMD=<path to vtfcmd>
#   -DMKANIMVTF=<path to mkanimvtf>
#   -DIMGDIFF=<path to imgdiff>
#   -DFRAME0=<path to first input PNG>
#   -DFRAME1=<path to second input PNG>
#   -DWORK_DIR=<scratch dir>
# Optional:
#   -DDIFF_TOL (default 255) / -DDIFF_MIN_PSNR (default 20)

foreach(var VTFCMD MKANIMVTF IMGDIFF FRAME0 FRAME1 WORK_DIR)
    if(NOT ${var})
        message(FATAL_ERROR "roundtrip_animated.cmake: ${var} is required")
    endif()
endforeach()
foreach(var VTFCMD MKANIMVTF IMGDIFF FRAME0 FRAME1)
    if(NOT EXISTS "${${var}}")
        message(FATAL_ERROR "roundtrip_animated.cmake: ${var}='${${var}}' does not exist")
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

set(_vtf "${WORK_DIR}/animated.vtf")
set(_ref0 "${WORK_DIR}/frame0_ref.png")
set(_ref1 "${WORK_DIR}/frame1_ref.png")
configure_file("${FRAME0}" "${_ref0}" COPYONLY)
configure_file("${FRAME1}" "${_ref1}" COPYONLY)

# 1) Build animated VTF.
execute_process(
    COMMAND "${MKANIMVTF}" "${_vtf}" "${FRAME0}" "${FRAME1}"
    RESULT_VARIABLE _rc1
    OUTPUT_VARIABLE _out1
    ERROR_VARIABLE _err1
)
if(NOT _rc1 EQUAL 0)
    message(FATAL_ERROR "roundtrip_animated: mkanimvtf failed (rc=${_rc1})\nstdout:\n${_out1}\nstderr:\n${_err1}")
endif()
if(NOT EXISTS "${_vtf}")
    message(FATAL_ERROR "roundtrip_animated: mkanimvtf did not produce '${_vtf}'")
endif()

# Validate VTF header has frame count == 2. Layout per SVTFHeader_70:
#   Signature(4) Version[2](8) HeaderSize(4) Width(2) Height(2) Flags(4) Frames(2) ...
#   byte offset of Frames = 4+8+4+2+2+4 = 24, so hex offsets 48..51.
file(READ "${_vtf}" _hex LIMIT 32 HEX)
string(SUBSTRING "${_hex}" 48 2 _fr_lo)
string(SUBSTRING "${_hex}" 50 2 _fr_hi)
math(EXPR _frames "0x${_fr_hi}${_fr_lo}")
if(NOT _frames EQUAL 2)
    message(FATAL_ERROR "roundtrip_animated: expected 2 frames in VTF header, got ${_frames}")
endif()

# 2) Decode VTF back to per-frame PNGs. `-extract-all-frames` tells vtfcmd to write every frame
#    with a `_frameN` suffix instead of collapsing to frame 0.
execute_process(
    COMMAND "${VTFCMD}" -file "${_vtf}" -exportformat png -extract-all-frames -silent -output "${WORK_DIR}"
    RESULT_VARIABLE _rc2
    OUTPUT_VARIABLE _out2
    ERROR_VARIABLE _err2
)
if(NOT _rc2 EQUAL 0)
    message(FATAL_ERROR "roundtrip_animated: VTF→PNG failed (rc=${_rc2})\nstdout:\n${_out2}\nstderr:\n${_err2}")
endif()

# Diff every frame against its input reference.
set(_per_frame_refs "${_ref0}" "${_ref1}")
math(EXPR _last_frame "${_frames} - 1")
foreach(i RANGE 0 ${_last_frame})
    file(GLOB _candidates "${WORK_DIR}/*_frame${i}.png")
    if(NOT _candidates)
        message(FATAL_ERROR "roundtrip_animated: no *_frame${i}.png decoded in ${WORK_DIR}")
    endif()
    list(GET _candidates 0 _cand)
    list(GET _per_frame_refs ${i} _ref_i)
    execute_process(
        COMMAND "${IMGDIFF}" "${_ref_i}" "${_cand}" ${DIFF_TOL} ${DIFF_MIN_PSNR}
        RESULT_VARIABLE _rc3
        OUTPUT_VARIABLE _out3
        ERROR_VARIABLE _err3
    )
    if(NOT _rc3 EQUAL 0)
        message(FATAL_ERROR "roundtrip_animated[frame${i}]: imgdiff failed (rc=${_rc3})\nstdout:\n${_out3}\nstderr:\n${_err3}")
    endif()
    string(STRIP "${_out3}" _out3_stripped)
    message(STATUS "roundtrip_animated[frame${i}]: ${_out3_stripped}")
endforeach()
