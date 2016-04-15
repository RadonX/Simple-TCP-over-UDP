cmake_minimum_required(VERSION 3.5)
project(TCPlikeProtocol)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp sender.cpp receiver.cpp)
add_executable(TCPlikeProtocol ${SOURCE_FILES})