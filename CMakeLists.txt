cmake_minimum_required(VERSION 3.5)

project(TgGifToEmoji VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        main.cpp
        mainWindow.cpp
        mainWindow.h
        styleSheetUtility.h
        filesListModel.h
        filesListModel.cpp
        file.h
        file.cpp
)

add_executable(TgGifToEmoji ${PROJECT_SOURCES} resources.qrc)

target_link_libraries(TgGifToEmoji PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(TgGifToEmoji PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)
