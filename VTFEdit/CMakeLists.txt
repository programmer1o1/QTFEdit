set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 QUIET COMPONENTS Widgets)
if(Qt6_FOUND)
    set(VTFEDITQT_QT_MAJOR 6)
else()
    find_package(Qt5 QUIET COMPONENTS Widgets)
    if(Qt5_FOUND)
        set(VTFEDITQT_QT_MAJOR 5)
    endif()
endif()

if(NOT VTFEDITQT_QT_MAJOR)
    message(FATAL_ERROR "Qt 6/5 Widgets not found. Install Qt (e.g. qt6-base-dev) or set CMAKE_PREFIX_PATH to your Qt installation.")
endif()

add_executable(vtfeditqt
    MACOSX_BUNDLE
    main.cpp
    BatchConvertDialog.h
    BatchConvertDialog.cpp
    CommandPaletteDialog.h
    CommandPaletteDialog.cpp
    CrashReporter.h
    CrashReporter.cpp
    BatchConvertRunDialog.h
    BatchConvertRunDialog.cpp
    CreateVmtDialog.h
    CreateVmtDialog.cpp
    CreateVtfDialog.h
    CreateVtfDialog.cpp
    GuiImageLoader.h
    GuiImageLoader.cpp
    Toast.h
    Toast.cpp
    VtfEditCommands.h
    VtfEditCommands.cpp
    ${CMAKE_SOURCE_DIR}/VTFCmd/tinyexr_wrapper.cpp
    QtUtil.h
    QtUtil.cpp
    VtfPropertiesDialog.h
    VtfPropertiesDialog.cpp
    VtfFlagsDialog.h
    VtfFlagsDialog.cpp
    VtfResourcesDialog.h
    VtfResourcesDialog.cpp
    VtfSheet.h
    VtfSheet.cpp
    VtfSheetDialog.h
    VtfSheetDialog.cpp
    VmtHighlighter.h
    VmtHighlighter.cpp
    ResourceHexDialog.h
    ResourceHexDialog.cpp
    OptionsDialog.h
    OptionsDialog.cpp
    WindowsVtfFormats.h
    WadFile.h
    WadFile.cpp
    WadConvertDialog.h
    WadConvertDialog.cpp
    MainWindow.h
    MainWindow.cpp
)

set_target_properties(vtfeditqt PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "QTFEdit"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.qtfedit.vtfeditqt"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_INFO_STRING "QTFEdit - VTF/VMT Editor"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/MacInfo.plist.in"
)

target_include_directories(vtfeditqt
    PRIVATE
        ${PROJECT_SOURCE_DIR}/thirdparty/include
        ${PROJECT_SOURCE_DIR}/VTFCmd
)

find_package(ZLIB REQUIRED)

target_link_libraries(vtfeditqt
    PRIVATE
        vtflib
        Qt${VTFEDITQT_QT_MAJOR}::Widgets
        ZLIB::ZLIB
)

target_compile_features(vtfeditqt PRIVATE cxx_std_17)

target_compile_definitions(vtfeditqt PRIVATE
    QTFEDIT_VERSION="${PROJECT_VERSION}"
)

if(MSVC)
    target_compile_options(vtfeditqt PRIVATE /W4)
else()
    target_compile_options(vtfeditqt PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS vtfeditqt
    RUNTIME DESTINATION .
    BUNDLE DESTINATION .
)
