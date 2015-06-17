#include <iostream>
#include <map>
#include <sstream>
#include <vector>
#include "sqlite-amalgamation-3081002/sqlite3.h"
#include "shared.h"
#include "server-db.h"

void close_datbase(sqlite3 * db) {
	sqlite3_close(db);
}

static int db_callback(void * results, int argc, char **argv, char **colName) {
	DatabaseRow row;
	for (int i = 0; i < argc; i++) {
		row[std::string(colName[i])] = argv[i] ? std::string(argv[i]) : nullptr;
		//std::cout << colName[i] << " = " << (argv[i] ? argv[i] : "NULL") << std::endl;
	}
	((DatabaseResults *)results)->push_back(row);
	return 0;
}

void exec_database(sqlite3 * db, const std::string & command) {
	char *errmsg = 0;
	int rc = sqlite3_exec(db, command.c_str(), 0, 0, &errmsg);
	if (rc != SQLITE_OK) {
		error(errmsg);
		sqlite3_free(errmsg);
	}
}

void exec_database_with_results(sqlite3 * db, const std::string & command, DatabaseResults * results) {
	char *errmsg = 0;
	
	int rc = sqlite3_exec(db, command.c_str(), db_callback, results, &errmsg);
	
	if (rc != SQLITE_OK) {
		error(errmsg);
		sqlite3_free(errmsg);
	}
}

void open_database(sqlite3 ** db) {
	std::stringstream query;
	
	// open database
	int rc = sqlite3_open(DB_NAME, db);
	if (rc) {
		sqlite3_close(*db);
		error("ERROR can't open db");
	}
	
	// create schema if it doesn't exist
	query << "CREATE TABLE IF NOT EXISTS sessions (client_id TEXT UNIQUE, session_id TEXT);"
	<< std::endl;
	
	exec_database(*db, query.str());
}

void clean_database() {
	sqlite3 * db;
	open_database(&db);
	
	// remove any sessions from previous runs
	exec_database(db, "DELETE FROM sessions;");
	
	close_datbase(db);
}
