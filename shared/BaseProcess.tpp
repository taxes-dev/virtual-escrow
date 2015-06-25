namespace escrow {
	using namespace std;
	
	template<typename D>
	void BaseProcess<D>::add_callback(const uuid_t & request_id, const MessageCallbackInt & callback) {
		string s_request_id = string((char *)request_id, sizeof(uuid_t));
		this->m_callbacks[s_request_id] = callback;
	}
	
	template<typename D>
	void BaseProcess<D>::invoke_callback(const uuid_t & request_id, const google::protobuf::MessageLite * message) {
		string s_request_id = string((char *)request_id, sizeof(uuid_t));
		map<string, MessageCallbackInt>::iterator iter = this->m_callbacks.find(s_request_id);
		
		if (iter != this->m_callbacks.end()) {
			this->m_callbacks[s_request_id](message);
			this->m_callbacks.erase(iter);
		}
	}
	
	template<typename D>
	bool BaseProcess<D>::process_message(int timeout)
	{
		struct pollfd sds;
		char buffer[MESSAGE_BUFFER_SIZE];
		int n = 0;
		
		sds.fd = this->m_sock_fd;
		sds.events = POLLIN | POLLPRI | POLLHUP;
		
		if (poll(&sds, 1, timeout) == 1) {
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
				::message_dispatch(buffer, n, [this](const escrow::MessageWrapper * wrapper, google::protobuf::MessageLite * message) {
					D * derived = static_cast<D *>(this);
					switch (wrapper->message_id) {
						case MSG_ID_AVAILABLETRADEPARTNERSREQUEST:
							derived->handle(wrapper, static_cast<escrow::AvailableTradePartnersRequest *>(message));
							break;
						case MSG_ID_AVAILABLETRADEPARTNERSRESPONSE:
							derived->handle(wrapper, static_cast<escrow::AvailableTradePartnersResponse *>(message));
							break;
						case MSG_ID_ECHOREQUEST:
							derived->handle(wrapper, static_cast<EchoRequest *>(message));
							break;
						case MSG_ID_ECHORESPONSE:
							derived->handle(wrapper, static_cast<escrow::EchoResponse *>(message));
							break;
						case MSG_ID_SESSIONSTARTREQUEST:
							derived->handle(wrapper, static_cast<SessionStartRequest *>(message));
							break;
						case MSG_ID_SESSIONSTARTRESPONSE:
							derived->handle(wrapper, static_cast<escrow::SessionStartResponse *>(message));
							break;
						default:
							error("ERROR unhandled message");
							break;
					}
					
					this->invoke_callback(wrapper->request_id, message);
				});
			}
		}
		
		return true;
		
	}

	template<typename D>
	void BaseProcess<D>::socket_write_message(const int message_id, const uuid_t request_id, const google::protobuf::MessageLite * message) {
		::socket_write_message(this->m_sock_fd, message_id, request_id, message);
	}
}