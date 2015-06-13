#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <uuid/uuid.h>
#include "echo.pb.h"
#include "session.pb.h"
#include "messageformat.h"
#include "shared.h"

uuid_t g_client_id;
bool g_session_set = false;
uuid_t g_session_id;


void cmd_EchoRequest(const int sockfd) {
	std::cout << "Please enter the message: " << std::endl;
	std::string input;
	std::cin.ignore();
	std::getline(std::cin, input);
	
	escrow::EchoRequest * echoRequest = new escrow::EchoRequest();
	echoRequest->set_message(input);
	socket_write_message(sockfd, MSG_ID_ECHOREQUEST, echoRequest);
	delete echoRequest;
}

void cmd_SessionStartRequest(const int sockfd, const uuid_t client_id) {
	escrow::SessionStartRequest * sessionStartRequest = new escrow::SessionStartRequest();
	sessionStartRequest->set_client_id(client_id, sizeof(uuid_t));
	socket_write_message(sockfd, MSG_ID_SESSIONSTARTREQUEST, sessionStartRequest);
	delete sessionStartRequest;
}

void handle_EchoResponse(const escrow::EchoResponse * echoResponse) {
	std::stringstream logmsg;
	logmsg << "Got response: " << echoResponse->message() << std::endl;
	info(logmsg.str().c_str());
}

void handle_SessionStartResponse(const escrow::SessionStartResponse * sessionStartResponse) {
	switch (sessionStartResponse->error()) {
		case escrow::SessionStartError::OK:
			sessionStartResponse->session_id().copy((char *)g_session_id, sizeof(uuid_t));
			g_session_set = true;
			break;
		case escrow::SessionStartError::CLIENT_ALREADY_CONNECTED:
			break;
		default:
			error("ERROR unknown session start error code");
			break;
	}
}

bool process_message(const int sockfd, bool wait) {
	struct pollfd sds;
	char buffer[BUFFER_SIZE];
	int n = 0;
	
	sds.fd = sockfd;
	sds.events = POLLIN | POLLPRI | POLLHUP;
	
	if (poll(&sds, 1, wait ? -1 : 0) == 1) {
		if (sds.revents & POLLHUP) {
			return false;
		} else if (sds.revents & POLLPRI) {
			n = recv(sockfd, buffer, BUFFER_SIZE, MSG_OOB);
		} else {
			n = recv(sockfd, buffer, BUFFER_SIZE, 0);
		}
		
		if (n == 0) {
			return false;
		}
		
		if (n < 0) {
			error("ERROR reading from socket");
		}
		
		if (n > 0) {
			message_dispatch(buffer, n, [sockfd](int message_id, google::protobuf::MessageLite * message) {
				switch (message_id) {
					case MSG_ID_ECHORESPONSE:
						handle_EchoResponse((escrow::EchoResponse *)message);
						break;
					case MSG_ID_SESSIONSTARTRESPONSE:
						handle_SessionStartResponse((escrow::SessionStartResponse *)message);
						break;
					default:
						error("ERROR unhandled message");
						break;
				}
			});
		}
	}
	
	return true;
}

int char_if_ready() {
	struct pollfd fds;
	fds.fd = 0; /* STDIN */
	fds.events = POLLIN;
	if (poll(&fds, 1, 100) == 1) {
		return std::getchar(); // only care about the first character
	}
	return 0;
}

void client_menu(const int sockfd) {
	int c = 0;
	while (c != 'q') {
		c = 0;
		std::cout << "Choose a message to send (select and press ENTER):\n1) echo\nq) quit" << std::endl;
		while (c == 0) {
			c = char_if_ready(); // times out after 100ms
			switch (c) {
				case '1': cmd_EchoRequest(sockfd); break;
			}
			if (process_message(sockfd, false) == false) {
				info("Lost connection, shutting down");
				return;
			}
		}
	}
}
	
int main(int argc, char *argv[]) {
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	std::stringstream logmsg;
	char uuid_buffer[UUID_STR_SIZE];
	
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	
	char buffer[BUFFER_SIZE];
	if (argc < 3) {
		error("usage: ve-client hostname port");
	}
	
	portno = atoi(argv[2]);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		error("ERROR opening socket");
	}
	
	server = gethostbyname(argv[1]);
	if (server == NULL) {
		error("ERROR, no such host");
	}
	
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);
	if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
		error("ERROR connecting");
	}
	
	uuid_generate(g_client_id);
	uuid_unparse(g_client_id, uuid_buffer);
	logmsg << "I am client " << uuid_buffer << std::endl;
	info(logmsg.str().c_str());
	
	cmd_SessionStartRequest(sockfd, g_client_id);
	process_message(sockfd, true); // block until we get a SessionStartResponse
	if (g_session_set) {
		client_menu(sockfd);
	} else {
		info("Client with this ID is already connected, disconnecting");
	}
	shutdown(sockfd, SHUT_RDWR);
	google::protobuf::ShutdownProtobufLibrary();
	info("done");
	return 0;
}
