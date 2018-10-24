#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "reqchannel.h"

const bool VERBOSE = false;


// constructor
RequestChannel::RequestChannel(const std::string _name, const Side _side) :
my_name(_name), my_side(_side), side_name((_side == RequestChannel::SERVER_SIDE) ? "SERVER" : "CLIENT"){
	key_t key1 = ftok("a.txt", 100);
	key_t key2 = ftok("b.txt", 100);
	
	// create a message queue
	msqid1 = msgget(key1, 0644| IPC_CREAT);
	msqid2 = msgget(key2, 0644| IPC_CREAT);
	
	// check if successful
	if(msqid1 < 0 || msqid2 < 0){
		perror("Message Queue could not be created");
		return;
	}
	
	//printf("Message Queued ID: %ld\n", msqid1);
	//printf("Message Queued ID: %ld\n", msqid2);
}

// destructor
RequestChannel::~RequestChannel(){
	//std::cout << "k bai\n";
	msgctl(msqid1, IPC_RMID, NULL);
	msgctl(msqid2, IPC_RMID, NULL);
}

/*--------------------------------------------------------------------------*/
/* READ/WRITE FROM/TO REQUEST CHANNELS  */
/*--------------------------------------------------------------------------*/

const int MAX_MESSAGE = 255;

std::string RequestChannel::send_request(std::string _request){
	if(cwrite(_request) < 0) 
		return "ERROR";
	
	std::string s = cread();

	return s;
}

std::string RequestChannel::cread(){
	key_t key1 = ftok("a.txt", 100);
	key_t key2 = ftok("b.txt", 100);
	
	// create a message queue
	msqid1 = msgget(key1, 0644);
	msqid2 = msgget(key2, 0644);
	
	struct my_msgbuf buf;
	key_t key = ftok("a.txt", 100);
	
	// only read client side messages
	if(my_side == RequestChannel::CLIENT_SIDE){
		if(msgrcv(msqid1, &buf, sizeof(buf.mtext), 0, 0) <= 0){
			//perror("msgrcv");
			exit(1);
		}
		//std::cout << "receiving from client: " << buf.mtext << std::endl;
	}
	// only read server side messages
	else if(my_side == RequestChannel::SERVER_SIDE){
		if(msgrcv(msqid2, &buf, sizeof(buf.mtext), 0, 0) <= 0){
			//perror("msgrcv");
			exit(1);
		}
		//std::cout << "receiving from server: " << buf.mtext << std::endl;
	}
	
	std::string s = buf.mtext;
	//std::cout << "side_name: " << side_name << std::endl;
	
	//if(s == "")
	//	s = "0";

	if(buf.mtext == "quit"){
		msgctl(msqid2, IPC_RMID, NULL);
	}
	else if(buf.mtext == "bye"){
		msgctl(msqid1, IPC_RMID, NULL);
	}
	
	return s;
}

int RequestChannel::cwrite(std::string _msg){
	// check if msg fits in the buffer
	if (_msg.length() >= MAX_MESSAGE){
		if(VERBOSE) 
			std::cerr << my_name << ":" << side_name << "Message too long for Channel!" << std::endl;
		return -1;
	}
	
	key_t key1 = ftok("a.txt", 100);
	key_t key2 = ftok("b.txt", 100);
	
	// create a message queue
	msqid1 = msgget(key1, 0644);
	msqid2 = msgget(key2, 0644);
	
	struct my_msgbuf buf;
	
	// change the type based on side_name
	if(my_side == RequestChannel::CLIENT_SIDE)
		buf.mtype = 1;
	else if(my_side == RequestChannel::SERVER_SIDE)
		buf.mtype = 2;
	
	int len = _msg.size();
	
	strcpy(buf.mtext, _msg.c_str());
	
	if(my_side == RequestChannel::CLIENT_SIDE){
		if (msgsnd(msqid2, &buf, len+1, 0) == -1){}
			//perror ("msgsnd");
	}
	else if(my_side == RequestChannel::SERVER_SIDE){
		if (msgsnd(msqid1, &buf, len+1, 0) == -1){}
			//perror ("msgsnd");
	}
	
	//std::cout << "sending: " << buf.mtext << std::endl;
	
	return len;
}

/*--------------------------------------------------------------------------*/
/* ACCESS THE NAME OF REQUEST CHANNEL  */
/*--------------------------------------------------------------------------*/

std::string RequestChannel::name(){
	return my_name;
}
