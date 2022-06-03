


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

    target_link_libraries(Checkout CheckoutGui CheckoutCore QHttp Qt::Widgets Qt::Core Qt::Network  Qt::Concurrent Qt::WebView  Qt::WebEngineWidgets ${OpenCV_LIBS} ${ARROW_LIBS}  arrow mongo::mongocxx_static mongo::bsoncxx_static  Boost::boost )
ENDIF()

add_executable(ckcli  mainCli.cpp mainCli.h ${AppRCFILE} )

target_include_directories(ckcli PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


if (WIN32)
    target_link_libraries(ckcli ${CheckoutLibs} QHttp Qt::Widgets Qt::Core Qt::Network  Qt::Concurrent  ${OpenCV_LIBS} )
else()
    target_link_libraries(ckcli ${CheckoutLibs} QHttp Qt::Widgets Qt::Core Qt::Network  Qt::Concurrent  arrow ${ARROW_LIBRARIES} ${OpenCV_LIBS} )
endif(WIN32)

install(TARGETS ckcli COMPONENT applications DESTINATION bin)



#add_custom_command(TARGET Checkout  PRE_BUILD
#    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/../kill_server.bat"
#   )

add_definitions(-DQHTTP_MEMORY_LOG=0)
add_definitions(-DQHTTP_HAS_CLIENT)


add_executable(CheckoutHttpServer mainHttpServer.cpp checkouthttpserver.h ${SrvRCFILE} )
target_include_directories(CheckoutHttpServer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (WIN32)
    target_link_libraries( CheckoutHttpServer ${CheckoutLibs} QHttp Qt::Core Qt::Network Qt::Widgets Qt::WinExtras ${OpenCV_LIBS})
else()
        target_link_libraries( CheckoutHttpServer ${CheckoutLibs} QHttp Qt::Core Qt::Network Qt::Widgets Qt::Concurrent arrow ${ARROW_LIBRARIES} ${OpenCV_LIBS}  )
endif(WIN32)

install(TARGETS CheckoutHttpServer COMPONENT applications DESTINATION bin)

add_executable(CheckoutQueueServer mainQueueServer.cpp checkouqueuserver.h ${SrvRCFILE} )
target_include_directories(CheckoutQueueServer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (WIN32)
    target_link_libraries( CheckoutQueueServer ${CheckoutLibs} QHttp Qt::Core Qt::Network Qt::Widgets Qt::WinExtras ${ARROW_STATIC_LIB} ${ARROW_LIBRARIES} ${OpenCV_LIBS})
else()
        target_link_libraries( CheckoutQueueServer PRIVATE ${CheckoutLibs} QHttp Qt::Concurrent Qt::Core Qt::Network Qt::Widgets arrow ${ARROW_STATIC_LIB} ${ARROW_LIBRARIES} ${OpenCV_LIBS})
endif(WIN32)

install(TARGETS CheckoutQueueServer COMPONENT applications DESTINATION bin)



