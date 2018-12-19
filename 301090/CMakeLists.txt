cmake_minimum_required(VERSION 3.13)
project(301090 C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(301090
        list.c
        list.h
        lock.c
        lock.h
        own_types.h
        region.c
        region.h
        segment.c
        segment.h
        test.cpp
        tm.c
        tm.h
        transaction.c
        transaction.h
        version_list.c
        version_list.h
        version_list_item.c
        version_list_item.h)
