#ifndef H_CLIENT_PROCESS
#define H_CLIENT_PROCESS
#include <uuid/uuid.h>
#include "echo.pb.h"
#include "session.pb.h"
#include "trade.pb.h"
#include "shared/virtualitems.h"

#define DEFAULT_INVENTORY_SIZE 5

namespace escrow {
	using namespace std;
	
	class ClientProcess {
	public:
		ClientProcess(const int sock_fd);
		~ClientProcess();
		inline string client_id_parsed() { return this->m_str_client_id; };
		bool process_message(bool wait);
		void show_inventory();
		bool start_session();

		void cmd_AvailableTradePartnersRequest();
		void cmd_EchoRequest();
		void cmd_SessionStartRequest();
	private:
		int m_sock_fd;
		uuid_t m_client_id;
		string m_str_client_id;
		bool m_session_set = false;
		uuid_t m_session_id;
		Inventory * m_inventory;
		
		void handle_AvailableTradePartnersResponse(const escrow::AvailableTradePartnersResponse * partnersResponse);
		void handle_EchoResponse(const escrow::EchoResponse * echoResponse);
		void handle_SessionStartResponse(const escrow::SessionStartResponse * sessionStartResponse);
	};
}

#endif