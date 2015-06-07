cmake_minimum_required(VERSION 2.6)
project(virtualescrow)

add_library(SQLITE sqlite-amalgamation-3081002/sqlite3.c)
add_executable(ve-server server.cpp)
target_link_libraries(ve-server SQLITE pthread dl)
add_executable(ve-client client.cpp)
target_link_libraries(ve-client SQLITE pthread dl)

install(TARGETS ve-server ve-client RUNTIME DESTINATION bin)
