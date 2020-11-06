cmake_minimum_required(VERSION 3.10)

project(AMessageQueue)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED true)

file(MAKE_DIRECTORY out)
set(CMAKE_OUTPUT_DIRECTORY out)

list(APPEND MQ_INCLUDE include)
list(APPEND MQ_SOURCE source)

include_directories(${MQ_INCLUDE})

file(GLOB MQ_SOURCE_FILES ${MQ_SOURCE}/*.cpp)

add_executable(test main.cpp ${MQ_SOURCE_FILES} A.cpp B.cpp)
target_link_libraries(test pthread)