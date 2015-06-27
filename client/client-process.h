#ifndef H_CLIENT_PROCESS
#define H_CLIENT_PROCESS
#include <functional>
#include <map>
#include <uuid/uuid.h>
#include "echo.pb.h"
#include "session.pb.h"
#include "trade.pb.h"
#include "shared/messageformat.h"
#include "shared/shared.h"
#include "shared/virtualitems.h"

#define DEFAULT_INVENTORY_SIZE 5

namespace escrow {
	using namespace std;
	
	class ClientProcess : public BaseProcess<ClientProcess> {
		friend BaseProcess<ClientProcess>;
	public:
		ClientProcess(const int sock_fd);
		inline string client_id_parsed() { return this->m_str_client_id; };
		void copy_client_id(uuid_t * dst);
		inline Inventory * inventory() { return &this->m_inventory; };
		bool start_session();

		void cmd_AvailableTradePartnersRequest(const MessageCallback<AvailableTradePartnersResponse> & callback, void * data);
		void cmd_EchoRequest(const string & message, const MessageCallback<EchoResponse> & callback, void * data);
	protected:
		template <typename T>
		void handle(const MessageWrapper * wrapper, const T * message) { };
	private:
		uuid_t m_client_id;
		string m_str_client_id;
		bool m_session_set = false;
		uuid_t m_session_id;
		Inventory m_inventory;

		void cmd_SessionStartRequest();		
	};
	
	inline void ClientProcess::copy_client_id(uuid_t * dst)
	{
		uuid_copy(*dst, this->m_client_id);
	}
}

#endif