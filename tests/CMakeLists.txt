include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
    GIT_SHALLOW    TRUE
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
    set(CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST)
endif()

set(PLUGIN_BUILD_DIR "${CMAKE_BINARY_DIR}/plugins")

add_subdirectory(unit)
add_subdirectory(integration)
