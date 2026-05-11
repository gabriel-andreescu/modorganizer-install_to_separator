cmake_minimum_required(VERSION 3.22)

find_package(mo2-cmake CONFIG REQUIRED)
find_package(mo2-uibase CONFIG REQUIRED)
find_package(Qt6 COMPONENTS Widgets REQUIRED)

function(install_to_separator_fix_qt_tool_locations)
    get_filename_component(qt_install_prefix "${Qt6_DIR}/../../.." ABSOLUTE)
    foreach (qt_tool_name
            androiddeployqt
            androidtestrunner
            cmake_automoc_parser
            lconvert
            lrelease
            lupdate
            moc
            qlalr
            qmake
            qmlcachegen
            qmldom
            qmlformat
            qmlimportscanner
            qmljsrootgen
            qmllint
            qmlplugindump
            qmlprofiler
            qmltc
            qmltestrunner
            qmltime
            qmltyperegistrar
            qtpaths
            qvkgen
            rcc
            svgtoqml
            syncqt
            tracegen
            tracepointgen
            uic
            windeployqt)
        set(qt_tool_exe "${qt_install_prefix}/bin/${qt_tool_name}.exe")
        if (EXISTS "${qt_tool_exe}")
            foreach (qt_tool_namespace Qt Qt6)
                if (TARGET ${qt_tool_namespace}::${qt_tool_name})
                    set_target_properties(${qt_tool_namespace}::${qt_tool_name} PROPERTIES
                        IMPORTED_CONFIGURATIONS "Debug;Release;RelWithDebInfo;MinSizeRel"
                        IMPORTED_LOCATION "${qt_tool_exe}"
                        IMPORTED_LOCATION_DEBUG "${qt_tool_exe}"
                        IMPORTED_LOCATION_RELEASE "${qt_tool_exe}"
                        IMPORTED_LOCATION_RELWITHDEBINFO "${qt_tool_exe}"
                        IMPORTED_LOCATION_MINSIZEREL "${qt_tool_exe}")
                endif ()
            endforeach ()
        endif ()
    endforeach ()
endfunction()

install_to_separator_fix_qt_tool_locations()

add_library(install_to_separator SHARED)
mo2_configure_plugin(install_to_separator WARNINGS OFF)
install_to_separator_fix_qt_tool_locations()

# Keep generated Qt MOC object paths short enough for MSVC.
set_target_properties(install_to_separator PROPERTIES
    AUTOGEN_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/autogen")

set(MO2_UIBASE_INCLUDE_DIR "" CACHE PATH
    "Optional uibase include directory to build against a specific MO2 ABI")
if (MO2_UIBASE_INCLUDE_DIR)
    message(STATUS "[install_to_separator] Using MO2 uibase ABI headers: ${MO2_UIBASE_INCLUDE_DIR}")
    target_include_directories(install_to_separator SYSTEM BEFORE PRIVATE "${MO2_UIBASE_INCLUDE_DIR}")
endif ()

target_link_libraries(install_to_separator PRIVATE Qt6::Widgets mo2::uibase)
mo2_install_plugin(install_to_separator)
install_to_separator_fix_qt_tool_locations()

set(INSTALL_TO_SEPARATOR_PACKAGE_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../dist" CACHE PATH
    "Directory where release ZIP files are written")
set(INSTALL_TO_SEPARATOR_PACKAGE_MO2_VERSION "" CACHE STRING
    "MO2 compatibility label appended to the release ZIP name")

set(install_to_separator_package_name "Install.To.Separator")
if (INSTALL_TO_SEPARATOR_PACKAGE_MO2_VERSION)
    string(APPEND install_to_separator_package_name ".${INSTALL_TO_SEPARATOR_PACKAGE_MO2_VERSION}")
endif ()
string(APPEND install_to_separator_package_name ".zip")

add_custom_target(install_to_separator_zip
    COMMAND "${CMAKE_COMMAND}"
        "-DOUTPUT_ZIP=${INSTALL_TO_SEPARATOR_PACKAGE_OUTPUT_DIR}/${install_to_separator_package_name}"
        "-DPLUGIN_DLL=$<TARGET_FILE:install_to_separator>"
        "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/install_to_separator_zip"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/package_install_to_separator.cmake"
    DEPENDS install_to_separator
    COMMENT "Creating ${INSTALL_TO_SEPARATOR_PACKAGE_OUTPUT_DIR}/${install_to_separator_package_name}")
