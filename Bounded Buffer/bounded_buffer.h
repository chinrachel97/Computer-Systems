//
//  bounded_buffer.hpp
//  
//
//  Created by Joshua Higginbotham on 11/4/15.
//
//

#ifndef bounded_buffer_h
#define bounded_buffer_h

#include <stdio.h>
#include <queue>
#include <string>
#include <pthread.h>
#include "semaphore.h"

class bounded_buffer {
	/* Internal data here */
	pthread_mutex_t mtx;
	semaphore full = semaphore(0);
	semaphore empty = semaphore(0);
	std::queue<std::string> buffer;
	
public:
    bounded_buffer(int _capacity);
	~bounded_buffer();
    void push_back(std::string str);
    std::string retrieve_front();
    int size();
};

#endif /* bounded_buffer_h */