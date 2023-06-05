
add_library(CheckoutYokogawaLoader SHARED
                                yokogawaloader.cpp yokogawaloader.h)

set_target_properties(CheckoutYokogawaLoader
                PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins)

target_link_libraries(CheckoutYokogawaLoader  CheckoutCore Qt5::Core Qt5::Xml )

install(TARGETS CheckoutYokogawaLoader COMPONENT PluginLoader DESTINATION bin/plugins)

add_library(CheckoutInCellLoader SHARED
                                incellloader.cpp incellloader.h)

set_target_properties(CheckoutInCellLoader
                PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins)

target_link_libraries(CheckoutInCellLoader  CheckoutCore Qt5::Core Qt5::Xml )

install(TARGETS CheckoutInCellLoader COMPONENT PluginLoader DESTINATION bin/plugins)
