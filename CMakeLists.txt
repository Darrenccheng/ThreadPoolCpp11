cmake_minimum_required(VERSION 3.17)
project(ThreadPoolCPP11)

set(CMAKE_CXX_STANDARD 11)

file(GLOB SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(ThreadPoolCPP11 ${SRC})

target_link_libraries(ThreadPoolCPP11 pthread)