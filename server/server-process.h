#ifndef H_SERVER_PROCESS
#define H_SERVER_PROCESS
#include <uuid/uuid.h>
#include "echo.pb.h"
#include "session.pb.h"
#include "trade.pb.h"
#include "server/server-db.h"

namespace escrow {

	class ServerProcess {
	public:
		ServerProcess(int sock_fd) { this->m_sock_fd = sock_fd; };
		void process();
	private:
		bool m_connected = false;
		uuid_t m_client_id;
		uuid_t m_session_id;
		ServerDatabase * db;
		int m_sock_fd;
		
		template <typename T>
		void handle(const T * message) { };
	};
}

#endif