cmake_minimum_required(VERSION 3.5)
project(InkStyle VERSION 0.1 LANGUAGES CXX)
set(EXE_NAME inkstyle)

if(${CMAKE_BUILD_TYPE} MATCHES Debug)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

# SOURCES ######################################################################
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Svg REQUIRED)

set(PROJECT_SOURCES
    # Sources
    src/main.cpp
    src/button.cpp
    src/hiddenbutton.cpp
    src/main.cpp
    src/panel.cpp
    src/config.cpp
    src/utils.cpp
    src/texeditor.cpp
    src/visitorpattern.hpp
    src/runguard.hpp
    # Headers
    src/button.hpp
    src/hiddenbutton.hpp
    src/panel.hpp
    src/config.hpp
    src/constants.hpp
    src/utils.hpp
    src/texeditor.hpp
    src/runguard.cpp
    # Configs
    src/global.hpp.in
    # Resources
    res.qrc)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${EXE_NAME}
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES})
else()
    add_executable(${EXE_NAME} ${PROJECT_SOURCES})
endif()

set_target_properties(${EXE_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${EXE_NAME})
endif()

configure_file(src/global.hpp.in src/global.hpp)

# DEPENDENCIES ################################################################
include(ExternalProject)

add_subdirectory(3rdparty/QHotkey)
add_subdirectory(3rdparty/yaml-cpp)

# requires cargo to build resvg
find_program(CARGO cargo REQUIRED)
ExternalProject_Add(
    resvg PREFIX ${CMAKE_BINARY_DIR}/3rdparty
    URL ${CMAKE_SOURCE_DIR}/3rdparty/resvg
    CONFIGURE_COMMAND cargo fetch --locked
    BUILD_COMMAND cargo build --workspace --frozen --release --all-features
    BUILD_IN_SOURCE ON
    INSTALL_COMMAND "")
ExternalProject_Get_property(resvg SOURCE_DIR)
message("SOURCE DIR IS ${SOURCE_DIR}")
set(resvg_INCLUDE_DIRS ${SOURCE_DIR}/c-api)
set(resvg_LIBRARY_DIRS ${SOURCE_DIR}/target/release)
set(resvg_LIBRARIES -lresvg)

add_dependencies(${EXE_NAME} resvg qhotkey yaml-cpp)

if(UNIX AND NOT APPLE)
    # Require x11 library to send keys to inkscape
    find_package(PkgConfig)
    pkg_check_modules(x11 REQUIRED)
endif()

target_include_directories(
    ${EXE_NAME} PRIVATE
    ${CMAKE_BINARY_DIR}/src
    ${x11_INCLUDE_DIRS}
    ${resvg_INCLUDE_DIRS})

target_link_directories(
    ${EXE_NAME} PRIVATE
    ${resvg_LIBRARY_DIRS})

target_link_libraries(
    ${EXE_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Svg
    qhotkey yaml-cpp ${x11_LDFLAGS} ${resvg_LIBRARIES})
