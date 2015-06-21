#ifndef H_SERVER_PROCESS
#define H_SERVER_PROCESS
#include <uuid/uuid.h>
#include "echo.pb.h"
#include "session.pb.h"
#include "trade.pb.h"
#include "server/server-db.h"
#include "shared/shared.h"

namespace escrow {

	class ServerProcess {
	public:
		ServerProcess(const int sock_fd) { this->m_sock_fd = sock_fd; };
		void process();
	private:
		bool m_connected = false;
		uuid_t m_client_id;
		char m_str_client_id[UUID_STR_SIZE];
		uuid_t m_session_id;
		char m_str_session_id[UUID_STR_SIZE];
		ServerDatabase * db;
		int m_sock_fd;
		
		template <typename T>
		void handle(const T * message, const uuid_t & request_id) { };
		void set_client_id(const std::string & client_id);
		void start_session();
	};
}

#endif