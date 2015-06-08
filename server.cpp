#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "sqlite-amalgamation-3081002/sqlite3.h"
#include "protos/echo.pb.h"
#include "messageformat.h"

#define BUFFER_SIZE sizeof(escrow::MessageWrapper)

void error(const char *msg) {
	std::cerr << msg << std::endl;
	exit(1);
}

void process(int newsockfd) {
	char buffer[BUFFER_SIZE];
	int n, rc;
	sqlite3 *db;
	
	bzero(buffer, BUFFER_SIZE);
	n = read(newsockfd, buffer, BUFFER_SIZE);
	if (n < 0) {
		error("ERROR reading from socket");
	}

	escrow::MessageWrapper requestFrame;
	memcpy(&requestFrame, buffer, n);
	escrow::EchoRequest* request = new escrow::EchoRequest();
	if (request->ParseFromArray(requestFrame.body, requestFrame.body_size) == false) {
		error("ERROR parsing request from array");
	}
	
	std::cout << "Here is the message: " << request->message() << std::endl;
	
	escrow::EchoResponse* response = new escrow::EchoResponse();
	response->set_message("I got your message: " + request->message());
	if (response->SerializeToArray(buffer, BUFFER_SIZE) == false) {
		error("ERROR serializing response to array");
	}
	
	escrow::MessageWrapper responseFrame;
	responseFrame.message_id = 0;
	responseFrame.body_size = response->ByteSize();
	memcpy(responseFrame.body, buffer, responseFrame.body_size);
	n = write(newsockfd, &responseFrame, MESSAGEWRAPPER_SIZE(responseFrame));
	if (n < 0) {
		error("ERROR writing to socket");
	}
	
	rc = sqlite3_open("test.db", &db);
	if (rc) {
		sqlite3_close(db);
		error("ERROR can't open db");
	}
	sqlite3_close(db);
	
	close(newsockfd);
	
	delete request, response;
}

int main(int argc, char **argv) {
	int sockfd, newsockfd, portno;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	int pid;
	char cli_ip[INET_ADDRSTRLEN];
	
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	
	if (argc < 2) {
		error("ERROR, no port provided");
	}
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) { 
		error("ERROR opening socket");
	}
	
	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) { 
		error("ERROR on binding");
	}
	
	listen(sockfd, 5);
	clilen = sizeof(cli_addr);
	
	while (1)
	{
		std::cout << "Waiting for connection" << std::endl;
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd < 0) {
			error("ERROR on accept");
		}
		
		inet_ntop(AF_INET, &(cli_addr.sin_addr), cli_ip, INET_ADDRSTRLEN);
		std::cout << "Client connected: " << cli_ip << ":" << cli_addr.sin_port << std::endl;
		
		/* Create child process */
		pid = fork();
		if (pid < 0) {
			error("ERROR on fork");
		}
		
		if (pid == 0) {
			/* This is the client process */
			close(sockfd);
			process(newsockfd);
			exit(0);
		} else {
			close(newsockfd);
		}
	} /* end of while */
		
	close(sockfd);
	google::protobuf::ShutdownProtobufLibrary();
	return 0; 
}
