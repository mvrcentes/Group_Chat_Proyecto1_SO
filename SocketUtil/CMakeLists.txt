cmake_minimum_required(VERSION 3.27)
project(SocketUtil C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_library(SocketUtil
        socketutil.c
        socketutil.h)
