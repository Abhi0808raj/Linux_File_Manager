add_executable(test_logger
        test_logger.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../file_manager/utilities/logger.cpp
)

target_include_directories(test_logger PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include/utilities
)

target_link_libraries(test_logger PRIVATE Qt6::Widgets)
