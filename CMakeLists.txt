cmake_minimum_required(VERSION 3.27)

if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
    set(DEBUG "#define DEBUG")
    string(TIMESTAMP VERSION "%Y.%m.%d")
else ()
    if (${CMAKE_BUILD_TYPE} MATCHES "Release")
        set(CMAKE_INSTALL_PREFIX /usr)
    endif ()
    set(DEBUG "//#define DEBUG")
    file(READ "${CMAKE_SOURCE_DIR}/src/Version.hpp" VERSION_FILE_CONTENT)
    string(REGEX MATCH "define VERSION \"([^\"]+)" VERSION_MATCH "${VERSION_FILE_CONTENT}")
    set(VERSION ${CMAKE_MATCH_1})
endif ()

set(PRG_NAME Klass)
project(${PRG_NAME} LANGUAGES CXX VERSION ${VERSION})

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(QAPPLICATION_CLASS QApplication CACHE STRING "Inheritance class for SingleApplication")
configure_file("${CMAKE_SOURCE_DIR}/src/Version.hpp.in" "${CMAKE_SOURCE_DIR}/src/Version.hpp" @ONLY)

find_package(Qt6 REQUIRED COMPONENTS
        Core
        Gui
        Svg
        Widgets
        LinguistTools
)

set(TS_FILES lang/klass_pt_BR.ts)

set_source_files_properties(${TS_FILES} PROPERTIES
        OUTPUT_LOCATION "${CMAKE_SOURCE_DIR}/lang"
)

qt6_add_translation(QM_FILES ${TS_FILES})

find_package(ZLIB REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBNOTIFY REQUIRED libnotify)

add_subdirectory("src/SingleApplication")
include_directories(
        src
        src/Helper
        src/PackageUtils
        src/Style
        src/UI
        src/UI/MirrorManager
        src/UI/RulesDialog
        src/UI/TableView
)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
        src/*
        src/Helper/*
        src/PackageUtils/*
        src/Style/*
        src/UI/*
        src/UI/MirrorManager/*
        src/UI/RulesDialog/*
        src/UI/TableView/*
)

add_executable(${PRG_NAME} ${SRC_FILES})

set_property(TARGET ${PRG_NAME} PROPERTY OUTPUT_NAME "klass")
set_property(TARGET ${PRG_NAME} PROPERTY LINK_FLAGS "-Wl,-s")
target_compile_options(${PRG_NAME} PRIVATE -Os -finline-functions -ftree-vectorize)

include(CheckIPOSupported)
check_ipo_supported(RESULT supported OUTPUT error)

if (supported)
    message(STATUS "IPO / LTO enabled")
    set_property(TARGET ${PRG_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else ()
    message(STATUS "IPO / LTO not supported: <${error}>")
endif ()

target_link_libraries(${PRG_NAME} PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Svg
        Qt6::Widgets
        SingleApplication::SingleApplication
        ZLIB::ZLIB
        util
        ${LIBNOTIFY_LIBRARIES}
)

target_include_directories(${PRG_NAME} SYSTEM PRIVATE
        ${LIBNOTIFY_INCLUDE_DIRS}
)

add_custom_target(lang DEPENDS ${QM_FILES})
add_dependencies(${PRG_NAME} lang)

set(SHARE_DIR "share/klass")
install(TARGETS ${PRG_NAME} RUNTIME DESTINATION bin)
install(FILES ${QM_FILES} DESTINATION "${SHARE_DIR}/lang")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/mirrors" DESTINATION "${SHARE_DIR}")
install(FILES "${CMAKE_SOURCE_DIR}/klass.png" DESTINATION "${SHARE_DIR}")
install(FILES "${CMAKE_SOURCE_DIR}/klass.desktop" DESTINATION "share/applications")
install(FILES "${CMAKE_SOURCE_DIR}/klass.png" DESTINATION "share/pixmaps")

install(DIRECTORY "${CMAKE_SOURCE_DIR}/icons"
        DESTINATION "${SHARE_DIR}"
        FILES_MATCHING PATTERN "*.svg" PATTERN "*.png"
)
