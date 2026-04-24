# Round-trip test: PNG → VTF → PNG via vtfcmd.
# Expected args (passed with -D):
#   -DVTFCMD=<path to vtfcmd binary>
#   -DINPUT_PNG=<path to a small test PNG>
#   -DWORK_DIR=<scratch directory; will be created>
# Optional:
#   -DFORMAT=<vtfcmd -format value>   (default: rgba8888)
#   -DNO_MIPMAPS=1                    (pass -nomipmaps)

if(NOT VTFCMD OR NOT EXISTS "${VTFCMD}")
    message(FATAL_ERROR "roundtrip.cmake: VTFCMD not found at '${VTFCMD}'")
endif()
if(NOT INPUT_PNG OR NOT EXISTS "${INPUT_PNG}")
    message(FATAL_ERROR "roundtrip.cmake: INPUT_PNG not found at '${INPUT_PNG}'")
endif()
if(NOT WORK_DIR)
    message(FATAL_ERROR "roundtrip.cmake: WORK_DIR is required")
endif()
if(NOT FORMAT)
    set(FORMAT "rgba8888")
endif()

file(REMOVE_RECURSE "${WORK_DIR}")
file(MAKE_DIRECTORY "${WORK_DIR}")

# Stage the PNG next to where vtfcmd will write the VTF.
get_filename_component(_name "${INPUT_PNG}" NAME_WE)
set(_staged_png "${WORK_DIR}/${_name}.png")
configure_file("${INPUT_PNG}" "${_staged_png}" COPYONLY)
# Keep a reference copy because the VTF→PNG step overwrites the staged input.
set(_ref_png "${WORK_DIR}/${_name}_ref.png")
configure_file("${INPUT_PNG}" "${_ref_png}" COPYONLY)

set(_extra_args "")
if(NO_MIPMAPS)
    list(APPEND _extra_args -nomipmaps)
endif()
if(MIPMAP_FILTER)
    list(APPEND _extra_args -mfilter "${MIPMAP_FILTER}")
endif()

# 1) PNG → VTF.
execute_process(
    COMMAND "${VTFCMD}" -file "${_staged_png}" -format "${FORMAT}" ${_extra_args} -silent -output "${WORK_DIR}"
    RESULT_VARIABLE _rc1
    OUTPUT_VARIABLE _out1
    ERROR_VARIABLE _err1
)
if(NOT _rc1 EQUAL 0)
    message(FATAL_ERROR "roundtrip: PNG→VTF failed (rc=${_rc1})\nstdout:\n${_out1}\nstderr:\n${_err1}")
endif()

set(_produced_vtf "${WORK_DIR}/${_name}.vtf")
if(NOT EXISTS "${_produced_vtf}")
    file(GLOB _stragglers "${WORK_DIR}/*")
    message(FATAL_ERROR
        "roundtrip: expected VTF not produced at '${_produced_vtf}'\n"
        "vtfcmd exit=${_rc1}\n"
        "vtfcmd stdout:\n${_out1}\n"
        "vtfcmd stderr:\n${_err1}\n"
        "work dir contents: ${_stragglers}")
endif()

file(SIZE "${_produced_vtf}" _vtf_size)
if(_vtf_size LESS 80)
    message(FATAL_ERROR "roundtrip: produced VTF is suspiciously small (${_vtf_size} bytes)")
endif()

# Validate VTF header: "VTF\0" magic at offset 0, sane width/height at 16/18 (little-endian u16).
file(READ "${_produced_vtf}" _hex LIMIT 20 HEX)
string(SUBSTRING "${_hex}" 0 8 _magic)
if(NOT _magic STREQUAL "56544600")
    message(FATAL_ERROR "roundtrip[${FORMAT}]: VTF magic bytes wrong (got '${_magic}', want '56544600')")
endif()

# Width: hex chars 32..35 (bytes 16..17), little-endian u16.
string(SUBSTRING "${_hex}" 32 2 _w_lo_hex)
string(SUBSTRING "${_hex}" 34 2 _w_hi_hex)
string(SUBSTRING "${_hex}" 36 2 _h_lo_hex)
string(SUBSTRING "${_hex}" 38 2 _h_hi_hex)
math(EXPR _w "0x${_w_hi_hex}${_w_lo_hex}")
math(EXPR _h "0x${_h_hi_hex}${_h_lo_hex}")
if(NOT _w GREATER 0 OR NOT _h GREATER 0 OR _w GREATER 16384 OR _h GREATER 16384)
    message(FATAL_ERROR "roundtrip[${FORMAT}]: VTF header reports implausible dimensions ${_w}x${_h}")
endif()

if(EXPECTED_WIDTH AND NOT _w EQUAL EXPECTED_WIDTH)
    message(FATAL_ERROR "roundtrip[${FORMAT}]: VTF width ${_w} != expected ${EXPECTED_WIDTH}")
endif()
if(EXPECTED_HEIGHT AND NOT _h EQUAL EXPECTED_HEIGHT)
    message(FATAL_ERROR "roundtrip[${FORMAT}]: VTF height ${_h} != expected ${EXPECTED_HEIGHT}")
endif()

# 2) VTF → PNG. vtfcmd writes alongside the input.
execute_process(
    COMMAND "${VTFCMD}" -file "${_produced_vtf}" -exportformat png -silent -output "${WORK_DIR}"
    RESULT_VARIABLE _rc2
    OUTPUT_VARIABLE _out2
    ERROR_VARIABLE _err2
)
if(NOT _rc2 EQUAL 0)
    message(FATAL_ERROR "roundtrip: VTF→PNG failed (rc=${_rc2})\nstdout:\n${_out2}\nstderr:\n${_err2}")
endif()

# VTFCmd may overwrite the staged PNG; verify a PNG exists and is non-trivial.
set(_out_png "${WORK_DIR}/${_name}.png")
if(NOT EXISTS "${_out_png}")
    message(FATAL_ERROR "roundtrip: expected output PNG not produced at '${_out_png}'")
endif()

file(SIZE "${_out_png}" _png_size)
if(_png_size LESS 32)
    message(FATAL_ERROR "roundtrip: output PNG is suspiciously small (${_png_size} bytes)")
endif()

message(STATUS "roundtrip[${FORMAT}]: OK — VTF ${_vtf_size}B, ${_w}x${_h}, PNG ${_png_size}B")

# Optional pixel-diff against the reference PNG. Only runs when IMGDIFF is set and exists.
if(IMGDIFF AND EXISTS "${IMGDIFF}")
    if(NOT DIFF_TOL)
        set(DIFF_TOL 64)
    endif()
    if(NOT DIFF_MIN_PSNR)
        set(DIFF_MIN_PSNR 20)
    endif()
    execute_process(
        COMMAND "${IMGDIFF}" "${_ref_png}" "${_out_png}" ${DIFF_TOL} ${DIFF_MIN_PSNR}
        RESULT_VARIABLE _rc3
        OUTPUT_VARIABLE _out3
        ERROR_VARIABLE _err3
    )
    if(NOT _rc3 EQUAL 0)
        message(FATAL_ERROR
            "roundtrip[${FORMAT}]: pixel-diff failed (rc=${_rc3}, tol=${DIFF_TOL})\nstdout:\n${_out3}\nstderr:\n${_err3}")
    endif()
    string(STRIP "${_out3}" _out3_stripped)
    if(_out3_stripped)
        message(STATUS "roundtrip[${FORMAT}]: ${_out3_stripped}")
    endif()
endif()
