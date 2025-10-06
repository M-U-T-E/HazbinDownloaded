cmake_minimum_required(VERSION 3.16)

project(HazbinDownloaded VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_PREFIX_PATH "C:/Qt/6.9.1/mingw_64/lib/cmake")

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Network Concurrent)

qt_standard_project_setup(REQUIRES 6.8)

# Include miniz only for Windows builds, as it's used for ZIP extraction there
if(WIN32)
    include_directories(vendor/miniz)
endif()

# Executable
qt_add_executable(appHazbinDownloaded
    src/main.cpp
    src/ToolsManager.h
    src/ToolsManager.cpp
    src/YtDlp.h
    src/YtDlp.cpp
    src/YoutubeService.h
    src/YoutubeService.cpp
)

# QML module
set(QML_SOURCES
    qml/Main.qml
    qml/MainContent.qml
    qml/StyledButton.qml
    qml/Footer.qml
    qml/Console.qml
)

qt_add_qml_module(appHazbinDownloaded
    URI HazbinDownloaded
    VERSION 1.0
    QML_FILES ${QML_SOURCES}
)

# Target properties
set_target_properties(appHazbinDownloaded PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# Linking
target_link_libraries(appHazbinDownloaded
    PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Network Qt6::Concurrent
)


# Installation
include(GNUInstallDirs)
install(TARGETS appHazbinDownloaded
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
