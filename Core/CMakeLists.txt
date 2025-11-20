set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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



add_library(PhenoLinkCore SHARED
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
            phenolinkimage.h phenolinkimage.cpp
            Cloud/PhenoLinkDir.h Cloud/PhenoLinkDir.cpp
            zmq/mdcliapi.hpp zmq/mdp.hpp
            zmq/mdwrkapi.cpp zmq/mdwrkapi.hpp
            zmq/zhelpers.hpp
            zmq/zmsg.hpp zmq/zmsg.cpp
            zmq/mdbroker.hpp zmq/mdbroker.cpp
            zmq/performances.hpp
            )

include_directories(${ARROW_INCLUDE_DIR})
if (MSVC)
    target_compile_options(PhenoLinkCore PRIVATE /WX)
endif()

find_package(bsoncxx   REQUIRED)
find_package(mongocxx   REQUIRED)
find_package(Boost 1.56.0 REQUIRED)
find_package(cppzmq CONFIG REQUIRED)


#find_package(Qt6 REQUIRED COMPONENTS Core5Compat)
#find_package(Qt6 REQUIRED COMPONENTS Xml)
#find_package(Qt6 REQUIRED COMPONENTS WinExtras)

find_package(LibArchive REQUIRED)

find_package(PkgConfig)
pkg_check_modules(Jxl REQUIRED IMPORTED_TARGET libjxl)
pkg_check_modules(JxlThreads REQUIRED IMPORTED_TARGET libjxl_threads)

#find_package(Jxl REQUIRED)
#find_package(JxlThreads REQUIRED)


if (BUILD_PLUGINS)
    target_compile_definitions(PhenoLinkCore PUBLIC  -DGitPluginDate=${plugin_date})
else()
    set(plugin_date "BuildWithoutPlugins")
target_compile_definitions(PhenoLinkCore PUBLIC  -DGitPluginDate="${plugin_date}")
endif(BUILD_PLUGINS)

target_include_directories(PhenoLinkCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
    target_link_libraries(PhenoLinkCore Pdh QHttp Qt::Widgets Qt::Core Qt::Network Qt::Xml Qt::Concurrent "$<IF:$<BOOL:${ARROW_BUILD_STATIC}>,Arrow::arrow_static,Arrow::arrow_shared>"  ${OpenCV_LIBS} arrow_shared mongo::mongocxx_shared mongo::bsoncxx_shared  Boost::boost  ${AWSSDK_LIBRARIES} CURL::libcurl google-cloud-cpp::storage  Azure::azure-storage-blobs PkgConfig::Jxl PkgConfig::JxlThreads LibArchive::LibArchive ${cppzmq_LIBRARY} libzmq libzmq-static ${CZI_LIB} )
else()
    target_link_libraries(PhenoLinkCore PRIVATE QHttp Qt::Widgets Qt::Core Qt::Network Qt::Xml Qt::Concurrent "$<IF:$<BOOL:${ARROW_BUILD_STATIC}>,Arrow::arrow_static,Arrow::arrow_shared>"  ${OpenCV_LIBS}  mongo::mongocxx_static mongo::bsoncxx_static Boost::boost  ${AWSSDK_LIBRARIES} CURL::libcurl google-cloud-cpp::storage Azure::azure-storage-blobs PkgConfig::Jxl PkgConfig::JxlThreads  LibArchive::LibArchive ${cppzmq_LIBRARY} libzmq libzmq-static ${CZI_LIB})
endif(WIN32)


install(TARGETS PhenoLinkCore COMPONENT applications DESTINATION bin)
