
add_definitions(-DExportPluginSymbols)


add_subdirectory(${PLUGIN_PATH} ${CMAKE_CURRENT_BINARY_DIR}/bin/plugins)

if (PLUGIN_PRIVATE_PATH)
    add_subdirectory(${PLUGIN_PRIVATE_PATH} ${CMAKE_CURRENT_BINARY_DIR}/bin/plugins)
endif(PLUGIN_PRIVATE_PATH)

#add_subdirectory(Process)


