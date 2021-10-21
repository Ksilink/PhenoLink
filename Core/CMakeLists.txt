

add_definitions(-DExportCoreSymbols)
add_definitions(-DExportPluginManagerSymbols)
add_definitions(-DQHTTP_MEMORY_LOG=0)
add_definitions(-DQHTTP_HAS_CLIENT)
add_library(CheckoutCore SHARED
            sequenceinteractor.cpp sequenceinteractor.h
            imageinfos.cpp imageinfos.h
            wellplatemodel.cpp wellplatemodel.h
            pluginmanager.cpp pluginmanager.h
            checkoutdataloaderplugininterface.cpp checkoutdataloaderplugininterface.h
            checkoutprocessplugininterface.cpp checkoutprocessplugininterface.h
            checkoutprocess.cpp checkoutprocess.h
            networkmessages.h networkmessages.cpp
            networkprocesshandler.cpp networkprocesshandler.h
            Dll.h
            RegistrableTypes.cpp RegistrableTypes.h
            RegistrableImageType.cpp  RegistrableImageType.h
            RegistrableIntegralType.h RegistrableStdContainerType.h
            RegistrableSecondOrderType.h
            ImageContainers.cpp ImageContainers.h
            matrixdatamodel.cpp matrixdatamodel.h
            checkouterrorhandler.cpp checkouterrorhandler.h

            )

include_directories(${ARROW_INCLUDE_DIR})
if (MSVC)
    target_compile_options(CheckoutCore PRIVATE /WX)
endif()

target_include_directories(CheckoutCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(CheckoutCore  QHttp Qt::Widgets Qt::Core Qt::Network Qt::Sql Qt::Xml Qt::XmlPatterns  Qt::Concurrent ${OpenCV_LIBS} arrow_shared )

#add_custom_command(TARGET CheckoutCore   PRE_BUILD
#    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/../kill_server.bat"
#    )

install(TARGETS CheckoutCore COMPONENT applications DESTINATION bin)
