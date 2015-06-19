#include <iostream>
#include <map>
#include <sstream>
#include <vector>
#include "sqlite-amalgamation-3081002/sqlite3.h"
#include "shared/shared.h"
#include "server/server-db.h"

namespace escrow {
	static int db_callback(void * results, int argc, char **argv, char **colName) {
		DatabaseRow row;
		for (int i = 0; i < argc; i++) {
			row[std::string(colName[i])] = argv[i] ? std::string(argv[i]) : nullptr;
			//std::cout << colName[i] << " = " << (argv[i] ? argv[i] : "NULL") << std::endl;
		}
		((DatabaseResults *)results)->push_back(row);
		return 0;
	}
	
	void ServerDatabase::close() {
		sqlite3_close(this->m_db);
	}


	void ServerDatabase::exec(const std::string & command) const {
		char *errmsg = 0;
		int rc = sqlite3_exec(this->m_db, command.c_str(), 0, 0, &errmsg);
		if (rc != SQLITE_OK) {
			error(errmsg);
			sqlite3_free(errmsg);
		}
	}

	void ServerDatabase::exec_with_results(const std::string & command, DatabaseResults * results) const {
		char *errmsg = 0;
		
		int rc = sqlite3_exec(this->m_db, command.c_str(), db_callback, results, &errmsg);
		
		if (rc != SQLITE_OK) {
			error(errmsg);
			sqlite3_free(errmsg);
		}
	}

	void ServerDatabase::open() {
		std::stringstream query;
		
		// open database
		int rc = sqlite3_open(DB_NAME, &(this->m_db));
		if (rc) {
			sqlite3_close(this->m_db);
			error("ERROR can't open db");
		}
		
		// create schema if it doesn't exist
		query << "CREATE TABLE IF NOT EXISTS sessions (client_id TEXT UNIQUE, session_id TEXT);"
		<< std::endl;
		
		this->exec(query.str());
	}

	void ServerDatabase::clean() {
		ServerDatabase db;
		db.open();
		// remove any sessions from previous runs
		db.exec("DELETE FROM sessions;");
		db.close();
	}
}