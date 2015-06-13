#include <iostream>
#include <sstream>
#include "sqlite-amalgamation-3081002/sqlite3.h"
#include "shared.h"
#include "server-db.h"

static int db_callback(void * notUsed, int argc, char **argv, char **colName) {
	for (int i = 0; i < argc; i++) {
		std::cout << colName[i] << " = " << (argv[i] ? argv[i] : "NULL") << std::endl;
	}
	return 0;
}

void exec_database(sqlite3 * db, const char * command) {
	char *errmsg = 0;
	int rc = sqlite3_exec(db, command, db_callback, 0, &errmsg);
	if (rc != SQLITE_OK) {
		error(errmsg);
		sqlite3_free(errmsg);
	}
}

void open_database(sqlite3 ** db) {
	std::stringstream query;
	
	// open database
	int rc = sqlite3_open("virtualescrow.db", db);
	if (rc) {
		sqlite3_close(*db);
		error("ERROR can't open db");
	}
	
	// create schema if it doesn't exist
	query << "CREATE TABLE IF NOT EXISTS sessions (client_id TEXT UNIQUE, session_id TEXT);"
	<< std::endl;
	
	exec_database(*db, query.str().c_str());
}
