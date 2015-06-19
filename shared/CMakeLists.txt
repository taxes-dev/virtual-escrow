add_library(SHAREDLIB shared.cpp virtualitems.cpp)
target_compile_options(SHAREDLIB PUBLIC "-std=c++11")
