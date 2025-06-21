# Plugin configuration setup
set(PLUGIN_INSTALL_DIR ${CMAKE_BINARY_DIR}/plugins)
file(MAKE_DIRECTORY ${PLUGIN_INSTALL_DIR})

# Macro for building plugins
macro(add_plugin PLUGIN_NAME)
    add_library(${PLUGIN_NAME} SHARED ${ARGN})
    set_target_properties(${PLUGIN_NAME} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_INSTALL_DIR}
            POSITION_INDEPENDENT_CODE ON
    )
    target_include_directories(${PLUGIN_NAME}
            PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include
    )
endmacro()
