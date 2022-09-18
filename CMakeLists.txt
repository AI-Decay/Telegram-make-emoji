cmake_minimum_required(VERSION 3.18)
set(FFmmpeg_PATH "E:/Libs/FFmpeg")
set(FFmmpeg_INCLUDE "${FFmmpeg_PATH}/include")
set(FFmmpeg_LIB "${FFmmpeg_PATH}/bin")

project(TgGifToEmoji VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_FLAGS "-D__STDC_CONSTANT_MACROS")

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        src/main/main.cpp
        src/main/MainWindow.cpp
        src/main/MainWindow.h
        src/utility/StyleSheetUtility.h
        src/model/ConvertItemListModel.h
        src/model/ConvertItemListModel.cpp
        src/model/ConvertItem.h
        src/model/ConvertItem.cpp
        src/converter/ToWebmConvertor.h
        src/converter/ToWebmConvertor.cpp
        src/model/ConvertItemDelegate.h
        src/model/ConvertItemDelegate.cpp
        src/utility/MultiIndex.h
        src/custom/InputSliderWidget.h
        src/custom/InputSliderWidget.cpp
        src/utility/FFmpegUtility.h
        src/utility/FFmpegUtility.cpp
)

find_path(SWSCALE_INCLUDE_DIR PATHS ${FFmmpeg_INCLUDE}/libavcodec NAMES swscale.h)
find_path(AVCODEC_INCLUDE_DIR PATHS ${FFmmpeg_INCLUDE}/libavcodec NAMES avcodec.h)
find_path(AVFORMAT_INCLUDE_DIR PATHS ${FFmmpeg_INCLUDE}/libavformat NAMES avformat.h)
find_path(AVUTIL_INCLUDE_DIR PATHS ${FFmmpeg_INCLUDE}/libavutil NAMES avutil.h)
find_path(AVDEVICE_INCLUDE_DIR PATHS ${FFmmpeg_INCLUDE}/libavdevice NAMES avdevice.h)

find_library(SWSCALE_LIBRARY PATHS ${FFmmpeg_LIB} NAMES swscale)
find_library(AVCODEC_LIBRARY PATHS ${FFmmpeg_LIB} NAMES avcodec)
find_library(AVFORMAT_LIBRARY PATHS ${FFmmpeg_LIB} NAMES avformat)
find_library(AVUTIL_LIBRARY PATHS ${FFmmpeg_LIB} NAMES avutil)
find_library(AVDEVICE_LIBRARY PATHS ${FFmmpeg_LIB} NAMES avdevice)

add_executable(TgGifToEmoji ${PROJECT_SOURCES} resources.qrc)
target_include_directories(TgGifToEmoji PRIVATE "src")
target_include_directories(TgGifToEmoji PRIVATE ${FFmmpeg_INCLUDE})
target_link_libraries(TgGifToEmoji PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(TgGifToEmoji PRIVATE ${AVCODEC_LIBRARY} ${AVFORMAT_LIBRARY} ${AVUTIL_LIBRARY} ${AVDEVICE_LIBRARY} ${SWSCALE_LIBRARY})
