cmake_minimum_required(VERSION 3.16)
project(file_manager VERSION 1.0 LANGUAGES CXX)

# Modern CMake features
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6 setup
find_package(Qt6 REQUIRED COMPONENTS Widgets)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Project structure
set(MAIN_EXECUTABLE_NAME file_manager)
set(PLUGINS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/plugins)
set(RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources)

# Include directories
include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/core
        ${CMAKE_CURRENT_SOURCE_DIR}/include/gui
        ${CMAKE_CURRENT_SOURCE_DIR}/include/utilities
)

# Source files
file(GLOB_RECURSE GUI_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/gui/*.hpp"
)

file(GLOB_RECURSE CORE_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/file_manager/core/*.cpp"
)
file(GLOB_RECURSE GUI_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/file_manager/gui/*.cpp"
)
file(GLOB_RECURSE UTILITY_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/file_manager/utilities/*.cpp"
)
set(APP_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/app/main.cpp
        file_manager/utilities/error_handler.cpp
        file_manager/utilities/logger.cpp
        plugins/basic_operations/include/copy_plugin.hpp
        plugins/basic_operations/include/delete_plugin.hpp
        plugins/basic_operations/include/move_plugin.hpp
        plugins/example_plugin/include/example_plugin.hpp
#        tests/Logger_Test/test_logger.cpp
#        tests/Error_Handler_Test/error_handler_test.cpp

)

# Shared library for core logic
add_library(file_manager_core STATIC)

target_sources(file_manager_core
        PRIVATE
        ${GUI_HEADERS}
        ${CORE_SOURCES}
        ${GUI_SOURCES}
        ${UTILITY_SOURCES}
)


target_include_directories(file_manager_core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(file_manager_core
        PUBLIC Qt6::Widgets
)

# Main application executable
add_executable(${MAIN_EXECUTABLE_NAME}
        ${APP_SOURCES}
)
target_include_directories(${MAIN_EXECUTABLE_NAME}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
)


target_link_libraries(${MAIN_EXECUTABLE_NAME}
        PRIVATE file_manager_core
)

# Set output directory
set_target_properties(${MAIN_EXECUTABLE_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Plugin setup
include(cmake/PluginConfig.cmake)

add_subdirectory(plugins/basic_operations)
add_subdirectory(plugins/example_plugin)

# Resource handling
if(EXISTS ${RESOURCES_DIR})
    file(GLOB_RECURSE RESOURCE_FILES "${RESOURCES_DIR}/*")
    qt_add_resources(RES_SOURCES ${RESOURCE_FILES})
    target_sources(${MAIN_EXECUTABLE_NAME} PRIVATE ${RES_SOURCES})
endif()

# Installation
install(TARGETS ${MAIN_EXECUTABLE_NAME}
        DESTINATION bin
)

install(DIRECTORY ${PLUGINS_DIR}/
        DESTINATION plugins
        PATTERN "CMakeLists.txt" EXCLUDE
)

install(DIRECTORY ${RESOURCES_DIR}/
        DESTINATION resources
)

install(DIRECTORY include/
        DESTINATION include
)

# Summary messages
message(STATUS "Project configured successfully")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Qt version: ${Qt6_VERSION}")
message(STATUS "Source directory: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Binary directory: ${CMAKE_BINARY_DIR}")


#Only for testing
# --- Modular Testing ---
option(TEST_PLUGIN_MANAGER_ONLY "Plugin Manager test only" OFF)
option(TEST_FILE_SYSTEM_ONLY "Build file system test only" OFF)
option(TEST_LOGGER_ONLY "Build logger test only" OFF)
option(TEST_ERROR_HANDLER_ONLY "Build error handler test only" OFF)


if(TEST_FILE_SYSTEM_ONLY )
    add_subdirectory(tests/File_system_Test)
endif()

if(TEST_LOGGER_ONLY)
    add_subdirectory(tests/Logger_Test)
endif()

if(TEST_ERROR_HANDLER_ONLY)
    add_subdirectory(tests/Error_Handler_Test)
endif()

if(TEST_PLUGIN_MANAGER_ONLY)
    add_subdirectory(tests/Plugin_Manager_Test)
endif()
