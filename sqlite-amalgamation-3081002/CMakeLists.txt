add_library(SQLITE sqlite3.c)
set_source_files_properties(sqlite3.c PROPERTIES COMPILE_FLAGS "-std=c99")