cmake_minimum_required(VERSION 3.0)

project(HTTP_WEB_C)
set(SOURCES main.cpp socket.cpp socket.hpp parseString.cpp parseString.hpp)

add_executable(${PROJECT_NAME} ${SOURCES})

# link against boost :
find_package(Boost 1.58.0 COMPONENTS system REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
