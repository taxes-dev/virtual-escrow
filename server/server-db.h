#ifndef _SERVER_DB_H
#define _SERVER_DB_H
#include <map>
#include <vector>
#include "sqlite-amalgamation-3081002/sqlite3.h"

#define DB_NAME "virtualescrow.db"

namespace escrow {
	using DatabaseRow = std::map<std::string, std::string>;
	using DatabaseResults = std::vector<DatabaseRow>;

	class ServerDatabase {
	public:
	static void clean();

	void close();

	void exec(const std::string & command) const;

	void exec_with_results(const std::string & command, DatabaseResults * results) const;

	void open();
	
	private:
		sqlite3 * m_db;
	};
}
#endif