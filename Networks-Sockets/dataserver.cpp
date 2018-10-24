#include <cassert>
#include <cstring>
#include <sstream>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#include "reqchannel.h"
#include "NetworkRequestChannel.h"

pthread_mutex_t channel_mutex;

static int nthreads = 0;

void process_request(NetworkRequestChannel* _channel, const std::string & _request) {

	if (_request.compare(0, 5, "hello") == 0) {
		_channel->cwrite("hello to you too");
	}
	else if (_request.compare(0, 4, "data") == 0) {
		usleep(1000 + (rand() % 5000));
		_channel->cwrite(std::to_string(rand() % 100));
	}
	else if (_request.compare(0, 4, "quit")) {
		std::cout << "k bai\n";
		delete _channel;
	}
	else {
		_channel->cwrite("unknown request");
	}
}

void* handle_process_loop(void * arg) {
	NetworkRequestChannel * _channel = (NetworkRequestChannel *) arg;
	for(;;) {
		std::cout << std::flush;
		std::string request = _channel->cread();
		process_request(_channel, request);
	}
}

int main(int argc, char * argv[]) {
	std::string p = "3000";
	int b = 20;
    int opt = 0;
    while ((opt = getopt(argc, argv, "p:b:")) != -1) {
        switch (opt) {
			case 'p':
				p = optarg;
				break;
			case 'b':
				b = atoi(optarg);
				break;
        }
    }
	NetworkRequestChannel control_channel(p, b, handle_process_loop);
	std::cout << "data server ready\n";
}

