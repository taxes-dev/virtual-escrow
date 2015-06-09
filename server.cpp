#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "sqlite-amalgamation-3081002/sqlite3.h"
#include "shared.h"


void socket_write_message(const int newsockfd, const int message_id, const google::protobuf::MessageLite * message) {
	escrow::MessageWrapper responseFrame;
	if (create_wrapper_from_protobuf(message, message_id, &responseFrame) == false) {
		error("ERROR framing response");
	}
	
	int n = write(newsockfd, &responseFrame, MESSAGEWRAPPER_SIZE(responseFrame));
	if (n < 0) {
		error("ERROR writing to socket");
	}
}

void handle_EchoRequest(const int newsockfd, const escrow::EchoRequest * echoRequest) {
	std::stringstream logmsg;
	
	logmsg << "Here is the message: " << echoRequest->message() << std::endl; 
	info(logmsg.str().c_str());
	
	escrow::EchoResponse * echoResponse = new escrow::EchoResponse();
	echoResponse->set_message(echoRequest->message());

	socket_write_message(newsockfd, MSG_ID_ECHORESPONSE, echoResponse);
	
	delete echoResponse;
}

void process(int newsockfd) {
	char buffer[BUFFER_SIZE];
	int n, rc;
	sqlite3 *db;
	
	n = read(newsockfd, buffer, BUFFER_SIZE);
	if (n < 0) {
		error("ERROR reading from socket");
	}
	
	message_dispatch(buffer, n, [newsockfd](int message_id, google::protobuf::MessageLite * message) {
		switch (message_id) {
			case MSG_ID_ECHOREQUEST:
				handle_EchoRequest(newsockfd, (escrow::EchoRequest *)message);
				break;
			default:
				error("ERROR unhandled message");
				break;
		}
	});
		
	/*rc = sqlite3_open("test.db", &db);
	if (rc) {
		sqlite3_close(db);
		error("ERROR can't open db");
	}
	sqlite3_close(db);*/
	
	close(newsockfd);
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
		info("Waiting for connection");
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd < 0) {
			error("ERROR on accept");
		}
		
		inet_ntop(AF_INET, &(cli_addr.sin_addr), cli_ip, INET_ADDRSTRLEN);
		std::stringstream logmsg;
		logmsg << "Client connected: " << cli_ip << ":" << cli_addr.sin_port << std::endl;
		info(logmsg.str().c_str());
		
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
