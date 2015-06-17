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
#include "trade.pb.h"
#include "messageformat.h"
#include "shared.h"
#include "virtualitems.h"

uuid_t g_client_id;
bool g_session_set = false;
uuid_t g_session_id;
escrow::Inventory * g_inventory;

void cmd_AvailableTradePartnersRequest(const int sockfd) {
	escrow::AvailableTradePartnersRequest * partnersRequest = new escrow::AvailableTradePartnersRequest();
	socket_write_message(sockfd, MSG_ID_AVAILABLETRADEPARTNERSREQUEST, partnersRequest);
	delete partnersRequest;
}

void cmd_EchoRequest(const int sockfd) {
	std::cout << "Please enter the message: " << std::endl;
	std::string input;
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

void handle_AvailableTradePartnersResponse(const escrow::AvailableTradePartnersResponse * partnersResponse) {
	uuid_t u_client_id;
	char s_client_id[UUID_STR_SIZE];
	google::protobuf::RepeatedPtrField<std::string>::const_iterator iter;
	
	if (partnersResponse->client_id_size() > 0) {
		for (iter = partnersResponse->client_id().begin(); iter < partnersResponse->client_id().end(); ++iter) {
			std::string client_id = *iter;
			std::stringstream clientmsg;
			
			bzero((char *)u_client_id, sizeof(uuid_t));
			bzero((char *)s_client_id, UUID_STR_SIZE);
			client_id.copy((char *)u_client_id, sizeof(uuid_t));
			uuid_unparse(u_client_id, s_client_id);
			
			clientmsg << "Available trade partner: " << s_client_id << std::endl;
			info(clientmsg.str().c_str());
		}
	} else {
		info("No available trade partners");
	}
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
			message_dispatch(buffer, n, [](int message_id, google::protobuf::MessageLite * message) {
				switch (message_id) {
					case MSG_ID_ECHORESPONSE:
						handle_EchoResponse((escrow::EchoResponse *)message);
						break;
					case MSG_ID_SESSIONSTARTRESPONSE:
						handle_SessionStartResponse((escrow::SessionStartResponse *)message);
						break;
					case MSG_ID_AVAILABLETRADEPARTNERSRESPONSE:
						handle_AvailableTradePartnersResponse((escrow::AvailableTradePartnersResponse *)message);
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

void show_inventory() {
	escrow::Inventory::iterator iter;
	int i = 1;
	char s_uuid_tmp[UUID_STR_SIZE];
	uuid_t uuid_tmp, uuid_tmp2;
	
	std::cout << "Current inventory:" << std::endl;
	for (iter = g_inventory->begin(); iter < g_inventory->end(); ++iter, ++i) {
		escrow::VirtualItem * item = *iter;
		
		bzero(s_uuid_tmp, UUID_STR_SIZE);
		bzero(uuid_tmp, sizeof(uuid_t));
		bzero(uuid_tmp2, sizeof(uuid_t));
		
		item->instance_id().copy((char *)uuid_tmp, sizeof(uuid_t));
		item->original_owner_id().copy((char *)uuid_tmp2, sizeof(uuid_t));
		uuid_unparse(uuid_tmp, s_uuid_tmp);
		
		std::cout << i << ") " << item->desc() << " [instance " << s_uuid_tmp << "] [local: " << (
			uuid_compare(g_client_id, uuid_tmp2) == 0 ? "Y" : "N"
		) << "]" << std::endl;
	}
	std::cout << std::endl;
}

int char_if_ready() {
	struct pollfd fds;
	fds.fd = 0; /* STDIN */
	fds.events = POLLIN;
	if (poll(&fds, 1, 100) == 1) {
		int c = std::getchar(); // only care about the first character
		std::cin.ignore(); // discard any remaining chars up to the newline
		return c;
	}
	return 0;
}

void client_menu(const int sockfd) {
	int c = 0;
	while (c != 'q') {
		c = 0;
		std::cout << "Choose a message to send (select and press ENTER):\n1) echo\n2) available trade partners\ni) display inventory\nq) quit" << std::endl;
		while (c == 0) {
			c = char_if_ready(); // times out after 100ms
			switch (c) {
				case '1': cmd_EchoRequest(sockfd); break;
				case '2': cmd_AvailableTradePartnersRequest(sockfd); break;
				case 'i': show_inventory(); break;
			}
			if (process_message(sockfd, false) == false) {
				info("Lost connection, shutting down");
				return;
			}
		}
	}
}
	
int main(int argc, char *argv[]) {
	int sockfd, portno;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	std::stringstream logmsg;
	char uuid_buffer[UUID_STR_SIZE];
	
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	
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
		g_inventory = new escrow::Inventory();
		escrow::generate_random_inventory(g_inventory, 5, g_client_id);
		client_menu(sockfd);
		escrow::free_inventory_items(g_inventory);
		delete g_inventory;
	} else {
		info("Client with this ID is already connected, disconnecting");
	}
	shutdown(sockfd, SHUT_RDWR);
	google::protobuf::ShutdownProtobufLibrary();
	info("done");
	return 0;
}
