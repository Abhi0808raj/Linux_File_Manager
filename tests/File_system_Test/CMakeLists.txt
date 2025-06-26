add_executable(test_file_system_only
        test_file_system_only.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../file_manager/core/file_system.cpp
)

target_include_directories(test_file_system_only PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include/core
)

target_link_libraries(test_file_system_only PRIVATE Qt6::Widgets)
