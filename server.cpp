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
#include "sqlite-amalgamation-3081002/sqlite3.h"
#include "echo.pb.h"
#include "session.pb.h"
#include "shared.h"

static int db_callback(void * notUsed, int argc, char **argv, char **colName) {
	for (int i = 0; i < argc; i++) {
		std::cout << colName[i] << " = " << (argv[i] ? argv[i] : "NULL") << std::endl;
	}
	return 0;
}

void exec_database(sqlite3 * db, const char * command) {
	char *errmsg = 0;
	int rc = sqlite3_exec(db, command, db_callback, 0, &errmsg);
	if (rc != SQLITE_OK) {
		error(errmsg);
		sqlite3_free(errmsg);
	}
}

void open_database(sqlite3 ** db) {
	std::stringstream query;
	
	// open database
	int rc = sqlite3_open("virtualescrow.db", db);
	if (rc) {
		sqlite3_close(*db);
		error("ERROR can't open db");
	}
	
	// create schema if it doesn't exist
	query << "CREATE TABLE IF NOT EXISTS sessions (client_id TEXT UNIQUE, session_id TEXT);"
		<< std::endl;
	
	exec_database(*db, query.str().c_str());
	std::cout << "DB " << *db << std::endl;
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

void handle_SessionStartRequest(const int newsockfd, sqlite3 * db, const escrow::SessionStartRequest * sessionStartRequest) {
	uuid_t client_id, session_id;
	std::stringstream logmsg, query;
	char s_client_id[37], s_session_id[37];
	
	sessionStartRequest->client_id().copy((char *)client_id, sizeof(uuid_t));
	uuid_unparse(client_id, s_client_id);
	logmsg << "Got connection from: " << s_client_id << std::endl;
	info(logmsg.str().c_str());
	
	escrow::SessionStartResponse * sessionStartResponse = new escrow::SessionStartResponse();
	sessionStartResponse->set_client_id(client_id, sizeof(uuid_t));
	uuid_generate(session_id);
	uuid_unparse(session_id, s_session_id);
	sessionStartResponse->set_session_id(session_id, sizeof(uuid_t));
	sessionStartResponse->set_error(escrow::SessionStartError::OK);
	
	std::cout << "DB " << db << std::endl;
	query << "INSERT INTO sessions VALUES ('" << s_client_id << "', '" << s_session_id << "');" << std::endl;
	info(query.str().c_str());
	exec_database(db, query.str().c_str());
	
	socket_write_message(newsockfd, MSG_ID_SESSIONSTARTRESPONSE, sessionStartResponse);
	
	delete sessionStartResponse;
}

void process(int newsockfd) {
	char buffer[BUFFER_SIZE];
	int n;
	sqlite3 * db;
	struct pollfd sds;
	
	open_database(&db);
	
	while (1) {
		sds.fd = newsockfd;
		sds.events = POLLIN | POLLPRI | POLLHUP;
		sds.revents = 0;
		
		if (poll(&sds, 1, 100) == 1) {
			if (sds.revents & POLLHUP) {
				break;
			} else if (sds.revents & POLLPRI) {
				n = recv(newsockfd, buffer, BUFFER_SIZE, MSG_OOB);
			} else {
				n = recv(newsockfd, buffer, BUFFER_SIZE, 0);
			}

			if (n == 0) {
				break;
			}
			
			if (n < 0) {
				error("ERROR reading from socket");
			}
		
			message_dispatch(buffer, n, [newsockfd, db](int message_id, google::protobuf::MessageLite * message) {
				switch (message_id) {
					case MSG_ID_ECHOREQUEST:
						handle_EchoRequest(newsockfd, (escrow::EchoRequest *)message);
						break;
					case MSG_ID_SESSIONSTARTREQUEST:
						handle_SessionStartRequest(newsockfd, db, (escrow::SessionStartRequest *)message);
						break;
					default:
						error("ERROR unhandled message");
						break;
				}
			});
		}
	}
	
	info("Client disconnected, shutting down");
	sqlite3_close(db);
	
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
			break;
		} else {
			close(newsockfd);
		}
	} /* end of while */

	if (pid == 0) {
		shutdown(sockfd, SHUT_RDWR);
	} else {
		shutdown(newsockfd, SHUT_RDWR);
	}
	google::protobuf::ShutdownProtobufLibrary();
	return 0; 
}
