cmake_minimum_required(VERSION 3.0)
project(virtualescrow)

# Setup
find_package(Protobuf 2.6.1 REQUIRED)
set(FLTK_DIR /usr/local/src/fltk-1.3.3)
find_package(FLTK REQUIRED NO_MODULE)
include(${FLTK_USE_FILE})


file(GLOB PROTO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/protos/*.proto")
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${PROTOBUF_INCLUDE_DIRS})

# Dependencies
add_subdirectory(sqlite-amalgamation-3081002)
add_subdirectory(server)
add_subdirectory(client)
add_subdirectory(shared)

add_library(GENPROTOLIB ${PROTO_SRCS})
target_compile_options(GENPROTOLIB PUBLIC "-std=c++11")

# Server executable
add_executable(ve-server server.cpp)
target_compile_options(ve-server PUBLIC "-std=c++11")
target_link_libraries(ve-server SERVERLIB GENPROTOLIB SHAREDLIB SQLITE pthread dl uuid ${PROTOBUF_LIBRARIES})

# Client executable
add_executable(ve-client client.cpp)
target_compile_options(ve-client PUBLIC "-std=c++11")
target_link_libraries(ve-client CLIENTLIB GENPROTOLIB SHAREDLIB uuid fltk ${PROTOBUF_LIBRARIES})

# Install binaries
install(TARGETS ve-server ve-client RUNTIME DESTINATION bin)

