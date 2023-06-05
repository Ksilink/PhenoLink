set(CMAKE_CXX_STANDARD 17)


if (WIN32)
    add_definitions(-DQHTTP_EXPORT)

endif(WIN32)

add_definitions(-DQHTTP_MEMORY_LOG=0)
add_definitions(-DQHTTP_HAS_CLIENT)

add_library(QHttp SHARED
    qhttpabstracts.cpp
    qhttpserverconnection.cpp
    qhttpserverrequest.cpp
    qhttpserverresponse.cpp
    qhttpserver.cpp
    qhttpfwd.hpp
    qhttpabstracts.hpp
    qhttpserverconnection.hpp
    qhttpserverrequest.hpp
    qhttpserverresponse.hpp
    qhttpserver.hpp
    qhttpclientrequest.cpp
    qhttpclientresponse.cpp
    qhttpclient.cpp
    qhttpclient.hpp
    qhttpclientresponse.hpp
    qhttpclientrequest.hpp
    http_parser.c http_parser.h
    )


target_include_directories(QHttp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(QHttp Qt::Core Qt::Network )

#add_custom_command(TARGET CheckoutCore   PRE_BUILD
#    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/../kill_server.bat"
#    )

install(TARGETS QHttp COMPONENT applications DESTINATION bin)
