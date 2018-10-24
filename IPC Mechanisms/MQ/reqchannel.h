#ifndef _reqchannel_H_
#define _reqchannel_H_

#include <iostream>
#include <fstream>
#include <exception>
#include <string>
#include <vector>
#include <queue>

struct my_msgbuf {
	long mtype;
	char mtext[200];// = "";
};

class sync_lib_exception : public std::exception {
	std::string err = "failure in sync library";
	
public:
	sync_lib_exception() {}
	sync_lib_exception(std::string msg) : err(msg) {}
	virtual const char* what() const throw() {
		return err.c_str();
	}
};


class RequestChannel{
public:

	typedef enum {SERVER_SIDE, CLIENT_SIDE} Side;
	typedef enum {READ_MODE, WRITE_MODE} Mode;

	// constructor
	RequestChannel(const std::string _name, const Side _side);
	
	// destructor
	~RequestChannel();
	
	// send a string over the channel and wait for a reply
	std::string send_request(std::string _request);
	
	// - blocking read of data from the channel
	// - returns a string of characters read from the channel
	// - returns NULL if read failed
	std::string cread();
	
	// - write the data to the channel
	// - the function returns the number of characters written to the channel
	int cwrite(std::string msg);
	
	// returns the name of the request channel
	std::string name();

	// returns the file descriptor used to read from the channel
	int read_fd();
	
	// returns the file descriptor used to write to the channel
	int write_fd();
	
private:

	std::string   my_name = "";
	std::string side_name = "";
	Side     my_side;
	std::vector<int>msqids;
	std::queue<int>msgqueue;
	int msqid1;
	int msqid2;
};


#endif


