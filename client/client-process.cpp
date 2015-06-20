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
	
	ClientProcess::ClientProcess(const int sock_fd)
	{
		char t_client_id[UUID_STR_SIZE];
		
		uuid_generate(this->m_client_id);
		uuid_unparse(this->m_client_id, t_client_id);
		this->m_str_client_id = string(t_client_id, UUID_STR_SIZE);
		this->m_sock_fd = sock_fd;
		this->m_inventory = new Inventory();
		
		generate_random_inventory(this->m_inventory, DEFAULT_INVENTORY_SIZE, this->m_client_id);
	}
	
	ClientProcess::~ClientProcess()
	{
		free_inventory_items(this->m_inventory);
		delete this->m_inventory;
	}
	
	void ClientProcess::cmd_AvailableTradePartnersRequest() {
		AvailableTradePartnersRequest partnersRequest;
		socket_write_message(this->m_sock_fd, MSG_ID_AVAILABLETRADEPARTNERSREQUEST, &partnersRequest);
	}

	void ClientProcess::cmd_EchoRequest() {
		std::cout << "Please enter the message: " << std::endl;
		std::string input;
		std::getline(std::cin, input);
		
		escrow::EchoRequest echoRequest;
		echoRequest.set_message(input);
		socket_write_message(this->m_sock_fd, MSG_ID_ECHOREQUEST, &echoRequest);
	}

	void ClientProcess::cmd_SessionStartRequest() {
		escrow::SessionStartRequest sessionStartRequest;
		sessionStartRequest.set_client_id(this->m_client_id, sizeof(uuid_t));
		socket_write_message(this->m_sock_fd, MSG_ID_SESSIONSTARTREQUEST, &sessionStartRequest);
	}

	void ClientProcess::handle_AvailableTradePartnersResponse(const escrow::AvailableTradePartnersResponse * partnersResponse) {
		uuid_t u_client_id;
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
		}
	}

	void ClientProcess::handle_EchoResponse(const escrow::EchoResponse * echoResponse) {
		std::stringstream logmsg;
		logmsg << "Got response: " << echoResponse->message() << std::endl;
		info(logmsg.str().c_str());
	}

	void ClientProcess::handle_SessionStartResponse(const escrow::SessionStartResponse * sessionStartResponse) {
		switch (sessionStartResponse->error()) {
			case SessionStartError::OK:
				sessionStartResponse->session_id().copy((char *)this->m_session_id, sizeof(uuid_t));
				this->m_session_set = true;
				break;
			case SessionStartError::CLIENT_ALREADY_CONNECTED:
			case SessionStartError::SESSION_STARTED:
				break;
			default:
				error("ERROR unknown session start error code");
				break;
		}
	}

	bool ClientProcess::process_message(bool wait) {
		struct pollfd sds;
		char buffer[MESSAGE_BUFFER_SIZE];
		int n = 0;
		
		sds.fd = this->m_sock_fd;
		sds.events = POLLIN | POLLPRI | POLLHUP;
		
		if (poll(&sds, 1, wait ? -1 : 0) == 1) {
			if (sds.revents & POLLHUP) {
				return false;
			} else if (sds.revents & POLLPRI) {
				n = recv(this->m_sock_fd, buffer, MESSAGE_BUFFER_SIZE, MSG_OOB);
			} else {
				n = recv(this->m_sock_fd, buffer, MESSAGE_BUFFER_SIZE, 0);
			}
			
			if (n == 0) {
				return false;
			}
			
			if (n < 0) {
				error("ERROR reading from socket");
			}
			
			if (n > 0) {
				message_dispatch(buffer, n, [this](int message_id, google::protobuf::MessageLite * message) {
					switch (message_id) {
						case MSG_ID_ECHORESPONSE:
							this->handle_EchoResponse((escrow::EchoResponse *)message);
							break;
						case MSG_ID_SESSIONSTARTRESPONSE:
							this->handle_SessionStartResponse((escrow::SessionStartResponse *)message);
							break;
						case MSG_ID_AVAILABLETRADEPARTNERSRESPONSE:
							this->handle_AvailableTradePartnersResponse((escrow::AvailableTradePartnersResponse *)message);
							break;
						default:
							error("ERROR unhandled message");
							break;
					}
				});
			}
		}
		
		return true;
	}

	void ClientProcess::show_inventory() {
		escrow::Inventory::iterator iter;
		int i = 1;
		uuid_t owner_id;
		
		std::cout << "Current inventory:" << std::endl;
		for (iter = this->m_inventory->begin(); iter < this->m_inventory->end(); ++iter, ++i) {
			escrow::VirtualItem * item = *iter;
			
			bzero(owner_id, sizeof(uuid_t));
			item->copy_original_owner_id(&owner_id);
			
			std::cout << i << ") " << item->desc() << " [instance " << item->instance_id_parsed() << "] [local: " << (
				uuid_compare(this->m_client_id, owner_id) == 0 ? "Y" : "N"
			) << "]" << std::endl;
		}
		std::cout << std::endl;
	}
	
	bool ClientProcess::start_session()
	{
		this->cmd_SessionStartRequest();
		this->process_message(true); // block until we get a SessionStartResponse
		return this->m_session_set;
	}

}