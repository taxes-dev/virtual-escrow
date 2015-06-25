#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <uuid/uuid.h>
#include "shared.h"
#include "echo.pb.h"
#include "session.pb.h"
#include "trade.pb.h"
#include "shared/messageformat.h"


bool create_protobuf_from_wrapper(const escrow::MessageWrapper * wrapper, google::protobuf::MessageLite * protobuf) {
	return protobuf->ParseFromArray(wrapper->body, wrapper->body_size);
}

bool create_wrapper_from_buffer(const char * buffer, const size_t buffer_size, escrow::MessageWrapper * wrapper) {
	memcpy(wrapper, buffer, buffer_size);
	return true;
}

bool create_wrapper_from_protobuf(const google::protobuf::MessageLite * protobuf, const int message_id, const uuid_t request_id, escrow::MessageWrapper * wrapper) {
	char buffer[MESSAGE_BUFFER_SIZE];
	
	if (protobuf->SerializeToArray(buffer, MESSAGE_BUFFER_SIZE) == false) {
		return false;
	}

	wrapper->message_id = message_id;
	uuid_copy(wrapper->request_id, request_id);
	wrapper->body_size = protobuf->ByteSize();
	memcpy(wrapper->body, buffer, wrapper->body_size);
	
	return true;
}

void error(const char *msg) {
	pid_t pid = getpid();
	std::cerr << "[" << pid << " ERROR] " << msg << std::endl;
	exit(1);
}

void info(const char *msg) {
	pid_t pid = getpid();
	std::cout << "[" << pid << " INFO] " << msg << std::endl;
}


