//
//  SafeBuffer.cpp
//
//
//  Created by Joshua Higginbotham on 11/4/15.
//
//

#include "SafeBuffer.h"
#include <string>
#include <queue>

using namespace std;

SafeBuffer::SafeBuffer() {
	pthread_mutex_init(&mtx, NULL);
}

SafeBuffer::~SafeBuffer() {
	pthread_mutex_destroy(&mtx);
}

int SafeBuffer::size() {
	int size = 0;
	pthread_mutex_lock(&mtx);
	size = req_queue.size();
	pthread_mutex_unlock(&mtx);
    return size;
}

void SafeBuffer::push_back(std::string str) {
	pthread_mutex_lock(&mtx);
	req_queue.push(str);
	pthread_mutex_unlock(&mtx);
	
	return;
}

std::string SafeBuffer::retrieve_front() {
	string front = "";
	pthread_mutex_lock(&mtx);
	front = req_queue.front();
	req_queue.pop();
	pthread_mutex_unlock(&mtx);
	
	return front;
}
