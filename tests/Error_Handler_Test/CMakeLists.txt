add_executable(error_handler_test
        error_handler_test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../file_manager/utilities/error_handler.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../file_manager/utilities/logger.cpp
)

target_include_directories(error_handler_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include/utilities
)

target_link_libraries(error_handler_test PRIVATE Qt6::Widgets)
