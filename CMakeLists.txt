cmake_minimum_required(VERSION 3.0)
project(virtualescrow)

# Setup
find_package(Protobuf 2.6.1 REQUIRED)

file(GLOB PROTO_FILES "${CMAKE_SOURCE_DIR}/protos/*.proto")
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

# Server executable
add_library(SQLITE sqlite-amalgamation-3081002/sqlite3.c)
add_executable(ve-server server.cpp ${PROTO_SRCS})
target_link_libraries(ve-server SQLITE pthread dl ${PROTOBUF_LIBRARIES})
target_include_directories(ve-server PUBLIC ${CMAKE_BINARY_DIR} ${PROTOBUF_INCLUDE_DIRS})

# Client executable
add_executable(ve-client client.cpp ${PROTO_SRCS})
target_link_libraries(ve-client SQLITE pthread dl ${PROTOBUF_LIBRARIES})
target_include_directories(ve-client PUBLIC ${CMAKE_BINARY_DIR} ${PROTOBUF_INCLUDE_DIRS})

# Install binaries
install(TARGETS ve-server ve-client RUNTIME DESTINATION bin)
