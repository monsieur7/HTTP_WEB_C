cmake_minimum_required(VERSION 3.0)

project(HTTP_WEB_C)
set(SOURCES main.cpp socket.cpp socket.hpp parseString.cpp parseString.hpp)

# set MIN C++ version
set(CMAKE_CXX_STANDARD 20)
add_executable(${PROJECT_NAME} ${SOURCES})

# link against boost :
find_package(Boost 1.58.0 COMPONENTS system REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})

# CMAKE google TEST :
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# enable tests :
enable_testing()
add_executable(
    unit_tests
    tests/parseUnitTest.cc
    socket.cpp
    parseString.cpp
)
target_link_libraries(
    unit_tests
    gtest_main
)
include(GoogleTest)
gtest_discover_tests(unit_tests)