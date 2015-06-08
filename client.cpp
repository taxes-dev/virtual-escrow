#include <iostream>
#include <string>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "protos/echo.pb.h"
#include "messageformat.h"

#define BUFFER_SIZE sizeof(escrow::MessageWrapper)

void error(const char *msg) {
	std::cout << msg << std::endl;
	exit(1);
}

int main(int argc, char *argv[]) {
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	
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
	bzero(buffer, BUFFER_SIZE);
	std::string input;
	std::getline(std::cin, input);
	
	escrow::EchoRequest* request = new escrow::EchoRequest();
	request->set_message(input);
	if (request->SerializeToArray(buffer, BUFFER_SIZE) == false) {
		error("ERROR serializing request to array");
	}
	escrow::MessageWrapper requestFrame;
	requestFrame.message_id = 0;
	requestFrame.body_size = request->ByteSize();
	memcpy(requestFrame.body, buffer, requestFrame.body_size);
	
	n = write(sockfd, &requestFrame, MESSAGEWRAPPER_SIZE(requestFrame));
	if (n < 0) {
		error("ERROR writing to socket");
	}
	
	bzero(buffer, BUFFER_SIZE);
	n = read(sockfd, buffer, BUFFER_SIZE);
	if (n < 0) {
		error("ERROR reading from socket");
	}
	
	escrow::MessageWrapper responseFrame;
	memcpy(&responseFrame, buffer, n);
	
	escrow::EchoResponse* response = new escrow::EchoResponse();
	if (response->ParseFromArray(responseFrame.body, responseFrame.body_size) == false) {
		error("ERROR parsing response from array");
	}
	
	std::cout << "Got response: " << response->message() << std::endl;
	close(sockfd);
	delete request;
	return 0;
}
