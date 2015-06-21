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
#include "shared/messageformat.h"
#include "shared/shared.h"
#include "shared/virtualitems.h"
#include "client/client-process.h"

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

void client_menu(escrow::ClientProcess & process) {
	int c = 0;
	while (c != 'q') {
		c = 0;
		std::cout << "Choose a message to send (select and press ENTER):\n1) echo\n2) available trade partners\ni) display inventory\nq) quit" << std::endl;
		while (c == 0) {
			c = char_if_ready(); // times out after 100ms
			switch (c) {
				case '1':
				{
					std::cout << "Please enter the message: " << std::endl;
					std::string input;
					std::getline(std::cin, input);
					
					process.cmd_EchoRequest(input);
					break;
				}
				case '2': process.cmd_AvailableTradePartnersRequest([](const escrow::AvailableTradePartnersResponse * response) {
					uuid_t u_client_id;
					char s_client_id[UUID_STR_SIZE];
					google::protobuf::RepeatedPtrField<std::string>::const_iterator iter;
					
					if (response->client_id_size() > 0) {
						for (iter = response->client_id().begin(); iter < response->client_id().end(); ++iter) {
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
				}); break;
				case 'i': process.show_inventory(); break;
			}
			if (process.process_message(false) == false) {
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
	
	escrow::ClientProcess process = escrow::ClientProcess(sockfd);
	
	{
		std::stringstream logmsg;
		logmsg << "I am client " << process.client_id_parsed() << std::endl;
		info(logmsg.str().c_str());
	}
	
	if (process.start_session()) {
		client_menu(process);
	} else {
		info("Client with this ID is already connected, disconnecting");
	}

	shutdown(sockfd, SHUT_RDWR);
	close(sockfd);
	google::protobuf::ShutdownProtobufLibrary();
	info("done");
	return 0;
}
