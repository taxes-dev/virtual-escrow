#ifndef H_CLIENT_PROCESS
#define H_CLIENT_PROCESS
#include <functional>
#include <map>
#include <uuid/uuid.h>
#include "echo.pb.h"
#include "session.pb.h"
#include "trade.pb.h"
#include "shared/virtualitems.h"

#define DEFAULT_INVENTORY_SIZE 5

namespace escrow {
	using namespace std;
	
	template <typename T>
	using MessageCallback = function<void(const T *, void * data)>;
	using MessageCallbackInt = function<void(const google::protobuf::MessageLite *)>;
	
	class ClientProcess {
	public:
		ClientProcess(const int sock_fd);
		inline string client_id_parsed() { return this->m_str_client_id; };
		inline Inventory * inventory() { return &this->m_inventory; };
		bool process_message(bool wait);
		bool start_session();

		void cmd_AvailableTradePartnersRequest(const MessageCallback<AvailableTradePartnersResponse> & callback, void * data);
		void cmd_EchoRequest(const string & message, const MessageCallback<EchoResponse> & callback, void * data);
	private:
		int m_sock_fd;
		uuid_t m_client_id;
		string m_str_client_id;
		bool m_session_set = false;
		uuid_t m_session_id;
		Inventory m_inventory;
		map<string, MessageCallbackInt> m_callbacks;

		void add_callback(const uuid_t & request_id, const MessageCallbackInt & callback);
		void cmd_SessionStartRequest();
		void invoke_callback(const uuid_t & request_id, const google::protobuf::MessageLite * message);
		
		template <typename T>
		void handle(const T * message) { };
	};
}

#endif