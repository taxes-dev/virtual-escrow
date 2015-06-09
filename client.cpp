#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "echo.pb.h"
#include "messageformat.h"
#include "shared.h"

void socket_write_message(const int sockfd, const int message_id, const google::protobuf::MessageLite * message) {
	escrow::MessageWrapper responseFrame;
	if (create_wrapper_from_protobuf(message, message_id, &responseFrame) == false) {
		error("ERROR framing response");
	}
	
	int n = write(sockfd, &responseFrame, MESSAGEWRAPPER_SIZE(responseFrame));
	if (n < 0) {
		error("ERROR writing to socket");
	}
}

void handle_EchoResponse(const int newsockfd, const escrow::EchoResponse * echoResponse) {
	std::stringstream logmsg;
	logmsg << "Got response: " << echoResponse->message() << std::endl;
	info(logmsg.str().c_str());
}
	
int main(int argc, char *argv[]) {
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	
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
	
	std::cout << "Please enter the message: " << std::endl;
	std::string input;
	std::getline(std::cin, input);
	
	escrow::EchoRequest* echoRequest = new escrow::EchoRequest();
	echoRequest->set_message(input);
	
	socket_write_message(sockfd, MSG_ID_ECHOREQUEST, echoRequest);
	
	n = read(sockfd, buffer, BUFFER_SIZE);
	if (n < 0) {
		error("ERROR reading from socket");
	}
	
	message_dispatch(buffer, n, [sockfd](int message_id, google::protobuf::MessageLite * message) {
		switch (message_id) {
			case MSG_ID_ECHORESPONSE:
				handle_EchoResponse(sockfd, (escrow::EchoResponse *)message);
				break;
			default:
				error("ERROR unhandled message");
				break;
		}
	});
	
	close(sockfd);
	delete echoRequest;
	google::protobuf::ShutdownProtobufLibrary();
	return 0;
}
