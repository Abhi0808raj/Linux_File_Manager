cmake_minimum_required(VERSION 3.16)
project(file_manager VERSION 1.0 LANGUAGES CXX)

# Enable modern CMake features
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # For IDE support
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6 configuration
find_package(Qt6 REQUIRED COMPONENTS Widgets)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Project structure configuration
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

# Core sources
file(GLOB_RECURSE CORE_SOURCES
        "file_manager/core/*.cpp"
)

# GUI sources
file(GLOB_RECURSE GUI_SOURCES
        "file_manager/gui/*.cpp"
)

# Utility sources
file(GLOB_RECURSE UTILITY_SOURCES
        "file_manager/utilities/*.cpp"
)

# Application entry
set(APP_SOURCES
        app/main.cpp
)

# Combine all sources
set(ALL_SOURCES
        ${CORE_SOURCES}
        ${GUI_SOURCES}
        ${UTILITY_SOURCES}
        ${APP_SOURCES}
        app/all_header_test.cpp
        app/test.cpp
)

# Main executable
add_executable(${MAIN_EXECUTABLE_NAME}
        ${ALL_SOURCES}
)

# Link Qt libraries
target_link_libraries(${MAIN_EXECUTABLE_NAME}
        PRIVATE Qt6::Widgets
)

# Plugin configuration
include(cmake/PluginConfig.cmake)

# Build plugins
add_subdirectory(plugins/basic_operations)
add_subdirectory(plugins/example_plugin)

# Set runtime output directory for plugins
set_target_properties(${MAIN_EXECUTABLE_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Resources handling
if(EXISTS ${RESOURCES_DIR})
    file(GLOB_RECURSE RESOURCE_FILES
            "${RESOURCES_DIR}/*"
    )
    qt_add_resources(RES_SOURCES ${RESOURCE_FILES})
    target_sources(${MAIN_EXECUTABLE_NAME} PRIVATE ${RES_SOURCES})
endif()

# Testing configuration
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    enable_testing()
    file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")

    add_executable(tests ${TEST_SOURCES})
    target_link_libraries(tests
            PRIVATE
            Qt6::Widgets
            ${MAIN_EXECUTABLE_NAME}  # Link against main executable
    )
    add_test(NAME core_tests COMMAND tests)
endif()

# Installation rules
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

# Summary
message(STATUS "Project configured successfully")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Qt version: ${Qt6_VERSION}")
message(STATUS "Source directory: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Binary directory: ${CMAKE_BINARY_DIR}")
