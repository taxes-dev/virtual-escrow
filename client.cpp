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

void error(const char *msg) {
	std::cout << msg << "\n" << std::endl;
	exit(1);
}

int main(int argc, char *argv[]) {
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	
	char buffer[256];
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
	bzero(buffer, 256);
	std::string input;
	std::getline(std::cin, input);
	std::strcpy(buffer, input.c_str());

	n = write(sockfd,buffer,strlen(buffer));
	if (n < 0) {
		error("ERROR writing to socket");
	}
	
	bzero(buffer, 256);
	n = read(sockfd, buffer, 255);
	if (n < 0) {
		error("ERROR reading from socket");
	}
	
	std::cout << buffer << "\n" << std::endl;
	close(sockfd);
	return 0;
}
