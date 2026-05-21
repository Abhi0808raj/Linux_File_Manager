add_executable(test_file_system_only
        test_file_system_only.cpp
        ${CMAKE_SOURCE_DIR}/file_manager/core/file_system.cpp
        ${CMAKE_SOURCE_DIR}/file_manager/utilities/error_handler.cpp
        ${CMAKE_SOURCE_DIR}/file_manager/utilities/logger.cpp
)

target_include_directories(test_file_system_only PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include/core
        ${CMAKE_SOURCE_DIR}/include/utilities
)

target_link_libraries(test_file_system_only PRIVATE Qt6::Widgets)
