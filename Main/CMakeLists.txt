
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig)
pkg_check_modules(Jxl REQUIRED IMPORTED_TARGET libjxl)
pkg_check_modules(JxlThreads REQUIRED IMPORTED_TARGET libjxl_threads)
#find_package(ZeroMQ REQUIRED)
find_package(cppzmq CONFIG REQUIRED)

IF(WIN32)

    SET(GUI_TYPE WIN32)
    SET(AppRCFILE "../Gui/AppIcon.rc")
    SET(SrvRCFILE "../Gui/SrvIcon.rc" "../Gui/rcs.qrc")


    add_executable(PhenoLink ${GUI_TYPE} main.cpp ${AppRCFILE} )
    if (MSVC)
        target_compile_options(PhenoLink PRIVATE /WX)
    endif()

    target_include_directories(PhenoLink PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#    target_compile_definitions(PhenoLink PUBLIC  -DGitPluginDate=${plugin_date} )

    target_link_libraries(PhenoLink PhenoLinkGui PhenoLinkCore QHttp Qt::Widgets Qt::Core Qt::Network  Qt::Concurrent Qt::WebView ${OpenCV_LIBS} )
    #install_qt5_executable(Checkout)
    install(TARGETS PhenoLink COMPONENT applications DESTINATION bin)

#    if ($<CONFIG:Debug>)
#    else()
#    add_custom_command(
#        TARGET PhenoLink
#            POST_BUILD
#            COMMAND "${PROJECT_BINARY_DIR}/vcpkg_installed/x64-windows/tools/Qt6/bin/windeployqt.exe" "$<TARGET_FILE:PhenoLink>"

#        )
#    endif()

ELSE()

    add_executable(PhenoLink ${GUI_TYPE} main.cpp "../Gui/rcs.qrc")
    set_property(TARGET PhenoLink PROPERTY AUTORCC_OPTIONS "--compress-algo;none")
 #   target_compile_definitions(PhenoLink PUBLIC  -DGitPluginDate=${plugin_date})
    target_include_directories(PhenoLink PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    target_link_libraries(PhenoLink PhenoLinkGui PhenoLinkCore QHttp Qt::Widgets Qt::Core Qt::Network  Qt::Concurrent ${OpenCV_LIBS} "$<IF:$<BOOL:${ARROW_BUILD_STATIC}>,Arrow::arrow_static,Arrow::arrow_shared>" mongo::mongocxx_static mongo::bsoncxx_static  Boost::boost )
ENDIF()

add_dependencies (PhenoLink PhenoLinkGui PhenoLinkCore)


add_executable(plcli  mainCli.cpp mainCli.h ${AppRCFILE} )

#target_compile_definitions(plcli PUBLIC  -DGitPluginDate="")
target_include_directories(plcli PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies (plcli PhenoLinkGui PhenoLinkCore)

if (WIN32)
    target_link_libraries(plcli ${CheckoutLibs} QHttp Qt::Widgets Qt::Core Qt::Network  Qt::Concurrent  ${OpenCV_LIBS} )

#    if ($<CONFIG:Debug>)
#    else()
#    add_custom_command(
#        TARGET PhenoLink
#            POST_BUILD
#            COMMAND "${PROJECT_BINARY_DIR}/vcpkg_installed/x64-windows/tools/Qt6/bin/windeployqt.exe" "$<TARGET_FILE:plcli>"

#        )
#    endif()

else()
    target_link_libraries(plcli ${CheckoutLibs} QHttp Qt::Widgets Qt::Core Qt::Network  Qt::Concurrent "$<IF:$<BOOL:${ARROW_BUILD_STATIC}>,Arrow::arrow_static,Arrow::arrow_shared>" ${OpenCV_LIBS} )
endif(WIN32)

install(TARGETS plcli COMPONENT applications DESTINATION bin)


add_executable(PhenoLinkUploader mainUploader.cpp mainUploader.h ${AppRCFile})

target_include_directories(PhenoLinkUploader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if (WIN32)
    target_link_libraries(PhenoLinkUploader Qt::Widgets Qt::Core Qt::Network  Qt::Concurrent  arrow_shared ${ARROW_LIBRARIES}  ${AWSSDK_LIBRARIES} CURL::libcurl google-cloud-cpp::storage google-cloud-cpp::experimental-storage-grpc)
else()
    target_link_libraries(PhenoLinkUploader Qt::Widgets Qt::Core Qt::Network  Qt::Concurrent  "$<IF:$<BOOL:${ARROW_BUILD_STATIC}>,Arrow::arrow_static,Arrow::arrow_shared>" ${AWSSDK_LIBRARIES} CURL::libcurl google-cloud-cpp::storage google-cloud-cpp::experimental-storage-grpc )
endif(WIN32)



install(TARGETS PhenoLinkUploader COMPONENT applications DESTINATION bin)


add_executable(CpTranscode mainCpTranscode.cpp ${AppRCFile} enc_fast_lossless.cc)
target_include_directories(CpTranscode PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if (WIN32)
    target_link_libraries(CpTranscode Qt::Widgets Qt::Core Qt::Network  Qt::Concurrent  ${OpenCV_LIBS}   arrow_shared ${ARROW_LIBRARIES}  ${AWSSDK_LIBRARIES} CURL::libcurl google-cloud-cpp::storage google-cloud-cpp::experimental-storage-grpc PkgConfig::Jxl PkgConfig::JxlThreads)
else()
    target_link_libraries(CpTranscode Qt::Widgets Qt::Core Qt::Network  Qt::Concurrent  ${OpenCV_LIBS}  "$<IF:$<BOOL:${ARROW_BUILD_STATIC}>,Arrow::arrow_static,Arrow::arrow_shared>" ${AWSSDK_LIBRARIES} CURL::libcurl google-cloud-cpp::storage google-cloud-cpp::experimental-storage-grpc PkgConfig::Jxl PkgConfig::JxlThreads)
endif(WIN32)

install(TARGETS CpTranscode COMPONENT applications DESTINATION bin)

#add_custom_command(TARGET Checkout  PRE_BUILD
#    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/../kill_server.bat"
#   )

add_definitions(-DQHTTP_MEMORY_LOG=0)
add_definitions(-DQHTTP_HAS_CLIENT)


add_executable(PhenoLinkHttpServer mainHttpServer.cpp checkouthttpserver.h checkout_python.h ${SrvRCFILE} )

#target_compile_definitions(PhenoLinkHttpServer PUBLIC  -DGitPluginDate="")

target_include_directories(PhenoLinkHttpServer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies (PhenoLinkHttpServer PhenoLinkGui PhenoLinkCore)

if (WIN32)
    target_link_libraries( PhenoLinkHttpServer ${CheckoutLibs} QHttp  Qt::Concurrent Qt::Core Qt::Network Qt::Widgets  ${OpenCV_LIBS})

#    if ($<CONFIG:Debug>)
#    else()
#    add_custom_command(
#        TARGET PhenoLink
#            POST_BUILD
#            COMMAND "${PROJECT_BINARY_DIR}/vcpkg_installed/x64-windows/tools/Qt6/bin/windeployqt.exe" "$<TARGET_FILE:PhenoLinkHttpServer>"

#        )
#    endif()
else()
        target_link_libraries( PhenoLinkHttpServer ${CheckoutLibs} QHttp Qt::Core Qt::Network Qt::Widgets Qt::Concurrent "$<IF:$<BOOL:${ARROW_BUILD_STATIC}>,Arrow::arrow_static,Arrow::arrow_shared>" ${OpenCV_LIBS}  )
endif(WIN32)

install(TARGETS PhenoLinkHttpServer COMPONENT applications DESTINATION bin)

add_executable(PhenoLinkQueueServer mainQueueServer.cpp checkouqueuserver.h checkout_arrow.h checkout_python.h ${SrvRCFILE} )

target_include_directories(PhenoLinkQueueServer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (WIN32)
    target_link_libraries( PhenoLinkQueueServer ${CheckoutLibs} QHttp Qt::Core Qt::Concurrent Qt::Network Qt::Widgets ${ARROW_STATIC_LIB} ${ARROW_LIBRARIES} ${OpenCV_LIBS})

    if ($<CONFIG:Debug>)
    else()
    add_custom_command(
         TARGET PhenoLinkQueueServer
            POST_BUILD
            COMMAND "${PROJECT_BINARY_DIR}/vcpkg_installed/x64-windows/tools/Qt6/bin/windeployqt.exe" "$<TARGET_FILE:PhenoLinkQueueServer>"

        )
    endif()

else()
        target_link_libraries( PhenoLinkQueueServer PRIVATE ${CheckoutLibs} QHttp Qt::Concurrent Qt::Core Qt::Network Qt::Widgets "$<IF:$<BOOL:${ARROW_BUILD_STATIC}>,Arrow::arrow_static,Arrow::arrow_shared>"  ${OpenCV_LIBS})
endif(WIN32)

install(TARGETS PhenoLinkQueueServer COMPONENT applications DESTINATION bin)
add_dependencies (PhenoLinkQueueServer PhenoLinkGui PhenoLinkCore)



add_executable(PhenoLinkZMQServer mainZMQ_QueueServer.cpp checkouqueuserver.h checkout_arrow.h checkout_python.h ${SrvRCFILE} )

target_include_directories(PhenoLinkZMQServer PUBLIC ${ZeroMQ_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
# target_link_libraries(main PRIVATE cppzmq cppzmq-static)
if (WIN32)
    target_link_libraries( PhenoLinkZMQServer ${CheckoutLibs} QHttp Qt::Core Qt::Concurrent Qt::Network Qt::Widgets ${ARROW_STATIC_LIB} ${ARROW_LIBRARIES} ${OpenCV_LIBS} cppzmq cppzmq-static)

#    if ($<CONFIG:Debug>)
#    else()
#    add_custom_command(
#         TARGET PhenoLinkZMQServer
#            POST_BUILD
#            COMMAND "${PROJECT_BINARY_DIR}/vcpkg_installed/x64-windows/tools/Qt6/bin/windeployqt.exe" "$<TARGET_FILE:PhenoLinkQueueServer>"

#        )
#    endif()

else()
        target_link_libraries( PhenoLinkZMQServer PRIVATE ${CheckoutLibs} QHttp Qt::Concurrent Qt::Core Qt::Network Qt::Widgets "$<IF:$<BOOL:${ARROW_BUILD_STATIC}>,Arrow::arrow_static,Arrow::arrow_shared>"  ${OpenCV_LIBS} cppzmq cppzmq-static)
endif(WIN32)

install(TARGETS PhenoLinkZMQServer COMPONENT applications DESTINATION bin)
add_dependencies (PhenoLinkZMQServer PhenoLinkGui PhenoLinkCore)





add_custom_target(Deploy
     #"C:/Program Files (x86)/Windows Kits/10/bin/10.0.19041.0/x64/signtool.exe"  sign /a /tr http://timestamp.globalsign.com/tsa/r6advanced1 /td SHA256  "${PROJECT_BINARY_DIR}/bin/Release/Checkout.exe"
     #COMMAND "C:/Program Files (x86)/Windows Kits/10/bin/10.0.19041.0/x64/signtool.exe"  sign /a /tr http://timestamp.globalsign.com/tsa/r6advanced1 /td SHA256  "${PROJECT_BINARY_DIR}/bin/Release/CheckoutHttpServer.exe"
     #COMMAND
     "C:/Program Files (x86)/NSIS/makensis.exe"  "${PROJECT_BINARY_DIR}/Installer_vs2015.nsi"
     #COMMAND "C:/Program Files (x86)/Windows Kits/10/bin/10.0.19041.0/x64/signtool.exe"  sign /a /tr http://timestamp.globalsign.com/tsa/r6advanced1 /td SHA256  "${PROJECT_BINARY_DIR}/Checkout_InstallerV${Checkout_VERSION_MAJOR}.${Checkout_VERSION_MINOR}.${Checkout_VERSION_COMPLEMENT}${Checkout_VERSION_RELEASE}.exe"

     WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
     DEPENDS CheckoutHttpServer Checkout
     )

