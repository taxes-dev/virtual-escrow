add_library(SERVERLIB server-db.cpp server-process.cpp)
target_compile_options(SERVERLIB PUBLIC "-std=c++11")
