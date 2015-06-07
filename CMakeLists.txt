cmake_minimum_required(VERSION 2.6)
project(virtualescrow)

add_executable(virtualescrow main.cpp)

install(TARGETS virtualescrow RUNTIME DESTINATION bin)
