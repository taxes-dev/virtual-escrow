#ifndef H_SERVER_PROCESS
#define H_SERVER_PROCESS
#include <memory>
#include <uuid/uuid.h>
#include "echo.pb.h"
#include "session.pb.h"
#include "trade.pb.h"
#include "server/server-db.h"
#include "shared/messageformat.h"
#include "shared/shared.h"

namespace escrow {

	class ServerProcess : public BaseProcess<ServerProcess> {
		friend BaseProcess<ServerProcess>;
	public:
		ServerProcess(const int sock_fd) : BaseProcess(sock_fd) { };
		void run();
	protected:
		template <typename T>
		void handle(const MessageWrapper * wrapper, const T * message) { };
	private:
		bool m_connected = false;
		uuid_t m_client_id;
		char m_str_client_id[UUID_STR_SIZE];
		uuid_t m_session_id;
		char m_str_session_id[UUID_STR_SIZE];
		ServerDatabase db;
		
		void set_client_id(const std::string & client_id);
		void start_session();
	};
}

#endif