include(FetchContent)
include(GoogleTest)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
# Do not install gtest alongside this project
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Use PRE_TEST discovery so plugin .so files are not required at build time
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
    set(CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST)
endif()

add_subdirectory(unit)
add_subdirectory(integration)
