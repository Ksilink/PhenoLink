set(CMAKE_CXX_STANDARD 14)

add_definitions(-DExportCoreSymbols)
add_definitions(-DExportPluginManagerSymbols)
add_definitions(-DQHTTP_MEMORY_LOG=0)
add_definitions(-DQHTTP_HAS_CLIENT)
add_definitions(-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)


execute_process(
    COMMAND           "${GIT_EXECUTABLE}"
    log
    -1
    --date=iso
    --pretty=format:"%ai"
    "."
    WORKING_DIRECTORY "${PLUGIN_PATH}"
    RESULT_VARIABLE   git_return
    OUTPUT_VARIABLE   plugin_date)

message("Computing hash for ${PluginFile}: ${plugin_date}")

string(STRIP "${plugin_date}" plugin_date)

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
            RegistrableIntegralType.h RegistrableStdContainerType.h RegistrableStdContainerTypeSpecial.h
            RegistrableSecondOrderType.h
            ImageContainers.cpp ImageContainers.h
            matrixdatamodel.cpp matrixdatamodel.h
            checkouterrorhandler.cpp checkouterrorhandler.h

            ck_mongo.h ck_mongo.cpp

            )

include_directories(${ARROW_INCLUDE_DIR})
if (MSVC)
    target_compile_options(CheckoutCore PRIVATE /WX)
endif()

find_package(bsoncxx   REQUIRED)
find_package(mongocxx   REQUIRED)
find_package(Boost 1.56.0 REQUIRED)


target_compile_definitions(CheckoutCore PUBLIC  -DGitPluginDate=${plugin_date})


target_include_directories(CheckoutCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
target_link_libraries(CheckoutCore  QHttp Qt::Widgets Qt::Core Qt::Network Qt::Xml Qt::XmlPatterns  Qt::Concurrent ${OpenCV_LIBS} arrow_shared mongo::mongocxx_shared mongo::bsoncxx_shared  Boost::boost)
else()
	target_link_libraries(CheckoutCore PRIVATE QHttp Qt::Widgets Qt::Core Qt::Network Qt::Xml Qt::XmlPatterns  Qt::Concurrent ${OpenCV_LIBS} ${ARROW_LIBRARIES}  mongo::mongocxx_static mongo::bsoncxx_static Boost::boost )
endif(WIN32)
#add_custom_command(TARGET CheckoutCore   PRE_BUILD
#    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/../kill_server.bat"
#    )

install(TARGETS CheckoutCore COMPONENT applications DESTINATION bin)
