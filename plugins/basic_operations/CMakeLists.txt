cmake_minimum_required(VERSION 3.16)
project(basic_operations)

# Copy plugin
add_library(copy_plugin SHARED src/copy_plugin.cpp)
target_include_directories(copy_plugin
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(copy_plugin PRIVATE file_manager_core)
set_target_properties(copy_plugin PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
)
install(TARGETS copy_plugin DESTINATION plugins)

# Move plugin
add_library(move_plugin SHARED src/move_plugin.cpp)
target_include_directories(move_plugin
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(move_plugin PRIVATE file_manager_core)
set_target_properties(move_plugin PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
)
install(TARGETS move_plugin DESTINATION plugins)

# Delete plugin
add_library(delete_plugin SHARED src/delete_plugin.cpp)
target_include_directories(delete_plugin
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(delete_plugin PRIVATE file_manager_core)
set_target_properties(delete_plugin PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
)
install(TARGETS delete_plugin DESTINATION plugins)
