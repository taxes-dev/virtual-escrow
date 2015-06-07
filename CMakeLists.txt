cmake_minimum_required(VERSION 2.6)
project(virtualescrow)

execute_process(COMMAND pkg-config --cflags protobuf OUTPUT_VARIABLE PROTOBUF_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND pkg-config --libs protobuf OUTPUT_VARIABLE PROTOBUF_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)

add_library(SQLITE sqlite-amalgamation-3081002/sqlite3.c)
add_executable(ve-server server.cpp)
set_target_properties(ve-server PROPERTIES COMPILE_FLAGS "${PROTOBUF_CFLAGS}")
set_target_properties(ve-server PROPERTIES LINK_FLAGS "${PROTOBUF_LIBS}")
target_link_libraries(ve-server SQLITE pthread dl)
add_executable(ve-client client.cpp)
set_target_properties(ve-client PROPERTIES COMPILE_FLAGS "${PROTOBUF_CFLAGS}")
set_target_properties(ve-client PROPERTIES LINK_FLAGS "${PROTOBUF_LIBS}")
target_link_libraries(ve-client SQLITE pthread dl)

install(TARGETS ve-server ve-client RUNTIME DESTINATION bin)
