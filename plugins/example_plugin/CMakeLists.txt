add_library(example_plugin SHARED
        src/example_plugin.cpp
)

# Include necessary headers
target_include_directories(example_plugin
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(example_plugin
        PRIVATE file_manager_core
)

# Output .so to a known location for plugin tests
set_target_properties(example_plugin PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
)
