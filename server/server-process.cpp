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
	
	template<> void ServerProcess::handle(const AvailableTradePartnersRequest * partnersRequest) {
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
	
	template<> void ServerProcess::handle(const EchoRequest * echoRequest) {
		stringstream logmsg;
		
		logmsg << "Here is the message: " << echoRequest->message() << std::endl; 
		info(logmsg.str().c_str());
		
		EchoResponse * echoResponse = new EchoResponse();
		echoResponse->set_message(echoRequest->message());
		
		socket_write_message(this->m_sock_fd, MSG_ID_ECHORESPONSE, echoResponse);
		
		delete echoResponse;
	}
	
	template<> void ServerProcess::handle(const SessionStartRequest * sessionStartRequest) {
		SessionStartResponse * sessionStartResponse = new SessionStartResponse();
		
		// message out of order?
		if (this->m_connected) {
			sessionStartResponse->set_client_id(this->m_client_id, sizeof(uuid_t));
			sessionStartResponse->set_session_id(this->m_session_id, sizeof(uuid_t));
			sessionStartResponse->set_error(SessionStartError::SESSION_STARTED);
		} else {
			stringstream logmsg, query;
			DatabaseResults results;
			
			// get client ID from message
			this->set_client_id(sessionStartRequest->client_id());
			
			logmsg << "Got connection from: " << this->m_str_client_id << std::endl;
			info(logmsg.str().c_str());
			
			// create response
			sessionStartResponse->set_client_id(this->m_client_id, sizeof(uuid_t));
			
			// check for duplicate session
			query << "SELECT session_id FROM sessions WHERE client_id = '" << this->m_str_client_id << "';" << std::endl;
			
			this->db->exec_with_results(query.str(), &results);
			if (results.size() > 0) {
				// duplicate client
				uuid_t t_session_id;
				uuid_parse(results.back()["session_id"].c_str(), t_session_id); 
				sessionStartResponse->set_session_id(t_session_id, sizeof(uuid_t));
				sessionStartResponse->set_error(SessionStartError::CLIENT_ALREADY_CONNECTED);
			} else {
				// generate new session id and record
				this->start_session();
				sessionStartResponse->set_session_id(this->m_session_id, sizeof(uuid_t));
				sessionStartResponse->set_error(SessionStartError::OK);
				
				query.str("");
				query << "INSERT INTO sessions VALUES ('" << this->m_str_client_id << "', '" << this->m_str_session_id << "');" << std::endl;
				this->db->exec(query.str());
			}
		}
		
		socket_write_message(this->m_sock_fd, MSG_ID_SESSIONSTARTRESPONSE, sessionStartResponse);
		
		delete sessionStartResponse;
	}
	
	void ServerProcess::set_client_id(const string & client_id)
	{
		if (this->m_connected == false) {
			client_id.copy((char *)this->m_client_id, sizeof(uuid_t));
			uuid_unparse(this->m_client_id, this->m_str_client_id);
		}
	}

	void ServerProcess::start_session()
	{
		if (this->m_connected == false) {
			uuid_generate(this->m_session_id);
			this->m_connected = true;
			uuid_unparse(this->m_session_id, this->m_str_session_id);
		}
	}
	
	void ServerProcess::process() {
		char buffer[MESSAGE_BUFFER_SIZE];
		int n;
		struct pollfd sds;
		
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
					bzero(buffer, MESSAGE_BUFFER_SIZE);
					n = recv(this->m_sock_fd, buffer, MESSAGE_BUFFER_SIZE, MSG_OOB);
				} else {
					bzero(buffer, MESSAGE_BUFFER_SIZE);
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
							this->handle(static_cast<AvailableTradePartnersRequest *>(message));
							break;
						case MSG_ID_ECHOREQUEST:
							this->handle(static_cast<EchoRequest *>(message));
							break;
						case MSG_ID_SESSIONSTARTREQUEST:
							this->handle(static_cast<SessionStartRequest *>(message));
							break;
						default:
							error("ERROR unhandled message");
							break;
					}
				});
			}
		}
		
		info("Client disconnected, shutting down");
		{
			stringstream query;
			query << "DELETE FROM sessions WHERE client_id = '" << this->m_str_client_id << "';" << std::endl;
			this->db->exec(query.str());
			this->db->close();
		}
		delete this->db;
	}
	
}