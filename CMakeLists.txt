#########################
##
## Description: Main Compilation file for Checkout Project
##
## Copyright (c) Wiest-Daessle Nicolas, 2015
## Licensing details shall be checked in the "License.txt" belonging to this project
##
##########################



cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)

project(Checkout)


set(Checkout_VERSION_MAJOR 1)
set(Checkout_VERSION_MINOR 4)
set(Checkout_VERSION_COMPLEMENT R)
set(Checkout_VERSION_RELEASE 9)

option(BUILD_PLUGINS "Build plugins" ON)
option(BUILD_PYTHON_SCRIPTING "Build with Core side Python Scripting" OFF)
option(BUILD_WITH_VTK "Build the VTK optional stuff" OFF)
option(BUILD_RAW_PROCESSOR "Build the Raw data processor (very advanced users - developper feature)" OFF)
option(PLUGIN_IN_CORE "Shall Core call plugins" OFF)

#option(PLUGIN_NWD_ONLY "Build only WD plugins" OFF)

set(PLUGIN_NWD_OLY OFF)


set(PLUGIN_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../CheckoutPlugins")


set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Update to CMake
find_package(Qt5 COMPONENTS Widgets Xml XmlPatterns OpenGL Concurrent Sql WinExtras REQUIRED)

# find_package(Qt5Widgets REQUIRED)
# find_package(Qt5Xml REQUIRED)
# find_package(Qt5XmlPatterns REQUIRED)
# find_package(Qt5OpenGL REQUIRED)
#find_package(Qt5WebKitWidgets REQUIRED)
# find_package(Qt5Concurrent REQUIRED)
# find_package(Qt5Sql REQUIRED)
# find_package(Qt5WinExtras REQUIRED)


#find_package(Qt5Test REQUIRED)

#include(DeployQt5.cmake)

#find_package(OpenCV REQUIRED core highgui)

find_package(OpenCV REQUIRED )

message(STATUS "Found OpenCV: ${OpenCV_INCLUDE_DIRS} ${OpenCV_LIBS} ")

include_directories( ${OpenCV_INCLUDE_DIRS})

if(BUILD_WITH_VTK)
    find_package(VTK)
    if(VTK_FOUND)
        message(STATUS "With VTK ${VTK_INCLUDE_DIRS}")
        add_definitions(-DCheckout_With_VTK)
        include_directories( ${VTK_INCLUDE_DIRS})
    endif(VTK_FOUND)
endif(BUILD_WITH_VTK)

set(CheckoutLibs CheckoutCore)

if(BUILD_PYTHON_SCRIPTING)
# Need to search for anaconda
    Find_package(PythonLibs REQUIRED)
    find_package(PythonInterp REQUIRED)

    add_definitions(-DCheckoutCoreWithPython)
    add_definitions(-DCheckoutServerWithPython)


    include_directories(${PYTHON_INCLUDE_DIRS})
    set(CheckoutLibs ${CheckoutLibs} ${PYTHON_LIBRARIES} )
    LINK_DIRECTORIES(${PYTHON_LIBRARIES})
    message(STATUS "Found Python " ${PYTHON_LIBRARIES} ${PYTHON_LIBRARY})
    find_path(NUMPY_CORE_INCLUDE numpy/ndarrayobject.h)
    INCLUDE_DIRECTORIES(${NUMPY_CORE_INCLUDE})
    message(STATUS "Found Numpy ${NUMPY_CORE_INCLUDE}")
    find_file(NUMPY_CORE_LIB npymath.lib)
    message(STATUS "Found Numpy Lib ${NUMPY_CORE_LIB}")

     set(CheckoutPythonLibs  ${NUMPY_CORE_LIB} ${CheckoutLibs} )

endif(BUILD_PYTHON_SCRIPTING)

#find_package(Git REQUIRED)

#if(GIT_FOUND)
#    GIT_WC_INFO(${PROJECT_SOURCE_DIR} Checkout)
#    message("Current revision is  ${Checkout_WC_REVISION_HASH}")
#endif()

#DEFINE_SYMBOL()

set(CheckoutCore_lib_path ${OpenCV_LIB_DIR} )
link_directories(${OpenCV_LIB_DIR})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories( ${CMAKE_SOURCE_DIR} )
include_directories( ${CMAKE_SOURCE_DIR}/include)

configure_file(
    "${PROJECT_SOURCE_DIR}/Core/config.h.in"
    "${PROJECT_BINARY_DIR}/Core/config.h"
)

configure_file(
    "${PROJECT_SOURCE_DIR}/CMake/UseCheckoutCore.cmake.in"
    "${PROJECT_BINARY_DIR}/CMake/UseCheckoutCore.cmake"
)

configure_file(
    Installer.nsi.in
    Installer.nsi
)
configure_file(
    Installer_vs2015.nsi.in
    Installer_vs2015.nsi
)

include_directories(${PROJECT_BINARY_DIR})

if(PLUGIN_IN_CORE)
    add_definitions(-DCheckoutPluginInCore)
endif(PLUGIN_IN_CORE)

#add_subdirectory(PluginManager)
add_subdirectory(Core)
add_subdirectory(Gui)

if(BUILD_PYTHON_SCRIPTING)
    add_subdirectory(Python)
#else(BUILD_PYTHON_SCRIPTING)
endif(BUILD_PYTHON_SCRIPTING)

add_subdirectory(Server)
add_subdirectory(Main)

if(BUILD_PLUGINS)
    add_subdirectory(Plugins)
endif(BUILD_PLUGINS)

if(BUILD_RAW_PROCESSOR)
    add_subdirectory(RawProcessor)
endif(BUILD_RAW_PROCESSOR)

add_subdirectory(Testing)


# CPack Installer package
include (InstallRequiredSystemLibraries)
set (CPACK_RESOURCE_FILE_LICENSE
     "${CMAKE_CURRENT_SOURCE_DIR}/License.txt")
set (CPACK_PACKAGE_VERSION_MAJOR "${Checkout_VERSION_MAJOR}")
set (CPACK_PACKAGE_VERSION_MINOR "${Checkout_VERSION_MINOR}.${Checkout_VERSION_COMPLEMENT}")
include (CPack)


#include ()
