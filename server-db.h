#ifndef _SERVER_DB_H
#define _SERVER_DB_H
#include "sqlite-amalgamation-3081002/sqlite3.h"

void exec_database(sqlite3 * db, const char * command);

void open_database(sqlite3 ** db);

#endif