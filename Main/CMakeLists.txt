IF(WIN32)

SET(GUI_TYPE WIN32)
SET(AppRCFILE "../Gui/AppIcon.rc")
SET(SrvRCFILE "../Gui/SrvIcon.rc" "../Gui/rcs.qrc")

ENDIF(WIN32)

add_executable(Checkout ${GUI_TYPE} main.cpp ${AppRCFILE} )

target_include_directories(Checkout PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(Checkout CheckoutGui  ${CheckoutLibs} Qt5::Widgets Qt5::Sql Qt5::Concurrent Qt5::WinExtras ${OpenCV_LIBS} )
#install_qt5_executable(Checkout)
install(TARGETS Checkout COMPONENT applications DESTINATION bin)


#add_custom_command(TARGET Checkout  PRE_BUILD
#    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/../kill_server.bat"
 #   )

add_definitions(-DCheckoutServer)
add_executable(CheckoutProcessServer ${GUI_TYPE} mainServer.cpp   ${SrvRCFILE} )

target_link_libraries(CheckoutProcessServer  CheckoutServer ${CheckoutLibs} Qt5::Core Qt5::Network Qt5::Widgets Qt5::Sql Qt5::WinExtras ${OpenCV_LIBS})

#install_qt5_executable(CheckoutProcessServer)
install(TARGETS CheckoutProcessServer COMPONENT applications DESTINATION bin)
