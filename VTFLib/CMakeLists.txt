set(VTFLIB_SOURCES
    VMTFile.cpp
    VMTIntegerNode.cpp
    VMTStringNode.cpp
    VMTGroupNode.cpp
    VMTNode.cpp
    VMTSingleNode.cpp
    VMTValueNode.cpp
    VMTWrapper.cpp
    VTFFile.cpp
    VTFLib.cpp
    VTFMathlib.cpp
    VTFWrapper.cpp
    Error.cpp
    FileReader.cpp
    FileWriter.cpp
    MemoryReader.cpp
    MemoryWriter.cpp
    ProcReader.cpp
    ProcWriter.cpp
    Proc.cpp
    Float16.cpp
)

add_library(vtflib STATIC ${VTFLIB_SOURCES})
target_include_directories(vtflib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/lib
        ${PROJECT_SOURCE_DIR}/thirdparty/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_definitions(vtflib PUBLIC VTFLIB_STATIC)

set_target_properties(vtflib PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Compressonator (optional)
if(VTFLIB_USE_COMPRESSONATOR)
    set(_vtflib_has_compressonator OFF)
    set(_vtflib_compressonator_intentionally_disabled OFF)
    set(_compressonator_search_paths
        ${PROJECT_SOURCE_DIR}/thirdparty/lib
        /usr/local/lib
    )
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # Precompiled Linux static libs live under lib/linux/<arch>/.
        list(APPEND _compressonator_search_paths
            ${PROJECT_SOURCE_DIR}/lib/linux/${CMAKE_SYSTEM_PROCESSOR}
            ${PROJECT_SOURCE_DIR}/lib/linux/x86_64
        )
    elseif(APPLE)
        # macOS static libs live under lib/darwin/<arch>/ (built from source in CI).
        list(APPEND _compressonator_search_paths
            ${PROJECT_SOURCE_DIR}/lib/darwin/${CMAKE_SYSTEM_PROCESSOR}
            ${PROJECT_SOURCE_DIR}/lib/darwin/arm64
            ${PROJECT_SOURCE_DIR}/lib/darwin/x86_64
        )
    endif()

    if(WIN32)
        # The bundled Windows libs are named `Compressonator_MT{d}.lib` and are built with the static MSVC runtime (/MT).
        # If this project is built with the dynamic runtime (/MD), linking will fail with LNK2038.
        set(_vtflib_msvc_runtime_is_dynamic TRUE)
        if(MSVC)
            get_target_property(_vtflib_msvc_runtime vtflib MSVC_RUNTIME_LIBRARY)
            if(NOT _vtflib_msvc_runtime)
                set(_vtflib_msvc_runtime "${CMAKE_MSVC_RUNTIME_LIBRARY}")
            endif()

            # Heuristic: treat any runtime string containing "DLL" as /MD, and plain "MultiThreaded" as /MT.
            if(NOT _vtflib_msvc_runtime)
                set(_vtflib_msvc_runtime_is_dynamic TRUE)
            elseif(_vtflib_msvc_runtime MATCHES "DLL")
                set(_vtflib_msvc_runtime_is_dynamic TRUE)
            else()
                set(_vtflib_msvc_runtime_is_dynamic FALSE)
            endif()
        endif()

        if(_vtflib_msvc_runtime_is_dynamic)
            # Prefer a /MD-built Compressonator if one is provided by the user/toolchain.
            find_library(COMPRESSONATOR_LIB_RELEASE NAMES Compressonator compressonator
                PATHS ${_compressonator_search_paths}
            )
            find_library(COMPRESSONATOR_LIB_DEBUG NAMES Compressonatord Compressonator_d compressonatord Compressonator compressonator
                PATHS ${_compressonator_search_paths}
            )
            if(COMPRESSONATOR_LIB_RELEASE OR COMPRESSONATOR_LIB_DEBUG)
                if(COMPRESSONATOR_LIB_RELEASE AND COMPRESSONATOR_LIB_DEBUG)
                    target_link_libraries(vtflib PRIVATE
                        optimized ${COMPRESSONATOR_LIB_RELEASE}
                        debug ${COMPRESSONATOR_LIB_DEBUG}
                    )
                else()
                    target_link_libraries(vtflib PRIVATE ${COMPRESSONATOR_LIB_RELEASE} ${COMPRESSONATOR_LIB_DEBUG})
                endif()
                set(_vtflib_has_compressonator ON)
            else()
                message(WARNING "VTFLIB_USE_COMPRESSONATOR is ON, but no /MD-compatible Compressonator library was found (the bundled ones are /MT); disabling Compressonator to avoid linker errors.")
                set(_vtflib_compressonator_intentionally_disabled ON)
            endif()
        else()
            find_library(COMPRESSONATOR_LIB_RELEASE NAMES Compressonator_MT Compressonator compressonator
                PATHS ${_compressonator_search_paths}
            )
            find_library(COMPRESSONATOR_LIB_DEBUG NAMES Compressonator_MTd Compressonator compressonator
                PATHS ${_compressonator_search_paths}
            )
            if(COMPRESSONATOR_LIB_RELEASE OR COMPRESSONATOR_LIB_DEBUG)
                if(COMPRESSONATOR_LIB_RELEASE AND COMPRESSONATOR_LIB_DEBUG)
                    target_link_libraries(vtflib PRIVATE
                        optimized ${COMPRESSONATOR_LIB_RELEASE}
                        debug ${COMPRESSONATOR_LIB_DEBUG}
                    )
                else()
                    target_link_libraries(vtflib PRIVATE ${COMPRESSONATOR_LIB_RELEASE} ${COMPRESSONATOR_LIB_DEBUG})
                endif()
                set(_vtflib_has_compressonator ON)
            endif()
        endif()
    else()
        find_library(COMPRESSONATOR_LIB NAMES Compressonator compressonator CMP_Compressonator
            PATHS ${_compressonator_search_paths}
        )
        if(COMPRESSONATOR_LIB)
            target_link_libraries(vtflib PRIVATE ${COMPRESSONATOR_LIB})
            set(_vtflib_has_compressonator ON)
        endif()
    endif()

    if(_vtflib_has_compressonator)
        # Link CMP_Framework if available (needed by Yellow-Dog-Man fork builds)
        find_library(CMP_FRAMEWORK_LIB NAMES CMP_Framework
            PATHS ${_compressonator_search_paths}
        )
        if(CMP_FRAMEWORK_LIB)
            target_link_libraries(vtflib PRIVATE ${CMP_FRAMEWORK_LIB})
        endif()
        # Link all CMP_Core variants (base, SSE, AVX)
        find_library(CMP_CORE_LIB NAMES CMP_Core
            PATHS ${_compressonator_search_paths}
        )
        find_library(CMP_CORE_SSE_LIB NAMES CMP_Core_SSE
            PATHS ${_compressonator_search_paths}
        )
        find_library(CMP_CORE_AVX_LIB NAMES CMP_Core_AVX
            PATHS ${_compressonator_search_paths}
        )
        find_library(CMP_CORE_AVX512_LIB NAMES CMP_Core_AVX512
            PATHS ${_compressonator_search_paths}
        )
        if(CMP_CORE_LIB)
            target_link_libraries(vtflib PRIVATE ${CMP_CORE_LIB})
        endif()
        if(CMP_CORE_SSE_LIB)
            target_link_libraries(vtflib PRIVATE ${CMP_CORE_SSE_LIB})
        endif()
        if(CMP_CORE_AVX_LIB)
            target_link_libraries(vtflib PRIVATE ${CMP_CORE_AVX_LIB})
        endif()
        if(CMP_CORE_AVX512_LIB)
            target_link_libraries(vtflib PRIVATE ${CMP_CORE_AVX512_LIB})
        endif()
        target_compile_definitions(vtflib PRIVATE VTFLIB_HAS_COMPRESSONATOR)
        target_include_directories(vtflib PRIVATE ${PROJECT_SOURCE_DIR}/lib/include)
    else()
        if(NOT _vtflib_compressonator_intentionally_disabled)
            message(WARNING "Compressonator library not found; DXT compression will be disabled.")
        endif()
    endif()
endif()

if(NOT WIN32)
    target_link_libraries(vtflib PRIVATE m)
endif()
