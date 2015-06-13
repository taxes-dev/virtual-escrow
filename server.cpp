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
#include "server-db.h"

bool g_connected = false;
uuid_t g_connected_client_id;
uuid_t g_connected_session_id;

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
	std::stringstream logmsg, query;
	char s_client_id[UUID_STR_SIZE], s_session_id[UUID_STR_SIZE];
	escrow::SessionStartResponse * sessionStartResponse = new escrow::SessionStartResponse();
	
	// message out of order?
	if (g_connected) {
		sessionStartResponse->set_client_id(g_connected_client_id, sizeof(uuid_t));
		sessionStartResponse->set_session_id(g_connected_session_id, sizeof(uuid_t));
		sessionStartResponse->set_error(escrow::SessionStartError::SESSION_STARTED);
	} else {
		// get client ID from message
		sessionStartRequest->client_id().copy((char *)g_connected_client_id, sizeof(uuid_t));
		uuid_unparse(g_connected_client_id, s_client_id);
		logmsg << "Got connection from: " << s_client_id << std::endl;
		info(logmsg.str().c_str());
		
		// create response
		sessionStartResponse->set_client_id(g_connected_client_id, sizeof(uuid_t));
		
		// check for duplicate session
		query << "SELECT session_id FROM sessions WHERE client_id = '" << s_client_id << "';" << std::endl;
		DatabaseResults results;
		exec_database_with_results(db, query.str().c_str(), &results);
		if (results.size() > 0) {
			// duplicate client
			uuid_parse(results.back()["session_id"].c_str(), g_connected_session_id); 
			sessionStartResponse->set_session_id(g_connected_session_id, sizeof(uuid_t));
			sessionStartResponse->set_error(escrow::SessionStartError::CLIENT_ALREADY_CONNECTED);
		} else {
			// generate new session id and record
			uuid_generate(g_connected_session_id);
			g_connected = true;
			uuid_unparse(g_connected_session_id, s_session_id);
			sessionStartResponse->set_session_id(g_connected_session_id, sizeof(uuid_t));
			sessionStartResponse->set_error(escrow::SessionStartError::OK);
		
			query.clear();
			query << "INSERT INTO sessions VALUES ('" << s_client_id << "', '" << s_session_id << "');" << std::endl;
			exec_database(db, query.str().c_str());
		}
	}
	
	socket_write_message(newsockfd, MSG_ID_SESSIONSTARTRESPONSE, sessionStartResponse);
	
	delete sessionStartResponse;
}

void process(int newsockfd) {
	char buffer[BUFFER_SIZE];
	int n;
	sqlite3 * db;
	struct pollfd sds;
	std::stringstream query;
	char s_client_id[UUID_STR_SIZE];
	
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
	uuid_unparse(g_connected_client_id, s_client_id);
	query << "DELETE FROM sessions WHERE client_id = '" << s_client_id << "';" << std::endl;
	exec_database(db, query.str().c_str());
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
