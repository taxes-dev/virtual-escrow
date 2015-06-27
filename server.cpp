#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <uuid/uuid.h>
#include "shared/shared.h"
#include "server/server-db.h"
#include "server/server-process.h"

using namespace escrow;

int main(int argc, char **argv) {
	int sockfd, newsockfd, portno;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	int pid;
	char cli_ip[INET_ADDRSTRLEN];
	
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	
	if (argc < 2) {
		Logger::fatal("ERROR, no port provided");
	}
	
	ServerDatabase::clean();
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) { 
		Logger::fatal("ERROR opening socket");
	}
	
	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) { 
		Logger::fatal("ERROR on binding");
	}
	
	listen(sockfd, 5);
	clilen = sizeof(cli_addr);
	
	while (1)
	{
		Logger::info("Waiting for connection");
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd < 0) {
			Logger::fatal("ERROR on accept");
		}
		
		inet_ntop(AF_INET, &(cli_addr.sin_addr), cli_ip, INET_ADDRSTRLEN);
		std::stringstream logmsg;
		logmsg << "Client connected: " << cli_ip << ":" << cli_addr.sin_port << std::endl;
		Logger::info(logmsg.str());
		
		/* Create child process */
		pid = fork();
		if (pid < 0) {
			Logger::fatal("ERROR on fork");
		}
		
		if (pid == 0) {
			/* This is the client process */
			close(sockfd);
			ServerProcess serverProcess(newsockfd);
			serverProcess.run(); // blocks until done
			break;
		} else {
			close(newsockfd);
		}
	} /* end of while */

	if (pid == 0) {
		shutdown(sockfd, SHUT_RDWR);
		close(sockfd);
	} else {
		shutdown(newsockfd, SHUT_RDWR);
		close(newsockfd);
	}
	google::protobuf::ShutdownProtobufLibrary();
	return 0; 
}
