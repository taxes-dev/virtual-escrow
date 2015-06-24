#ifndef SHARED_H
#define SHARED_H
#include <functional>
#include <map>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <uuid/uuid.h>
#include "messageformat.h"
#include "echo.pb.h"
#include "session.pb.h"
#include "trade.pb.h"

#define UUID_STR_SIZE 37

bool create_protobuf_from_wrapper(const escrow::MessageWrapper * wrapper, google::protobuf::MessageLite * protobuf);

bool create_wrapper_from_buffer(const char * buffer, const size_t buffer_size, escrow::MessageWrapper * wrapper);

bool create_wrapper_from_protobuf(const google::protobuf::MessageLite * protobuf, const int message_id, const uuid_t request_id, escrow::MessageWrapper * wrapper);

void error(const char *msg);

void info(const char *msg);

void message_dispatch(const char * buffer, const size_t buffer_size, const std::function<void(const escrow::MessageWrapper *, google::protobuf::MessageLite *)> & handler);

void socket_write_message(const int sockfd, const int message_id, const uuid_t request_id, const google::protobuf::MessageLite * message);

namespace escrow {
	using namespace std;
	
	template <typename T>
	using MessageCallback = function<void(const T *, void * data)>;
	using MessageCallbackInt = function<void(const google::protobuf::MessageLite *)>;
	
	template <typename D>
	class BaseProcess {
	public:
		BaseProcess(const int sock_fd) { this->m_sock_fd = sock_fd; };
		bool process_message(bool wait);		
	protected:
		void add_callback(const uuid_t & request_id, const MessageCallbackInt & callback);
		void socket_write_message(const int message_id, const uuid_t request_id, const google::protobuf::MessageLite * message);
	private:
		int m_sock_fd;
		map<string, MessageCallbackInt> m_callbacks;
	
		void invoke_callback(const uuid_t & request_id, const google::protobuf::MessageLite * message);		
	};
}

#include "shared/BaseProcess.tpp"
	
#endif