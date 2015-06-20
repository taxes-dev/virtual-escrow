#include <sstream>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include "server/server-db.h"
#include "server/server-process.h"
#include "shared/messageformat.h"
#include "shared/shared.h"
#include "echo.pb.h"
#include "session.pb.h"
#include "trade.pb.h"

namespace escrow {
	using namespace std;
	
	void ServerProcess::handle_AvailableTradePartnersRequest(const AvailableTradePartnersRequest * partnersRequest) {
		stringstream query;
		DatabaseResults results;
		DatabaseResults::iterator iter;
		uuid_t u_client_id;
		char s_client_id[UUID_STR_SIZE];
		
		AvailableTradePartnersResponse * partnersResponse = new AvailableTradePartnersResponse();
		
		uuid_unparse(this->m_client_id, s_client_id);
		query << "SELECT client_id FROM sessions WHERE client_id != '" << s_client_id << "';" << std::endl;
		this->db->exec_with_results(query.str(), &results);
		
		for (iter = results.begin(); iter < results.end(); ++iter) {
			DatabaseRow row = *iter;
			
			bzero((char *)u_client_id, sizeof(uuid_t));
			uuid_parse(row["client_id"].c_str(), u_client_id);
			partnersResponse->add_client_id((char *)u_client_id, sizeof(uuid_t));
		}
		
		socket_write_message(this->m_sock_fd, MSG_ID_AVAILABLETRADEPARTNERSRESPONSE, partnersResponse);
		
		delete partnersResponse;
	}
	
	void ServerProcess::handle_EchoRequest(const EchoRequest * echoRequest) {
		stringstream logmsg;
		
		logmsg << "Here is the message: " << echoRequest->message() << std::endl; 
		info(logmsg.str().c_str());
		
		EchoResponse * echoResponse = new EchoResponse();
		echoResponse->set_message(echoRequest->message());
		
		socket_write_message(this->m_sock_fd, MSG_ID_ECHORESPONSE, echoResponse);
		
		delete echoResponse;
	}
	
	void ServerProcess::handle_SessionStartRequest(const SessionStartRequest * sessionStartRequest) {
		stringstream logmsg, query;
		char s_client_id[UUID_STR_SIZE], s_session_id[UUID_STR_SIZE];
		SessionStartResponse * sessionStartResponse = new SessionStartResponse();
		
		// message out of order?
		if (this->m_connected) {
			sessionStartResponse->set_client_id(this->m_client_id, sizeof(uuid_t));
			sessionStartResponse->set_session_id(this->m_session_id, sizeof(uuid_t));
			sessionStartResponse->set_error(SessionStartError::SESSION_STARTED);
		} else {
			// get client ID from message
			sessionStartRequest->client_id().copy((char *)this->m_client_id, sizeof(uuid_t));
			uuid_unparse(this->m_client_id, s_client_id);
			logmsg << "Got connection from: " << s_client_id << std::endl;
			info(logmsg.str().c_str());
			
			// create response
			sessionStartResponse->set_client_id(this->m_client_id, sizeof(uuid_t));
			
			// check for duplicate session
			query << "SELECT session_id FROM sessions WHERE client_id = '" << s_client_id << "';" << std::endl;
			DatabaseResults results;
			this->db->exec_with_results(query.str(), &results);
			if (results.size() > 0) {
				// duplicate client
				uuid_parse(results.back()["session_id"].c_str(), this->m_session_id); 
				sessionStartResponse->set_session_id(this->m_session_id, sizeof(uuid_t));
				sessionStartResponse->set_error(SessionStartError::CLIENT_ALREADY_CONNECTED);
			} else {
				// generate new session id and record
				uuid_generate(this->m_session_id);
				this->m_connected = true;
				uuid_unparse(this->m_session_id, s_session_id);
				sessionStartResponse->set_session_id(this->m_session_id, sizeof(uuid_t));
				sessionStartResponse->set_error(SessionStartError::OK);
				
				query.str("");
				query << "INSERT INTO sessions VALUES ('" << s_client_id << "', '" << s_session_id << "');" << std::endl;
				this->db->exec(query.str());
			}
		}
		
		socket_write_message(this->m_sock_fd, MSG_ID_SESSIONSTARTRESPONSE, sessionStartResponse);
		
		delete sessionStartResponse;
	}
	
	void ServerProcess::process() {
		char buffer[MESSAGE_BUFFER_SIZE];
		int n;
		struct pollfd sds;
		std::stringstream query;
		char s_client_id[UUID_STR_SIZE];
		
		this->db = new ServerDatabase();
		this->db->open();
		
		while (1) {
			sds.fd = this->m_sock_fd;
			sds.events = POLLIN | POLLPRI | POLLHUP;
			sds.revents = 0;
			
			if (poll(&sds, 1, 100) == 1) {
				if (sds.revents & POLLHUP) {
					break;
				} else if (sds.revents & POLLPRI) {
					n = recv(this->m_sock_fd, buffer, MESSAGE_BUFFER_SIZE, MSG_OOB);
				} else {
					n = recv(this->m_sock_fd, buffer, MESSAGE_BUFFER_SIZE, 0);
				}
				
				if (n == 0) {
					break;
				}
				
				if (n < 0) {
					error("ERROR reading from socket");
				}
				
				message_dispatch(buffer, n, [this](int message_id, google::protobuf::MessageLite * message) {
					switch (message_id) {
						case MSG_ID_AVAILABLETRADEPARTNERSREQUEST:
							this->handle_AvailableTradePartnersRequest((escrow::AvailableTradePartnersRequest *)message);
							break;
						case MSG_ID_ECHOREQUEST:
							this->handle_EchoRequest((escrow::EchoRequest *)message);
							break;
						case MSG_ID_SESSIONSTARTREQUEST:
							this->handle_SessionStartRequest((escrow::SessionStartRequest *)message);
							break;
						default:
							error("ERROR unhandled message");
							break;
					}
				});
			}
		}
		
		info("Client disconnected, shutting down");
		uuid_unparse(this->m_client_id, s_client_id);
		query << "DELETE FROM sessions WHERE client_id = '" << s_client_id << "';" << std::endl;
		this->db->exec(query.str());
		this->db->close();
		delete this->db;
	}
	
}