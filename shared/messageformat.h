#ifndef H_MESSAGEFORMAT
#define H_MESSAGEFORMAT

#include <functional>
#include <uuid/uuid.h>

#define MESSAGE_BUFFER_SIZE 1024
#define MESSAGE_MAX_BODY_SIZE (MESSAGE_BUFFER_SIZE - sizeof(int) - sizeof(int) - sizeof(uuid_t))

namespace escrow {
	typedef struct s_MessageWrapper {
		int message_id;
		int body_size;
		uuid_t request_id;
		char body[MESSAGE_MAX_BODY_SIZE];
	} MessageWrapper;
}

#define MESSAGEWRAPPER_SIZE(p) (sizeof(int) + sizeof(int) + sizeof(uuid_t) + p.body_size)

// protobuf mappings
#define MSG_ID_ECHOREQUEST 1
#define MSG_ID_ECHORESPONSE 2
#define MSG_ID_SESSIONSTARTREQUEST 3
#define MSG_ID_SESSIONSTARTRESPONSE 4
#define MSG_ID_AVAILABLETRADEPARTNERSREQUEST 5
#define MSG_ID_AVAILABLETRADEPARTNERSRESPONSE 6

#endif