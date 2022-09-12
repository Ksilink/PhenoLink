#########################
##
## Description: Main Compilation file for Checkout Project
##
## Copyright (c) Wiest-Daessle Nicolas, 2015
## Licensing details shall be checked in the "License.txt" belonging to this project
##
##########################



cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
set(VCPKG_FEATURE_FLAGS versions)
project(Checkout)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)


find_package(Git REQUIRED)

find_package(Arrow CONFIG REQUIRED)

if(WIN32)
else()
    set(CMAKE_CXX_FLAGS "-Wno-deprecated")
    set(CMAKE_C_FLAGS "-Wno-deprecated")
endif(WIN32)

set(Checkout_VERSION_MAJOR 2)
set(Checkout_VERSION_MINOR 1)
set(Checkout_VERSION_COMPLEMENT R)
set(Checkout_VERSION_RELEASE 0)

message(STATUS "GIT : ${GIT_EXECUTABLE} ${CMAKE_SOURCE_DIR}")

set(git_dirty "")
execute_process(
  COMMAND           "${GIT_EXECUTABLE}"
                    rev-parse
                    --short
                    HEAD
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  RESULT_VARIABLE   git_return
  OUTPUT_VARIABLE   app_hash_short)
string(STRIP "${app_hash_short}" app_hash_short)
execute_process(
  COMMAND           "${GIT_EXECUTABLE}"
                    rev-parse
                    --short
                    HEAD
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/../CheckoutPlugins"
  RESULT_VARIABLE   git_return
  OUTPUT_VARIABLE   plugin_hash_short)
string(STRIP "${plugin_hash_short}" plugin_hash_short)

message(STATUS "With App git hash ${app_hash_short}-${plugin_hash_short}${git_dirty}")
set(Checkout_VERSION_RELEASE "${Checkout_VERSION_RELEASE}-${app_hash_short}-${plugin_hash_short}${git_dirty}")

add_definitions(-DCheckout_Git_Hash="${app_hash_short}-${plugin_hash_short}${git_dirty}")


option(BUILD_PLUGINS "Build plugins" ON)
option(BUILD_PYTHON_SCRIPTING "Build with Core side Python Scripting" OFF)
option(BUILD_WITH_VTK "Build the VTK optional stuff" OFF)
option(BUILD_RAW_PROCESSOR "Build the Raw data processor (very advanced users - developper feature)" OFF)
option(PLUGIN_IN_CORE "Shall Core call plugins" OFF)
option(ONNX_GPU "Use ONNX GPU Version" OFF)

if(UNIX AND NOT APPLE)
        set(LINUX TRUE)
endif()

if (WIN32) # do the win32 download
    if (ONNX_GPU)
        set(ONNX_URL "https://github.com/microsoft/onnxruntime/releases/download/v1.11.1/onnxruntime-win-x64-gpu-1.11.1.zip")
        set(ONNX_ROOTDIR "onnxruntime-win-x64-gpu-1.11.1")
        add_definitions(-DUSE_CUDA)
    else()
        set(ONNX_URL "https://github.com/microsoft/onnxruntime/releases/download/v1.11.1/onnxruntime-win-x64-1.11.1.zip")
        set(ONNX_ROOTDIR "onnxruntime-win-x64-1.11.1")
    endif(ONNX_GPU)
    set(ONNX_TARGET "onnxruntime-win.zip")

elseif(LINUX) # do the linux download
    if (ONNX_GPU)
        set(ONNX_URL "https://github.com/microsoft/onnxruntime/releases/download/v1.11.1/onnxruntime-linux-x64-gpu-1.11.1.tgz")
         set(ONNX_ROOTDIR "onnxruntime-linux-x64-gpu-1.11.1")
         add_definitions(-DUSE_CUDA)
    else()
        set(ONNX_URL "https://github.com/microsoft/onnxruntime/releases/download/v1.11.1/onnxruntime-linux-x64-1.11.1.tgz")
        set(ONNX_ROOTDIR "onnxruntime-linux-x64-1.11.1")
    endif(ONNX_GPU)
    set(ONNX_TARGET "onnxruntime-linux.tgz")

else() # MacOSX ?

endif(WIN32)

# now download the file
if (NOT EXISTS "${PROJECT_BINARY_DIR}/${ONNX_TARGET}" )
    message("Shall download ${ONNX_URL} to ${ONNX_TARGET}")
    file(DOWNLOAD ${ONNX_URL} "${PROJECT_BINARY_DIR}/${ONNX_TARGET}" )
endif()

file(ARCHIVE_EXTRACT INPUT "${PROJECT_BINARY_DIR}/${ONNX_TARGET}" )

include_directories("${PROJECT_BINARY_DIR}/${ONNX_ROOTDIR}/include" "${PROJECT_BINARY_DIR}/${ONNX_ROOTDIR}/include/onnxruntime/core/session")
link_directories("${PROJECT_BINARY_DIR}/${ONNX_ROOTDIR}/lib")

message("Setting ONNX include & Lib to : ${PROJECT_BINARY_DIR}/${ONNX_ROOTDIR}")


#option(PLUGIN_NWD_ONLY "Build only WD plugins" OFF)

set(PLUGIN_NWD_OLY OFF)


set(PLUGIN_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../CheckoutPlugins")

include_directories(${ARROW_INCLUDE_DIR})

message(STATUS "Found Arrow: ${ARROW_INCLUDE_DIR} ")

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Update to CMake
if (WIN32)
        find_package(Qt5 5.15 COMPONENTS  Widgets  Xml XmlPatterns
            OpenGL Concurrent WinExtras  WebView WebEngineWidgets REQUIRED)
ELSE()
    find_package(Qt5 5.15 COMPONENTS
        Widgets Xml XmlPatterns
        OpenGL Concurrent WebView WebEngineWidgets
        REQUIRED)
ENDIF(WIN32)

find_package(bsoncxx   REQUIRED)
find_package(mongocxx   REQUIRED)
find_package(Boost 1.56.0 REQUIRED)

find_package(OpenCV REQUIRED )
message(STATUS "Found OpenCV: ${OpenCV_INCLUDE_DIRS} ${OpenCV_LIBS} ")

include_directories( ${OpenCV_INCLUDE_DIRS})

find_package(Boost 1.56.0 REQUIRED)
find_package(Eigen3 CONFIG REQUIRED )
find_package(GSL REQUIRED )
find_package(FFTW3 CONFIG REQUIRED)


# CLOUD Supports

# AWS
#find_package(AWSSDK CONFIG COMPONENTS core dynamodb kinesis s3 REQUIRED)
find_package(AWSSDK CONFIG COMPONENTS core s3 REQUIRED)
#target_include_directories(main PRIVATE ${AWSSDK_INCLUDE_DIRS})
#target_link_libraries(main PRIVATE ${AWSSDK_LIBRARIES})

# Google Cloud (Google Cloud Storage)
# find_package(google_cloud_cpp_storage CONFIG REQUIRED)
# target_link_libraries(main PRIVATE CURL::libcurl google-cloud-cpp::storage google-cloud-cpp::experimental-storage-grpc)

# find_package(google_cloud_cpp_storagetransfer CONFIG REQUIRED)
# target_link_libraries(main PRIVATE google-cloud-cpp::storagetransfer google-cloud-cpp::storagetransfer_protos google-cloud-cpp::experimental-storagetransfer)




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
add_subdirectory(qhttp)
#add_subdirectory(Tags)
add_subdirectory(Gui)

if(BUILD_PYTHON_SCRIPTING)
    add_subdirectory(Python)
#else(BUILD_PYTHON_SCRIPTING)
endif(BUILD_PYTHON_SCRIPTING)

#add_subdirectory(Server)
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
