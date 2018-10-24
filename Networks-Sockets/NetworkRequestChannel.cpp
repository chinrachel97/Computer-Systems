#include <cassert>
#include <cstring>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>

#include "NetworkRequestChannel.h"

// creates a CLIENT-SIDE local copy of the channel
NetworkRequestChannel::NetworkRequestChannel(std::string host, std::string port){
	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	int status;
	
	if((status = getaddrinfo(host.c_str(), port.c_str(), &hints, &res)) != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		exit(-1);
	}
	
	// make a socket:
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd < 0){
		perror ("Error creating socket\n");	
		exit(-1);
	}
	
	// connect!
	if(connect(sockfd, res->ai_addr, res->ai_addrlen) < 0){
		perror("connect error\n");
		exit(-1);
	}
	std::cout << "Successfully connected to the server " << host << std::endl;
}
	
// creates a SERVER-SIDE local copy of the channel that is accepting connections
// at the given port number
NetworkRequestChannel::NetworkRequestChannel(std::string port, int backlog, void* (*connection_handler)(void*)){
	struct addrinfo hints, *serv;
	struct sockaddr_storage their_addr;	// connector's address information
	socklen_t sin_size;
	char s[INET6_ADDRSTRLEN];
	int rv;
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;	// use my IP
	
	
	if((rv = getaddrinfo(NULL, port.c_str(), &hints, &serv)) != 0){
		perror("getaddrinfo");
		exit(-1);
	}
	if((sockfd = socket(serv->ai_family, serv->ai_socktype, serv->ai_protocol)) == -1){
		perror("server: socket");
		exit(-1);
	}
	
	if (bind(sockfd, serv->ai_addr, serv->ai_addrlen) == -1) {
		close(sockfd);
		perror("server: bind");
		exit(-1);
	}
    freeaddrinfo(serv); // all done with this structure

    if (listen(sockfd, backlog) == -1) {
        perror("listen");
        exit(1);
    }
	
	printf("server: waiting for connection...\n");
	pthread_t tid;
	
	// main accept() loop
	while(1){
		sin_size = sizeof their_addr;
		int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if(new_fd == -1){
			perror("accept");
			continue;
		}
		NetworkRequestChannel* n = new NetworkRequestChannel(new_fd);
		pthread_create(&tid, 0, connection_handler, n);
	}
}

//
NetworkRequestChannel::NetworkRequestChannel(int sfd){
	sockfd = sfd;
}

// destructor
NetworkRequestChannel::~NetworkRequestChannel(){
	
}

// send a string over the channel and wait for a reply
std::string NetworkRequestChannel::send_request(std::string request){
	if(cwrite(request) < 0) {
		return "ERROR";
	}
	std::string s = cread();
	return s;
}

// blocking read of data from the channel
// returns a string of characters read from the channel, and NULL if read failed
std::string NetworkRequestChannel::cread(){
	char buf[1024];
	if(recv(sockfd, buf, sizeof(buf), 0) < 0){
		perror("recv");
		return "";
	}
	std::string msg = buf;
	return msg;
}

// write the data to the channel
// the function returns the number of characters written to the channel
int NetworkRequestChannel::cwrite(std::string msg){
	if(send(sockfd, msg.c_str(), msg.size()+1, 0) == -1){
		perror("send");
		return -1;
	}
	return 0;
}

int NetworkRequestChannel::socket_fd(){
	return sockfd;
}