#include <pthread.h>
#include "semaphore.h"

semaphore::semaphore(int _val) {
	pthread_mutex_init(&mtx, NULL);
	pthread_cond_init(&condition, NULL);
	counter = _val;
}

semaphore::~semaphore(){
	pthread_mutex_destroy(&mtx);
	pthread_cond_destroy(&condition);
}

/* -- SEMAPHORE OPERATIONS */    
void semaphore::P() {
	pthread_mutex_lock(&mtx);
	--counter;
	if(counter < 0){
		//pthread_mutex_unlock(&mtx);
		pthread_cond_wait(&condition, &mtx);
	}
	//else
		pthread_mutex_unlock(&mtx);
}

void semaphore::V() {
	pthread_mutex_lock(&mtx);
	++counter;
	if(counter <= 0){
		pthread_cond_signal(&condition);
	}
	pthread_mutex_unlock(&mtx);
}