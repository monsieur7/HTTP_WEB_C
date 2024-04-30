cmake_minimum_required(VERSION 3.0)

project(HTTP_WEB_C)
set(SOURCES main.cpp socket.cpp socket.hpp
    parseString.cpp parseString.hpp generateHeaders.cpp
    generateHeaders.hpp FileTypeDetector.hpp FileTypeDetector.cpp
    socketSecure.cpp socketSecure.hpp)

# set MIN C++ version
set(CMAKE_CXX_STANDARD 20)
add_executable(${PROJECT_NAME} ${SOURCES})

# link against boost :
find_package(Boost 1.58.0 COMPONENTS system REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_DEBUG} -O2 -Wall -Wextra -Wpedantic -std=c++2a")

# link against openssl :
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES})

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
    tests/FileTypeDetectorTest.cc
    FileTypeDetector.cpp
    socket.cpp
    parseString.cpp
    generateHeaders.cpp
)
target_link_libraries(
    unit_tests
    gtest_main
)
include(GoogleTest)
gtest_discover_tests(unit_tests)