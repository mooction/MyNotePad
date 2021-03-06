﻿cmake_minimum_required(VERSION 3.11)
project(mynotepad)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
#add_definitions("$<$<CONFIG:Debug>:-D_DEBUG>")

if (UNIX)
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    # set build type and flags
    string(CONCAT CMAKE_CXX_FLAGS_DEBUG
        "-Wall "            # display all warnings
        "-g "               # with debug information
        "-O0 "               # no optimize
        "-Wno-parentheses"   # disable parentheses warning
    )
    string(CONCAT CMAKE_CXX_FLAGS_RELEASE
        "-O3"               # enable optimize
    )
    if (CMAKE_BUILD_TYPE STREQUAL "")
        set(CMAKE_BUILD_TYPE "Release")
    endif()
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
    message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
endif()

set(MYNOTEPAD_VERSION_MAJOR 1)
set(MYNOTEPAD_VERSION_MINOR 0)
set(MYNOTEPAD_VERSION_PATCH 6)
message(STATUS "MyNotePad version: ${MYNOTEPAD_VERSION_MAJOR}."
        "${MYNOTEPAD_VERSION_MINOR}.${MYNOTEPAD_VERSION_PATCH}")

set(MYNOTEPAD_INSTALL_PATH "${CMAKE_INSTALL_PREFIX}/share")
configure_file(
    "${PROJECT_SOURCE_DIR}/inc/config.h.in"
    "${PROJECT_BINARY_DIR}/inc/config.h"
)

if (UNIX)
    configure_file(
        "${PROJECT_SOURCE_DIR}/static/mynotepad.desktop.in"
        "${PROJECT_SOURCE_DIR}/static/mynotepad.desktop"
    )
endif()

# specify the path manually
if (WIN32)
    #set(wxWidgets_ROOT_DIR "D:/wxWidgets-3.1.3/")
    #set(wxWidgets_LIB_DIR "D:/wxWidgets-3.1.3/lib/vc14x_dll")
endif()
set(wxWidgets_CONFIGURATION mswu) # or mswud

# find wxWidgets
find_package(wxWidgets COMPONENTS core base stc REQUIRED)
message(STATUS "wxWidgets version: ${wxWidgets_VERSION_MAJOR}."
        "${wxWidgets_VERSION_MINOR}.${wxWidgets_VERSION_PATCH}")

include(${wxWidgets_USE_FILE})
set(EXTRA_LIBS ${EXTRA_LIBS} ${wxWidgets_LIBRARIES})

# define header directory
include_directories(
    "${PROJECT_BINARY_DIR}/inc"
    "${PROJECT_SOURCE_DIR}/inc"
    "${PROJECT_SOURCE_DIR}/static"
    "${PROJECT_SOURCE_DIR}/md2html/inc"
)
# define source directory
aux_source_directory(src SOURCES)

if (WIN32)
    # add resource file for Windows
    list(APPEND SOURCES "static/wx.rc")
    list(APPEND SOURCES "static/mynotepad.manifest")
endif()

# target executable file
set(TARGET mynotepad)
add_executable(${TARGET} WIN32 ${SOURCES})

# Markdown to HTML converter
option(USE_EXTERN_LIB "Use external library to convert markdown to HTML" OFF)
if (USE_EXTERN_LIB)
    include(FetchContent)
    FetchContent_Declare(maddy
        GIT_REPOSITORY https://github.com/progsource/maddy.git
        GIT_TAG f3d934d6ec70bd4c077acfb810026d5f15e11001
    )
    FetchContent_GetProperties(maddy)
    if (NOT maddy_POPULATED)
        FetchContent_Populate(maddy)
        # Do not add_subdirectory() because we only want the header files
        include_directories("${maddy_SOURCE_DIR}/include")
    endif()
    add_definitions("-DUSE_EXTERN_LIB")
else()
    add_subdirectory(md2html)
    set(EXTRA_LIBS ${EXTRA_LIBS} md2html)
endif()

# link libraries
target_link_libraries(${TARGET} ${EXTRA_LIBS})

if (WIN32)
    set(WXVER "${wxWidgets_VERSION_MAJOR}${wxWidgets_VERSION_MINOR}${wxWidgets_VERSION_PATCH}")
    # required DLLs on Windows
    set(ALL_DLLS_DEBUG
        "${wxWidgets_LIB_DIR}/wxbase${WXVER}ud_vc14x_x64.dll"
        "${wxWidgets_LIB_DIR}/wxbase${WXVER}ud_net_vc14x_x64.dll"
        "${wxWidgets_LIB_DIR}/wxbase${WXVER}ud_xml_vc14x_x64.dll"
        "${wxWidgets_LIB_DIR}/wxmsw${WXVER}ud_core_vc14x_x64.dll"
        "${wxWidgets_LIB_DIR}/wxmsw${WXVER}ud_stc_vc14x_x64.dll"
        )
    set(ALL_DLLS_RELEASE
        "${wxWidgets_LIB_DIR}/wxbase${WXVER}u_vc14x_x64.dll"
        "${wxWidgets_LIB_DIR}/wxbase${WXVER}u_net_vc14x_x64.dll"
        "${wxWidgets_LIB_DIR}/wxbase${WXVER}u_xml_vc14x_x64.dll"
        "${wxWidgets_LIB_DIR}/wxmsw${WXVER}u_core_vc14x_x64.dll"
        "${wxWidgets_LIB_DIR}/wxmsw${WXVER}u_stc_vc14x_x64.dll"
        )
    set(ALL_DLLS $<$<CONFIG:Debug>:${ALL_DLLS_DEBUG}>$<$<CONFIG:Release>:${ALL_DLLS_RELEASE}>)
    #message(STATUS ${ALL_DLLS})
    set(STATIC_RESOURCES
        "${PROJECT_SOURCE_DIR}/static/highlight.css"
        "${PROJECT_SOURCE_DIR}/static/highlight.pack.js"
        "${PROJECT_SOURCE_DIR}/static/style.css"
        "${PROJECT_SOURCE_DIR}/static/logo.png"
        "${PROJECT_SOURCE_DIR}/static/mynotepad.visualelementsmanifest.xml"
        )
    # destination directory
    set(COPY_DLLS_TO ${PROJECT_BINARY_DIR}/)
    #message(STATUS ${COPY_DLLS_TO})
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND_EXPAND_LISTS
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ALL_DLLS}" "${COPY_DLLS_TO}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${STATIC_RESOURCES}" "${COPY_DLLS_TO}"
        COMMENT "Copying DLLs" VERBATIM
        )

elseif (UNIX)
    # default path is /usr/local/
    message(STATUS "CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")

    # install the program after build, /usr/local/bin/
    install(TARGETS ${TARGET} RUNTIME DESTINATION bin)
    
    # copy dependent files while installing, default path is /usr/local/share
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/static/"
            DESTINATION "share/${PROJECT_NAME}"
            # make the files writable
            FILE_PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ GROUP_WRITE WORLD_READ WORLD_WRITE
            FILES_MATCHING PATTERN "*.css" PATTERN "*.js" PATTERN "*.png"
            )
    
    # create desktop icon
    install(FILES "${PROJECT_SOURCE_DIR}/static/mynotepad.desktop"
            DESTINATION /usr/share/applications
            PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
            )
endif()
