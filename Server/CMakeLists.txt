

add_definitions(-DExportServerSymbols)

add_library(CheckoutServer SHARED
                processserver.cpp processserver.h
#                processthread.cpp processthread.h
  #              networkmessages.cpp networkmessages.h
            )


target_include_directories(CheckoutServer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


if(BUILD_PYTHON_SCRIPTING)
    target_link_libraries(CheckoutServer ${CheckoutPythonLib} CheckoutPythonCore CheckoutCore Qt::Widgets  Qt::Network Qt::Core  ${OpenCV_LIBS} )
else(BUILD_PYTHON_SCRIPTING)
    target_link_libraries(CheckoutServer ${CheckoutPythonLib} CheckoutCore Qt::Widgets  Qt::Network Qt::Core  ${OpenCV_LIBS} )
endif(BUILD_PYTHON_SCRIPTING)


install(TARGETS CheckoutServer COMPONENT applications DESTINATION bin)
