cmake_minimum_required(VERSION 2.6)
project(virtualescrow)

add_definitions(-fpermissive)
add_executable(ve-server server.cpp)
add_executable(ve-client client.cpp)

install(TARGETS ve-server ve-client RUNTIME DESTINATION bin)
