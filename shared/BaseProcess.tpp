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
	void BaseProcess<D>::message_dispatch(const char * buffer, const size_t buffer_size, const MessageHandler & handler) {
		MessageWrapper requestFrame;
		if (create_wrapper_from_buffer(buffer, buffer_size, &requestFrame) == false) {
			error("ERROR copying buffer to request frame");
		}
		
		google::protobuf::MessageLite * message;
		switch (requestFrame.message_id) {
			case MSG_ID_AVAILABLETRADEPARTNERSREQUEST:
				message = new AvailableTradePartnersRequest();
				break;
			case MSG_ID_AVAILABLETRADEPARTNERSRESPONSE:
				message = new AvailableTradePartnersResponse();
				break;
			case MSG_ID_ECHOREQUEST:
				message = new EchoRequest();
				break;
			case MSG_ID_ECHORESPONSE:
				message = new EchoResponse();
				break;
			case MSG_ID_SESSIONSTARTREQUEST:
				message = new SessionStartRequest();
				break;
			case MSG_ID_SESSIONSTARTRESPONSE:
				message = new SessionStartResponse();
				break;
			default:
				error("ERROR unknown message id");
				break;
		}
		
		if (create_protobuf_from_wrapper(&requestFrame, message) == false) {
			error("ERROR parsing protobuf from array");
		}
		handler(&requestFrame, message);
		delete message;
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
				this->message_dispatch(buffer, n, [this](const MessageWrapper * wrapper, google::protobuf::MessageLite * message) {
					D * derived = static_cast<D *>(this);
					switch (wrapper->message_id) {
						case MSG_ID_AVAILABLETRADEPARTNERSREQUEST:
							derived->handle(wrapper, static_cast<AvailableTradePartnersRequest *>(message));
							break;
						case MSG_ID_AVAILABLETRADEPARTNERSRESPONSE:
							derived->handle(wrapper, static_cast<AvailableTradePartnersResponse *>(message));
							break;
						case MSG_ID_ECHOREQUEST:
							derived->handle(wrapper, static_cast<EchoRequest *>(message));
							break;
						case MSG_ID_ECHORESPONSE:
							derived->handle(wrapper, static_cast<EchoResponse *>(message));
							break;
						case MSG_ID_SESSIONSTARTREQUEST:
							derived->handle(wrapper, static_cast<SessionStartRequest *>(message));
							break;
						case MSG_ID_SESSIONSTARTRESPONSE:
							derived->handle(wrapper, static_cast<SessionStartResponse *>(message));
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
		stringstream logmsg;
		uuid_t actual_request_id;
		if (message->IsInitialized() == false) {
			logmsg << "ERROR message not prepared: " << message->InitializationErrorString() << std::endl;
			error(logmsg.str().c_str());
		}
		
		MessageWrapper responseFrame;
		if (request_id == nullptr) {
			uuid_generate(actual_request_id);
		} else {
			uuid_copy(actual_request_id, request_id);
		}
		if (create_wrapper_from_protobuf(message, message_id, actual_request_id, &responseFrame) == false) {
			error("ERROR framing response");
		}
		
		int n = write(this->m_sock_fd, &responseFrame, MESSAGEWRAPPER_SIZE(responseFrame));
		if (n < 0) {
			error("ERROR writing to socket");
		}
	}
}