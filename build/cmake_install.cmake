# Install script for directory: D:/Checkout_2020/CheckoutApp

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files/Checkout")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE PROGRAM FILES
    "C:/Program Files/Microsoft Visual Studio/2022/Preview/VC/Redist/MSVC/14.30.30528/x64/Microsoft.VC142.CRT/msvcp140.dll"
    "C:/Program Files/Microsoft Visual Studio/2022/Preview/VC/Redist/MSVC/14.30.30528/x64/Microsoft.VC142.CRT/msvcp140_1.dll"
    "C:/Program Files/Microsoft Visual Studio/2022/Preview/VC/Redist/MSVC/14.30.30528/x64/Microsoft.VC142.CRT/msvcp140_2.dll"
    "C:/Program Files/Microsoft Visual Studio/2022/Preview/VC/Redist/MSVC/14.30.30528/x64/Microsoft.VC142.CRT/msvcp140_atomic_wait.dll"
    "C:/Program Files/Microsoft Visual Studio/2022/Preview/VC/Redist/MSVC/14.30.30528/x64/Microsoft.VC142.CRT/msvcp140_codecvt_ids.dll"
    "C:/Program Files/Microsoft Visual Studio/2022/Preview/VC/Redist/MSVC/14.30.30528/x64/Microsoft.VC142.CRT/vcruntime140_1.dll"
    "C:/Program Files/Microsoft Visual Studio/2022/Preview/VC/Redist/MSVC/14.30.30528/x64/Microsoft.VC142.CRT/vcruntime140.dll"
    "C:/Program Files/Microsoft Visual Studio/2022/Preview/VC/Redist/MSVC/14.30.30528/x64/Microsoft.VC142.CRT/concrt140.dll"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE DIRECTORY FILES "")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("D:/Checkout_2020/CheckoutApp/build/Core/cmake_install.cmake")
  include("D:/Checkout_2020/CheckoutApp/build/qhttp/cmake_install.cmake")
  include("D:/Checkout_2020/CheckoutApp/build/Gui/cmake_install.cmake")
  include("D:/Checkout_2020/CheckoutApp/build/Main/cmake_install.cmake")
  include("D:/Checkout_2020/CheckoutApp/build/Plugins/cmake_install.cmake")
  include("D:/Checkout_2020/CheckoutApp/build/Testing/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "D:/Checkout_2020/CheckoutApp/build/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
