IF(WIN32)
SET(GUI_TYPE WIN32)
ENDIF(WIN32)


add_definitions(-DCheckoutPluginInCore)

set(CheckoutRawProcessor_SRC
        main.cpp
        rawprocmainwin.h rawprocmainwin.cpp rawprocmainwin.ui
    )


add_executable(CheckoutRawProcessor ${GUI_TYPE} ${CheckoutRawProcessor_SRC}  )

QT5_WRAP_UI(rawprocmainwin.ui)

target_include_directories(CheckoutRawProcessor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(CheckoutRawProcessor CheckoutGui CheckoutCore  Qt5::Widgets  Qt5::Concurrent ${OpenCV_LIBS} )
#install_qt5_executable(CheckoutRawProcessor)
install(TARGETS CheckoutRawProcessor COMPONENT applications DESTINATION bin)
