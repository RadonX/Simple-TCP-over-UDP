cmake_minimum_required(VERSION 3.5)
project(TCPlikeProtocol)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES sender.cpp sharelib.h tcpstate.h pack.h sock.h mytcp.h)
add_executable(TCPlikeProtocol ${SOURCE_FILES})