



add_definitions(-DExportPythonCoreSymbols)

add_library(CheckoutPythonCore SHARED
                    checkoutcorepythoninterface.cpp  checkoutcorepythoninterface.h
                    checkoutpythonpluginshandler.cpp checkoutpythonpluginshandler.h
             )


target_include_directories(CheckoutPythonCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

message(STATUS "Linking with python " ${PYTHON_LIBRARIES})
target_link_libraries(CheckoutPythonCore   Qt5::Core Qt5::Sql Qt5::Xml Qt5::XmlPatterns  Qt5::Concurrent ${OpenCV_LIBS} ${CheckoutPythonLibs} CheckoutCore)


install(TARGETS CheckoutPythonCore COMPONENT applications DESTINATION bin)
