#ifndef _SERVER_DB_H
#define _SERVER_DB_H
#include <map>
#include <vector>
#include "sqlite-amalgamation-3081002/sqlite3.h"

using DatabaseRow = std::map<std::string, std::string>;
using DatabaseResults = std::vector<DatabaseRow>;

void exec_database(sqlite3 * db, const char * command);

void exec_database_with_results(sqlite3 * db, const char * command, DatabaseResults * results);

void open_database(sqlite3 ** db);

#endif