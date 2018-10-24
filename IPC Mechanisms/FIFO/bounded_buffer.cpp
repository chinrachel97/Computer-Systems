//
//  bounded_buffer.cpp
//  
//
//  Created by Joshua Higginbotham on 11/4/15.
//
//

#include "bounded_buffer.h"
#include <string>
#include <queue>
#include "semaphore.h"

bounded_buffer::bounded_buffer(int _capacity) {
	pthread_mutex_init(&mtx, NULL);
	full = semaphore(0);
	empty = semaphore(_capacity);
}

bounded_buffer::~bounded_buffer() {
	pthread_mutex_destroy(&mtx);
}

void bounded_buffer::push_back(std::string req) {
	empty.P();
	pthread_mutex_lock(&mtx);
	buffer.push(req);
	pthread_mutex_unlock(&mtx);
	full.V();
}

std::string bounded_buffer::retrieve_front() {
	full.P();
	pthread_mutex_lock(&mtx);
	std::string str = buffer.front();
	buffer.pop();
	pthread_mutex_unlock(&mtx);
	empty.V();
	return str;
}

int bounded_buffer::size() {
	return buffer.size();
}