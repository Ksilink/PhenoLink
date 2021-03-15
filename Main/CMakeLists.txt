IF(WIN32)

SET(GUI_TYPE WIN32)
SET(AppRCFILE "../Gui/AppIcon.rc")
SET(SrvRCFILE "../Gui/SrvIcon.rc" "../Gui/rcs.qrc")



add_executable(Checkout ${GUI_TYPE} main.cpp ${AppRCFILE} )

target_include_directories(Checkout PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(Checkout CheckoutGui  ${CheckoutLibs} Qt::Widgets Qt::Sql Qt::Concurrent Qt::WinExtras Qt::WebView ${OpenCV_LIBS} )
#install_qt5_executable(Checkout)
install(TARGETS Checkout COMPONENT applications DESTINATION bin)

ENDIF(WIN32)


#add_custom_command(TARGET Checkout  PRE_BUILD
#    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/../kill_server.bat"
 #   )

add_definitions(-DCheckoutServer)
add_executable(CheckoutProcessServer ${GUI_TYPE} mainServer.cpp   ${SrvRCFILE} )

if (WIN32)
    target_link_libraries(CheckoutProcessServer  CheckoutServer ${CheckoutLibs} Qt::Core Qt::Network Qt::Widgets Qt::Sql Qt::WinExtras ${OpenCV_LIBS})
else()
    target_link_libraries(CheckoutProcessServer  CheckoutServer ${CheckoutLibs} Qt::Core Qt::Network Qt::Widgets Qt::Sql ${OpenCV_LIBS})
endif(WIN32)
#install_qt5_executable(CheckoutProcessServer)
install(TARGETS CheckoutProcessServer COMPONENT applications DESTINATION bin)
