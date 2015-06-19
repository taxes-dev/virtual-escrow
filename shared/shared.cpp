#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <unistd.h>
#include "shared.h"
#include "echo.pb.h"
#include "session.pb.h"
#include "trade.pb.h"


bool create_protobuf_from_wrapper(const escrow::MessageWrapper * wrapper, google::protobuf::MessageLite * protobuf) {
	return protobuf->ParseFromArray(wrapper->body, wrapper->body_size);
}

bool create_wrapper_from_buffer(const char * buffer, const size_t buffer_size, escrow::MessageWrapper * wrapper) {
	memcpy(wrapper, buffer, buffer_size);
	return true;
}

bool create_wrapper_from_protobuf(const google::protobuf::MessageLite * protobuf, const int message_id, escrow::MessageWrapper * wrapper) {
	char buffer[BUFFER_SIZE];
	
	if (protobuf->SerializeToArray(buffer, BUFFER_SIZE) == false) {
		return false;
	}

	wrapper->message_id = message_id;
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

void message_dispatch(const char * buffer, const size_t buffer_size, const std::function<void(int, google::protobuf::MessageLite *)> & handler) {
	escrow::MessageWrapper requestFrame;
	if (create_wrapper_from_buffer(buffer, buffer_size, &requestFrame) == false) {
		error("ERROR copying buffer to request frame");
	}
	
	google::protobuf::MessageLite * message;
	switch (requestFrame.message_id) {
		case MSG_ID_AVAILABLETRADEPARTNERSREQUEST:
			message = new escrow::AvailableTradePartnersRequest();
			break;
		case MSG_ID_AVAILABLETRADEPARTNERSRESPONSE:
			message = new escrow::AvailableTradePartnersResponse();
			break;
		case MSG_ID_ECHOREQUEST:
			message = new escrow::EchoRequest();
			break;
		case MSG_ID_ECHORESPONSE:
			message = new escrow::EchoResponse();
			break;
		case MSG_ID_SESSIONSTARTREQUEST:
			message = new escrow::SessionStartRequest();
			break;
		case MSG_ID_SESSIONSTARTRESPONSE:
			message = new escrow::SessionStartResponse();
			break;
		default:
			error("ERROR unknown message id");
			break;
	}

	if (create_protobuf_from_wrapper(&requestFrame, message) == false) {
		error("ERROR parsing protobuf from array");
	}
	handler(requestFrame.message_id, message);
	delete message;
}

void socket_write_message(const int sockfd, const int message_id, const google::protobuf::MessageLite * message) {
	std::stringstream logmsg;
	if (message->IsInitialized() == false) {
		logmsg << "ERROR message not prepared: " << message->InitializationErrorString() << std::endl;
		error(logmsg.str().c_str());
	}
	
	escrow::MessageWrapper responseFrame;
	if (create_wrapper_from_protobuf(message, message_id, &responseFrame) == false) {
		error("ERROR framing response");
	}
	
	int n = write(sockfd, &responseFrame, MESSAGEWRAPPER_SIZE(responseFrame));
	if (n < 0) {
		error("ERROR writing to socket");
	}
}
