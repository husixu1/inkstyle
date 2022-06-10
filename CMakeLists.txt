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

find_package(QT NAMES Qt5 REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Svg REQUIRED)
if(UNIX)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS X11Extras REQUIRED)
endif()

set(PROJECT_SOURCES
    # Sources
    src/main.cpp
    src/button.cpp
    src/hiddenbutton.cpp
    src/main.cpp
    src/panel.cpp
    src/buttoninfo.cpp
    src/config.cpp
    src/configs.cpp
    src/utils.cpp
    src/texeditor.cpp
    src/runguard.cpp

    # Headers
    src/button.hpp
    src/hiddenbutton.hpp
    src/panel.hpp
    src/buttoninfo.hpp
    src/config.hpp
    src/configs.hpp
    src/utils.hpp
    src/texeditor.hpp
    src/runguard.hpp
    src/constants.hpp
    src/visitorpattern.hpp
    src/nonaccessiblewidget.hpp

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

add_subdirectory(3rdparty/yaml-cpp EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/pugixml EXCLUDE_FROM_ALL)

# Use a patched version of QHotkey to workaround some Linux bugs
set(qhotkey_PATCH_COMMAND "")
if(UNIX)
    set(qhotkey_PATCH_COMMAND patch -p1 < ${CMAKE_SOURCE_DIR}/patch/qhotkey.patch)
endif()
ExternalProject_Add(
    qhotkey PREFIX ${CMAKE_BINARY_DIR}/3rdparty
    URL ${CMAKE_SOURCE_DIR}/3rdparty/QHotkey
    PATCH_COMMAND ${qhotkey_PATCH_COMMAND}
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
)
ExternalProject_Get_property(qhotkey INSTALL_DIR)
set(qhotkey_INCLUDE_DIRS ${INSTALL_DIR}/include)
set(qhotkey_LIBRARY_DIRS ${INSTALL_DIR}/lib)
if(UNIX)
    set(qhotkey_LIBRARIES -lqhotkey)
elseif(WIN32)
    set(qhotkey_LIBRARIES qhotkey.lib)
endif()

# Requires cargo to build resvg
find_program(CARGO cargo REQUIRED)
ExternalProject_Add(
    resvg PREFIX ${CMAKE_BINARY_DIR}/3rdparty
    URL ${CMAKE_SOURCE_DIR}/3rdparty/resvg
    CONFIGURE_COMMAND ${CARGO} fetch --locked
    BUILD_COMMAND ${CARGO} build --workspace --frozen --release --all-features
    BUILD_IN_SOURCE ON
    INSTALL_COMMAND "")
ExternalProject_Get_property(resvg SOURCE_DIR)
message("SOURCE DIR IS ${SOURCE_DIR}")
set(resvg_INCLUDE_DIRS ${SOURCE_DIR}/c-api)
set(resvg_LIBRARY_DIRS ${SOURCE_DIR}/target/release)
if(UNIX)
    set(resvg_LIBRARIES -Wl,-Bstatic -lresvg -Wl,-Bdynamic)
elseif(WIN32)
    set(resvg_LIBRARIES resvg.dll wsock32 ws2_32 bcrypt userenv)
endif()
add_dependencies(${EXE_NAME} resvg qhotkey yaml-cpp)

if(UNIX AND NOT APPLE)
    # Require x11 library to send keys to inkscape
    find_package(X11 REQUIRED)
    # qhotkey depends on Qt's X11Extras
    list(APPEND qhotkey_LIBRARIES Qt5::X11Extras)
endif()

target_include_directories(
    ${EXE_NAME} PRIVATE
    ${CMAKE_BINARY_DIR}/src
    ${x11_INCLUDE_DIRS}
    ${qhotkey_INCLUDE_DIRS}
    ${resvg_INCLUDE_DIRS})

target_link_directories(
    ${EXE_NAME} PRIVATE
    ${qhotkey_LIBRARY_DIRS}
    ${resvg_LIBRARY_DIRS})

target_link_libraries(
    ${EXE_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Svg
    yaml-cpp
    pugixml
    ${resvg_LIBRARIES}
    ${qhotkey_LIBRARIES}
    ${X11_LIBRARIES})

# INSTALLATION ################################################################
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY ON)
install(TARGETS ${EXE_NAME} DESTINATION bin)
