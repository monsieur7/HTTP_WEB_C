cmake_minimum_required(VERSION 3.0)

# if cmake is 3.24 or newer :
if(CMAKE_VERSION GREATER 3.24)
    cmake_policy(SET DOWNLOAD_EXTRACT_TIMESTAMP NEW)
endif()

project(HTTP_WEB_C)
set(SOURCES main.cpp socket.cpp socket.hpp
    parseString.cpp parseString.hpp generateHeaders.cpp
    generateHeaders.hpp FileTypeDetector.hpp FileTypeDetector.cpp
    socketSecure.cpp socketSecure.hpp BME280.hpp BME280.cpp LTR559.hpp LTR559.cpp
    ADS1015.hpp ADS1015.cpp)

# MICS6814.hpp MICS6814.cpp)

# set MIN C++ version
set(CMAKE_CXX_STANDARD 20)
add_executable(${PROJECT_NAME} ${SOURCES})

# link against openssl :
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES})

# link against libgpiod :
find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED IMPORTED_TARGET libgpiod)
target_link_libraries(${PROJECT_NAME} PkgConfig::GPIOD)

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
