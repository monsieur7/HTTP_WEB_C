cmake_minimum_required(VERSION 3.0)

project(HTTP_WEB_C)
set(SOURCES main.cpp socket.cpp socket.hpp)

add_executable(${PROJECT_NAME} ${SOURCES})
