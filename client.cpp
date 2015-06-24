#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
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
#include "client/client-ui.h"


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
	
	std::unique_ptr<escrow::ClientProcess> process(new escrow::ClientProcess(sockfd));
	
	{
		std::stringstream logmsg;
		logmsg << "I am client " << process->client_id_parsed() << std::endl;
		info(logmsg.str().c_str());
	}
	
	if (process->start_session()) {
		escrow::ClientUI ui = escrow::ClientUI(process);
		ui.run();
	} else {
		info("Client with this ID is already connected, disconnecting");
	}

	shutdown(sockfd, SHUT_RDWR);
	close(sockfd);
	google::protobuf::ShutdownProtobufLibrary();
	info("done");
	return 0;
}
