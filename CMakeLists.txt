cmake_minimum_required(VERSION 3.0)

# Set the policy regarding DOWNLOAD_EXTRACT_TIMESTAMP
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

project(HTTP_WEB_C)
set(SOURCES main.cpp BME280.hpp BME280.cpp LTR559.hpp LTR559.cpp
    ADS1015.hpp ADS1015.cpp ST7735.hpp ST7735.cpp
    MICS6814.hpp MICS6814.cpp
)

# set MIN C++ version
set(CMAKE_CXX_STANDARD 20)
add_executable(${PROJECT_NAME} ${SOURCES})

# link against boost :
find_package(Boost 1.58.0 COMPONENTS system REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Wpedantic -std=c++2a -O2")

# link against openssl :
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES})

# link against libgpiod :
find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED IMPORTED_TARGET libgpiod)
target_link_libraries(${PROJECT_NAME} PkgConfig::GPIOD)

# link against freetype :
find_package(Freetype REQUIRED)
include_directories(${FREETYPE_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${FREETYPE_LIBRARIES})

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif()

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

# compile main_testing :
add_executable(main_testing main_testing.cpp socketSecure.cpp socketSecure.hpp socket.cpp socket.hpp parseString.cpp parseString.hpp generateHeaders.cpp generateHeaders.hpp FileTypeDetector.hpp FileTypeDetector.cpp)
target_link_libraries(main_testing ${OPENSSL_LIBRARIES})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Wpedantic -std=c++2a")
