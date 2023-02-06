
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

IF(WIN32)

    SET(GUI_TYPE WIN32)
    SET(AppRCFILE "../Gui/AppIcon.rc")
    SET(SrvRCFILE "../Gui/SrvIcon.rc" "../Gui/rcs.qrc")


    add_executable(Checkout ${GUI_TYPE} main.cpp ${AppRCFILE} )
    if (MSVC)
        target_compile_options(Checkout PRIVATE /WX)
    endif()

    target_include_directories(Checkout PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    target_link_libraries(Checkout CheckoutGui CheckoutCore QHttp Qt::Widgets Qt::Core Qt::Network  Qt::Concurrent Qt::WinExtras Qt::WebView ${OpenCV_LIBS} )
    #install_qt5_executable(Checkout)
    install(TARGETS Checkout COMPONENT applications DESTINATION bin)
ELSE()

    add_executable(Checkout ${GUI_TYPE} main.cpp )

    target_include_directories(Checkout PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    target_link_libraries(Checkout CheckoutGui CheckoutCore QHttp Qt::Widgets Qt::Core Qt::Network  Qt::Concurrent Qt::WebView  Qt::WebEngineWidgets ${OpenCV_LIBS} "$<IF:$<BOOL:${ARROW_BUILD_STATIC}>,Arrow::arrow_static,Arrow::arrow_shared>" mongo::mongocxx_static mongo::bsoncxx_static  Boost::boost )
ENDIF()

add_executable(ckcli  mainCli.cpp mainCli.h ${AppRCFILE} )

target_include_directories(ckcli PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


if (WIN32)
    target_link_libraries(ckcli ${CheckoutLibs} QHttp Qt::Widgets Qt::Core Qt::Network  Qt::Concurrent  ${OpenCV_LIBS} )
else()
    target_link_libraries(ckcli ${CheckoutLibs} QHttp Qt::Widgets Qt::Core Qt::Network  Qt::Concurrent "$<IF:$<BOOL:${ARROW_BUILD_STATIC}>,Arrow::arrow_static,Arrow::arrow_shared>" ${OpenCV_LIBS} )
endif(WIN32)

install(TARGETS ckcli COMPONENT applications DESTINATION bin)


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
    target_link_libraries(CpTranscode Qt::Widgets Qt::Core Qt::Network  Qt::Concurrent  ${OpenCV_LIBS}   arrow_shared ${ARROW_LIBRARIES}  ${AWSSDK_LIBRARIES} CURL::libcurl google-cloud-cpp::storage google-cloud-cpp::experimental-storage-grpc )
else()
    target_link_libraries(CpTranscode Qt::Widgets Qt::Core Qt::Network  Qt::Concurrent  ${OpenCV_LIBS}  "$<IF:$<BOOL:${ARROW_BUILD_STATIC}>,Arrow::arrow_static,Arrow::arrow_shared>" ${AWSSDK_LIBRARIES} CURL::libcurl google-cloud-cpp::storage google-cloud-cpp::experimental-storage-grpc )
endif(WIN32)

install(TARGETS CpTranscode COMPONENT applications DESTINATION bin)

#add_custom_command(TARGET Checkout  PRE_BUILD
#    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/../kill_server.bat"
#   )

add_definitions(-DQHTTP_MEMORY_LOG=0)
add_definitions(-DQHTTP_HAS_CLIENT)


add_executable(CheckoutHttpServer mainHttpServer.cpp checkouthttpserver.h checkout_python.h ${SrvRCFILE} )
target_include_directories(CheckoutHttpServer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (WIN32)
    target_link_libraries( CheckoutHttpServer ${CheckoutLibs} QHttp Qt::Core Qt::Network Qt::Widgets Qt::WinExtras ${OpenCV_LIBS})
else()
        target_link_libraries( CheckoutHttpServer ${CheckoutLibs} QHttp Qt::Core Qt::Network Qt::Widgets Qt::Concurrent "$<IF:$<BOOL:${ARROW_BUILD_STATIC}>,Arrow::arrow_static,Arrow::arrow_shared>" ${OpenCV_LIBS}  )
endif(WIN32)

install(TARGETS CheckoutHttpServer COMPONENT applications DESTINATION bin)

add_executable(CheckoutQueueServer mainQueueServer.cpp checkouqueuserver.h checkout_arrow.h checkout_python.h ${SrvRCFILE} )
target_include_directories(CheckoutQueueServer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (WIN32)
    target_link_libraries( CheckoutQueueServer ${CheckoutLibs} QHttp Qt::Core Qt::Network Qt::Widgets Qt::WinExtras ${ARROW_STATIC_LIB} ${ARROW_LIBRARIES} ${OpenCV_LIBS})
else()
        target_link_libraries( CheckoutQueueServer PRIVATE ${CheckoutLibs} QHttp Qt::Concurrent Qt::Core Qt::Network Qt::Widgets "$<IF:$<BOOL:${ARROW_BUILD_STATIC}>,Arrow::arrow_static,Arrow::arrow_shared>"  ${OpenCV_LIBS})
endif(WIN32)

install(TARGETS CheckoutQueueServer COMPONENT applications DESTINATION bin)



add_custom_target(Deploy
     #"C:/Program Files (x86)/Windows Kits/10/bin/10.0.19041.0/x64/signtool.exe"  sign /a /tr http://timestamp.globalsign.com/tsa/r6advanced1 /td SHA256  "${PROJECT_BINARY_DIR}/bin/Release/Checkout.exe"
     #COMMAND "C:/Program Files (x86)/Windows Kits/10/bin/10.0.19041.0/x64/signtool.exe"  sign /a /tr http://timestamp.globalsign.com/tsa/r6advanced1 /td SHA256  "${PROJECT_BINARY_DIR}/bin/Release/CheckoutHttpServer.exe"
     #COMMAND
     "C:/Program Files (x86)/NSIS/makensis.exe"  "${PROJECT_BINARY_DIR}/Installer_vs2015.nsi"
     #COMMAND "C:/Program Files (x86)/Windows Kits/10/bin/10.0.19041.0/x64/signtool.exe"  sign /a /tr http://timestamp.globalsign.com/tsa/r6advanced1 /td SHA256  "${PROJECT_BINARY_DIR}/Checkout_InstallerV${Checkout_VERSION_MAJOR}.${Checkout_VERSION_MINOR}.${Checkout_VERSION_COMPLEMENT}${Checkout_VERSION_RELEASE}.exe"

     WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
     DEPENDS CheckoutHttpServer Checkout
     )

