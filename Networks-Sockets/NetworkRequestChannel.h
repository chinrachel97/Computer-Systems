#ifndef Network_Request_Channel_H
#define Network_Request_Channel_H

#include <iostream>
#include <fstream>
#include <exception>
#include <string>

class NetworkRequestChannel{
private:
	int sockfd;
	
public:
	// creates a CLIENT-SIDE local copy of the channel
	NetworkRequestChannel(std::string host, std::string port);
	
	// creates a SERVER-SIDE local copy of the channel that is accepting connections
	// at the given port number
	NetworkRequestChannel(std::string port, int backlog, void* (*connection_handler) (void*));
	
	// create a channel by setting the socketfd
	NetworkRequestChannel(int sfd);
	
	// destructor
	~NetworkRequestChannel();
	
	// send a string over the channel and wait for a reply
	std::string send_request(std::string request);
	
	// blocking read of data from the channel
	// returns a string of characters read from the channel, and NULL if read failed
	std::string cread();
	
	// write the data to the channel
	// the function returns the number of characters written to the channel
	int cwrite(std::string msg);
	
	// return the socketfd
	int socket_fd();
};

#endif