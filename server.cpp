#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

void error(const char *msg) {
	std::cerr << msg << std::endl;
	exit(1);
}

void process(int newsockfd) {
	char buffer[256];
	int n;
	
	bzero(buffer, 256);
	n = read(newsockfd, buffer, 255);
	if (n < 0) {
		error("ERROR reading from socket");
	}
	
	std::cout << "Here is the message: " << buffer << std::endl;
	n = write(newsockfd, "I got your message", 18);
	if (n < 0) {
		error("ERROR writing to socket");
	}
	
	close(newsockfd);
}

int main(int argc, char **argv) {
	int sockfd, newsockfd, portno;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	int pid;
	
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
	
	listen(sockfd,5);
	clilen = sizeof(cli_addr);
	
	while (1)
	{
		std::cout << "Waiting for connection" << std::endl;
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd < 0) {
			error("ERROR on accept");
		}
		
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
	return 0; 
	
}
