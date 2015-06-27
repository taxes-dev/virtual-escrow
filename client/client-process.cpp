#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <uuid/uuid.h>
#include "echo.pb.h"
#include "session.pb.h"
#include "trade.pb.h"
#include "client/client-process.h"
#include "shared/shared.h"

namespace escrow {
	using namespace std;
	
	ClientProcess::ClientProcess(const int sock_fd) : BaseProcess(sock_fd)
	{
		char t_client_id[UUID_STR_SIZE];
		
		uuid_generate(this->m_client_id);
		uuid_unparse(this->m_client_id, t_client_id);
		this->m_str_client_id = string(t_client_id, UUID_STR_SIZE);
		
		generate_random_inventory(this->m_inventory, DEFAULT_INVENTORY_SIZE, this->m_client_id);
	}
			
	void ClientProcess::cmd_AvailableTradePartnersRequest(const MessageCallback<AvailableTradePartnersResponse> & callback, void * data) {
		uuid_t request_id;
		AvailableTradePartnersRequest partnersRequest;
		
		uuid_generate(request_id);
		this->socket_write_message(MSG_ID_AVAILABLETRADEPARTNERSREQUEST, request_id, &partnersRequest);
		
		this->add_callback(request_id, [callback, data](const google::protobuf::MessageLite * message) {
			callback(static_cast<const AvailableTradePartnersResponse *>(message), data);
		});
	}

	void ClientProcess::cmd_EchoRequest(const string & message, const MessageCallback<EchoResponse> & callback, void * data) {
		uuid_t request_id;
		EchoRequest echoRequest;
		
		uuid_generate(request_id);
		echoRequest.set_message(message);
		this->socket_write_message(MSG_ID_ECHOREQUEST, request_id, &echoRequest);

		this->add_callback(request_id, [callback, data](const google::protobuf::MessageLite * message){
			callback(static_cast<const EchoResponse *>(message), data);
		});
	}

	void ClientProcess::cmd_SessionStartRequest() {
		escrow::SessionStartRequest sessionStartRequest;
		sessionStartRequest.set_client_id(this->m_client_id, sizeof(uuid_t));
		this->socket_write_message(MSG_ID_SESSIONSTARTREQUEST, nullptr, &sessionStartRequest);
	}

	template<> void ClientProcess::handle(const MessageWrapper * wrapper, const escrow::AvailableTradePartnersResponse * partnersResponse) {
		/*uuid_t u_client_id;
		char s_client_id[UUID_STR_SIZE];
		google::protobuf::RepeatedPtrField<std::string>::const_iterator iter;
		
		if (partnersResponse->client_id_size() > 0) {
			for (iter = partnersResponse->client_id().begin(); iter < partnersResponse->client_id().end(); ++iter) {
				std::string client_id = *iter;
				std::stringstream clientmsg;
				
				bzero((char *)u_client_id, sizeof(uuid_t));
				bzero((char *)s_client_id, UUID_STR_SIZE);
				client_id.copy((char *)u_client_id, sizeof(uuid_t));
				uuid_unparse(u_client_id, s_client_id);
				
				clientmsg << "Available trade partner: " << s_client_id << std::endl;
				info(clientmsg.str().c_str());
			}
		} else {
			info("No available trade partners");
		}*/
	}

	template<> void ClientProcess::handle(const MessageWrapper * wrapper, const escrow::EchoResponse * echoResponse) {
		std::stringstream logmsg;
		logmsg << "Got response: " << echoResponse->message() << std::endl;
		Logger::info(logmsg.str());
	}

	template<> void ClientProcess::handle(const MessageWrapper * wrapper, const escrow::SessionStartResponse * sessionStartResponse) {
		switch (sessionStartResponse->error()) {
			case SessionStartError::OK:
				sessionStartResponse->session_id().copy((char *)this->m_session_id, sizeof(uuid_t));
				this->m_session_set = true;
				break;
			case SessionStartError::CLIENT_ALREADY_CONNECTED:
			case SessionStartError::SESSION_STARTED:
				break;
			default:
				Logger::warn("ERROR unknown session start error code");
				break;
		}
	}
	
	bool ClientProcess::start_session()
	{
		this->cmd_SessionStartRequest();
		this->process_message(PROCESS_TIMEOUT_BLOCK); // block until we get a SessionStartResponse
		return this->m_session_set;
	}

}