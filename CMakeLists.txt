cmake_minimum_required(VERSION 3.27)
project(os C)

set(CMAKE_C_STANDARD 99)

include_directories(.)

add_executable(os
        main.c
        scanner.c
        scanner.h
        shell.c
        shell.h
        pipeLine.c
        pipeLine.h)
