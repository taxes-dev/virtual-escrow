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
	
	template<> void ServerProcess::handle(const MessageWrapper * wrapper, const AvailableTradePartnersRequest * partnersRequest) {
		stringstream query;
		DatabaseResults results;
		DatabaseResults::iterator iter;
		uuid_t u_client_id;
		char s_client_id[UUID_STR_SIZE];
		
		AvailableTradePartnersResponse partnersResponse;
		
		uuid_unparse(this->m_client_id, s_client_id);
		query << "SELECT client_id FROM sessions WHERE client_id != '" << s_client_id << "';" << std::endl;
		this->db.exec_with_results(query.str(), &results);
		
		for (auto row : results) {			
			bzero((char *)u_client_id, sizeof(uuid_t));
			uuid_parse(row["client_id"].c_str(), u_client_id);
			partnersResponse.add_client_id((char *)u_client_id, sizeof(uuid_t));
		}
		
		socket_write_message(MSG_ID_AVAILABLETRADEPARTNERSRESPONSE, wrapper->request_id, &partnersResponse);
	}
	
	template<> void ServerProcess::handle(const MessageWrapper * wrapper, const EchoRequest * echoRequest) {
		stringstream logmsg;
		
		logmsg << "Here is the message: " << echoRequest->message() << std::endl; 
		Logger::info(logmsg.str());
		
		EchoResponse echoResponse;
		echoResponse.set_message(echoRequest->message());
		
		this->socket_write_message(MSG_ID_ECHORESPONSE, wrapper->request_id, &echoResponse);
	}
	
	template<> void ServerProcess::handle(const MessageWrapper * wrapper, const SessionStartRequest * sessionStartRequest) {
		SessionStartResponse sessionStartResponse;
		
		// message out of order?
		if (this->m_connected) {
			sessionStartResponse.set_client_id(this->m_client_id, sizeof(uuid_t));
			sessionStartResponse.set_session_id(this->m_session_id, sizeof(uuid_t));
			sessionStartResponse.set_error(SessionStartError::SESSION_STARTED);
		} else {
			stringstream logmsg, query;
			DatabaseResults results;
			
			// get client ID from message
			this->set_client_id(sessionStartRequest->client_id());
			
			logmsg << "Got connection from: " << this->m_str_client_id << std::endl;
			Logger::info(logmsg.str());
			
			// create response
			sessionStartResponse.set_client_id(this->m_client_id, sizeof(uuid_t));
			
			// check for duplicate session
			query << "SELECT session_id FROM sessions WHERE client_id = '" << this->m_str_client_id << "';" << std::endl;
			
			this->db.exec_with_results(query.str(), &results);
			if (results.size() > 0) {
				// duplicate client
				uuid_t t_session_id;
				uuid_parse(results.back()["session_id"].c_str(), t_session_id); 
				sessionStartResponse.set_session_id(t_session_id, sizeof(uuid_t));
				sessionStartResponse.set_error(SessionStartError::CLIENT_ALREADY_CONNECTED);
			} else {
				// generate new session id and record
				this->start_session();
				sessionStartResponse.set_session_id(this->m_session_id, sizeof(uuid_t));
				sessionStartResponse.set_error(SessionStartError::OK);
				
				query.str("");
				query << "INSERT INTO sessions VALUES ('" << this->m_str_client_id << "', '" << this->m_str_session_id << "');" << std::endl;
				this->db.exec(query.str());
			}
		}
		
		this->socket_write_message(MSG_ID_SESSIONSTARTRESPONSE, wrapper->request_id, &sessionStartResponse);
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
	
	void ServerProcess::run()
	{
		this->db.open();

		while (this->process_message()) ;
		
		Logger::info("Client disconnected, shutting down");
		
		{
			stringstream query;
			query << "DELETE FROM sessions WHERE client_id = '" << this->m_str_client_id << "';" << std::endl;
			this->db.exec(query.str());
			this->db.close();
		}
	}	
}