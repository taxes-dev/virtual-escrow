#ifndef _MESSAGEFORMAT_H
#define _MESSAGEFORMAT_H

#include <functional>

#define MESSAGE_MAX_BODY_SIZE (1024 - sizeof(int) - sizeof(int))

namespace escrow {
	typedef struct s_MessageWrapper {
		int message_id;
		int body_size;
		char body[MESSAGE_MAX_BODY_SIZE];
	} MessageWrapper;
}

#define MESSAGEWRAPPER_SIZE(p) (sizeof(int) + sizeof(int) + p.body_size)

// protobuf mappings
#define MSG_ID_ECHOREQUEST 1
#define MSG_ID_ECHORESPONSE 2
#define MSG_ID_SESSIONSTARTREQUEST 3
#define MSG_ID_SESSIONSTARTRESPONSE 4
#define MSG_ID_AVAILABLETRADEPARTNERSREQUEST 5
#define MSG_ID_AVAILABLETRADEPARTNERSRESPONSE 6

#endif