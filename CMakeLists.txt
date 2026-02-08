cmake_minimum_required(VERSION 3.16)
project(HazbinDownloaded VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_PREFIX_PATH "C:/Qt/6.9.1/mingw_64/lib/cmake" CACHE PATH "Qt SDK Path")

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Network Concurrent)
qt_standard_project_setup(REQUIRES 6.8)

if(WIN32)
    include_directories(vendor/miniz)
endif()

# --- Executable Definition ---
qt_add_executable(HazbinDownloaded
    src/main.cpp src/ToolsManager.h src/ToolsManager.cpp
    src/YtDlp.h src/YtDlp.cpp src/YoutubeService.h src/YoutubeService.cpp
    src/SettingsManager.h src/SettingsManager.cpp
)

set(QML_SOURCES
    qml/Main.qml qml/MainContent.qml qml/StyledButton.qml
    qml/Footer.qml qml/Console.qml qml/Settings.qml
)

qt_add_qml_module(HazbinDownloaded
    URI HazbinDownloaded
    VERSION 1.0
    QML_FILES ${QML_SOURCES}
)

set_target_properties(HazbinDownloaded PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(HazbinDownloaded
    PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Network Qt6::Concurrent
)

# --- Installation ---
include(GNUInstallDirs)

install(TARGETS HazbinDownloaded
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# --- Linux Desktop & AppImage Logic ---
if(UNIX AND NOT APPLE)
    set(DESKTOP_FILE "${CMAKE_CURRENT_BINARY_DIR}/hazbindownloaded.desktop")
    file(WRITE ${DESKTOP_FILE}
        "[Desktop Entry]\nName=Hazbin Downloaded\nExec=HazbinDownloaded\nIcon=hazbindownloaded\nType=Application\nCategories=Utility;Multimedia;\n"
    )
    install(FILES ${DESKTOP_FILE} DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)

    # APPIMAGE TARGET
    find_program(LINUXDEPLOY_BIN linuxdeploy)
    if(LINUXDEPLOY_BIN)
        add_custom_target(appimage
            COMMAND ${CMAKE_COMMAND} --install . --prefix ${CMAKE_BINARY_DIR}/AppDir
            COMMAND ${CMAKE_COMMAND} -E env
                DEPLOY_QT_VERSION=6
                QT_QML_SOURCES_DIR=${CMAKE_CURRENT_SOURCE_DIR}/qml
                ${LINUXDEPLOY_BIN} --appdir ${CMAKE_BINARY_DIR}/AppDir --plugin qt --output appimage
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating AppImage..."
        )
    endif()
endif()

# --- CPack Configuration ---
set(CPACK_PACKAGE_NAME "HazbinDownloaded")
set(CPACK_PACKAGE_VENDOR "M U T E")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})

if(WIN32)
    # Windows: Installer + Portable Zip
    set(CPACK_GENERATOR "NSIS;ZIP")

    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    install(CODE "
        execute_process(
            COMMAND \"${WINDEPLOYQT_EXECUTABLE}\"
            --no-translations --compiler-runtime
            --qmldir \"${CMAKE_CURRENT_SOURCE_DIR}/qml\"
            \"\${CMAKE_INSTALL_PREFIX}/bin/HazbinDownloaded.exe\"
        )
    ")

    set(CPACK_NSIS_DISPLAY_NAME "Hazbin Downloaded")
    set(CPACK_NSIS_PACKAGE_NAME "Hazbin Downloaded")
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY "bin")
    set(CPACK_CREATE_DESKTOP_LINKS "HazbinDownloaded")
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
        !define MUI_FINISHPAGE_SHOWREADME_NOT_CHECKED
        RMDir /r '\$INSTDIR\\\\bin\\\\tools'
    ")
    set(CPACK_NSIS_UNINSTALL_NAME "Uninstall")
else()
    # Linux: Just the Portable Tarball (AppImage is handled by the 'appimage' target)
    set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)
