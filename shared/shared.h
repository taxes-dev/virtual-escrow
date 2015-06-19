#ifndef _SHARED_H
#define _SHARED_H
#include <functional>
#include "messageformat.h"
#include "echo.pb.h"

#define BUFFER_SIZE sizeof(escrow::MessageWrapper)
#define UUID_STR_SIZE 37

bool create_protobuf_from_wrapper(const escrow::MessageWrapper * wrapper, google::protobuf::MessageLite * protobuf);

bool create_wrapper_from_buffer(const char * buffer, const size_t buffer_size, escrow::MessageWrapper * wrapper);

bool create_wrapper_from_protobuf(const google::protobuf::MessageLite * protobuf, const int message_id, escrow::MessageWrapper * wrapper);

void error(const char *msg);

void info(const char *msg);

void message_dispatch(const char * buffer, const size_t buffer_size, const std::function<void(int, google::protobuf::MessageLite *)> & handler);

void socket_write_message(const int sockfd, const int message_id, const google::protobuf::MessageLite * message);
	
#endif