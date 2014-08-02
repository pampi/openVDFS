cmake_minimum_required(VERSION 2.8)

project(openVDFS)

set(GNU_CXX_ADDITIONAL_FLAGS "-g -Wall -std=c++11")

if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GNU_CXX_ADDITIONAL_FLAGS}")
endif(CMAKE_COMPILER_IS_GNUCXX)

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_DEPRECATE -DSHARED_LIBRARY_EXPORT)
endif(MSVC)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(openVDFS SHARED openVDFS.cpp)

add_executable(testVDF openVDFS.cpp test/main.cpp)
