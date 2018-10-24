/*
    File: client.cpp

    Author: J. Higginbotham
    Department of Computer Science
    Texas A&M University
    Date  : 2016/05/21

    Based on original code by: Dr. R. Bettati, PhD
    Department of Computer Science
    Texas A&M University
    Date  : 2013/01/31
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */
    /* -- This might be a good place to put the size of
        of the patient response buffers -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*
    No additional includes are required
    to complete the assignment, but you're welcome to use
    any that you think would help.
*/
/*--------------------------------------------------------------------------*/

#include <cassert>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <string>
#include <sstream>
#include <sys/time.h>
#include <assert.h>
#include <fstream>
#include <numeric>
#include <vector>
#include <chrono>
#include <signal.h>
#include <cstdio>
#include <ctime>
#include <chrono>
#include "reqchannel.h"
#include "bounded_buffer.h"

typedef std::chrono::high_resolution_clock Clock;

using namespace std;

/*
    This next file will need to be written from scratch, along with
    semaphore.h and (if you choose) their corresponding .cpp files.
 */

//#include "bounded_buffer.h"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/*
    All *_params structs are optional,
    but they might help.
 */
 
struct thread_args{
	bounded_buffer* buffer_addr;
	string name;
	int NB_requests;
	thread_args(bounded_buffer* ba=NULL, string d1="", int d2=0) : 
		buffer_addr(ba), name(d1), NB_requests(d2) {}
};

struct w_thread_args{
	pthread_mutex_t* mtx;
	RequestChannel* req_channel;
	bounded_buffer* buffer_addr;
	bounded_buffer* buffer_john_addr;
	bounded_buffer* buffer_jane_addr;
	bounded_buffer* buffer_joe_addr;
	w_thread_args(pthread_mutex_t* m=NULL, RequestChannel* rc=NULL, 
		bounded_buffer* ba=NULL, bounded_buffer* b_john=NULL,
		bounded_buffer* b_jane=NULL, bounded_buffer* b_joe=NULL) :
		mtx(m), req_channel(rc), buffer_addr(ba), buffer_john_addr(b_john),
			buffer_jane_addr(b_jane), buffer_joe_addr(b_joe) {}
};

struct s_thread_args{
	int nb_req;
	pthread_mutex_t* mtx;
	bounded_buffer* buffer;
	vector<int>* freq_count;
	bool* finished;
	s_thread_args(int nr=0, pthread_mutex_t* m=NULL, bounded_buffer* b=NULL, vector<int>* fc=NULL, bool* f=NULL) :
		nb_req(nr), mtx(m), buffer(b), freq_count(fc), finished(f) {}
};

/*
    This class can be used to write to standard output
    in a multithreaded environment. It's primary purpose
    is printing debug messages while multiple threads
    are in execution.
 */
class atomic_standard_output {
    pthread_mutex_t console_lock;
public:
    atomic_standard_output() { pthread_mutex_init(&console_lock, NULL); }
    ~atomic_standard_output() { pthread_mutex_destroy(&console_lock); }
    void print(std::string s){
        pthread_mutex_lock(&console_lock);
        std::cout << s << std::endl;
        pthread_mutex_unlock(&console_lock);
    }
};

atomic_standard_output threadsafe_standard_output;

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* HELPER FUNCTIONS */
/*--------------------------------------------------------------------------*/

std::string make_histogram(std::string name, std::vector<int> *data) {
    std::string results = "Frequency count for " + name + ":\n";
	int total = 0;
    for(int i = 0; i < data->size(); ++i) {
        results += std::to_string(i * 10) + "-" + std::to_string((i * 10) + 9) + ": " + std::to_string(data->at(i)) + "\n";
		total += data->at(i);
	}
	results += "Total: " + to_string(total) + "\n";
    return results;
}

std::string make_histogram_table(std::string name1, std::string name2,
        std::string name3, std::vector<int> *data1, std::vector<int> *data2,
        std::vector<int> *data3) {
	std::stringstream tablebuilder;
	tablebuilder << std::setw(25) << std::right << name1;
	tablebuilder << std::setw(15) << std::right << name2;
	tablebuilder << std::setw(15) << std::right << name3 << std::endl;
	for (int i = 0; i < data1->size(); ++i) {
		tablebuilder << std::setw(10) << std::left
		        << std::string(
		                std::to_string(i * 10) + "-"
		                        + std::to_string((i * 10) + 9));
		tablebuilder << std::setw(15) << std::right
		        << std::to_string(data1->at(i));
		tablebuilder << std::setw(15) << std::right
		        << std::to_string(data2->at(i));
		tablebuilder << std::setw(15) << std::right
		        << std::to_string(data3->at(i)) << std::endl;
	}
	tablebuilder << std::setw(10) << std::left << "Total";
	tablebuilder << std::setw(15) << std::right
	        << accumulate(data1->begin(), data1->end(), 0);
	tablebuilder << std::setw(15) << std::right
	        << accumulate(data2->begin(), data2->end(), 0);
	tablebuilder << std::setw(15) << std::right
	        << accumulate(data3->begin(), data3->end(), 0) << std::endl;

	return tablebuilder.str();
}

// signal handler function
void signal_handler(int sig){
	alarm(1);
}

/*
    You'll need to fill these in.
*/
void* request_thread_function(void* arg) {
	thread_args* p = (thread_args*) arg;
	string n = p->name;
	int nb_req = p->NB_requests;
	string to_push = "";
	
	if(n == "John")
		to_push = "data John Smith";
	else if(n == "Jane")
		to_push = "data Jane Smith";
	else if(n == "Joe")
		to_push = "data Joe Smith";
	
	for(int i=0; i<nb_req; ++i)
		p->buffer_addr->push_back(to_push);

	delete p;
}

void* worker_thread_function(void* arg) {
	w_thread_args* p = (w_thread_args*) arg;
	while(true) {
		std::string request = p->buffer_addr->retrieve_front();
		std::string response = p->req_channel->send_request(request);
        if(request == "data John Smith") {
			pthread_mutex_lock(p->mtx);
			p->buffer_john_addr->push_back(response);
			pthread_mutex_unlock(p->mtx);
        }
        else if(request == "data Jane Smith") {
			pthread_mutex_lock(p->mtx);
			p->buffer_jane_addr->push_back(response);
			pthread_mutex_unlock(p->mtx);
        }
        else if(request == "data Joe Smith") {
			pthread_mutex_lock(p->mtx);
			p->buffer_joe_addr->push_back(response);
			pthread_mutex_unlock(p->mtx);
        }
        else if(request == "quit") {
            delete p->req_channel;
			break;
        }
    }
}

void* stat_thread_function(void* arg) {
	s_thread_args* p = (s_thread_args*) arg;
	for(int i=0; i<p->nb_req; ++i){
		string response = p->buffer->retrieve_front();
		pthread_mutex_lock(p->mtx);
		p->freq_count->at(stoi(response) / 10) += 1;
		pthread_mutex_unlock(p->mtx);
	}
	*p->finished = true;
}

/*--------------------------------------------------------------------------*/
/* MAIN FUNCTION */
/*--------------------------------------------------------------------------*/
int main(int argc, char * argv[]) {
    int n = 10; //default number of requests per "patient"
    int b = 50; //default size of request_buffer
    int w = 10; //default number of worker threads
    bool USE_ALTERNATE_FILE_OUTPUT = false;
    int opt = 0;
    while ((opt = getopt(argc, argv, "n:b:w:m:h")) != -1) {
        switch (opt) {
            case 'n':
                n = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 'w':
                w = atoi(optarg);
                break;
            case 'm':
                if(atoi(optarg) == 2) USE_ALTERNATE_FILE_OUTPUT = true;
                break;
            case 'h':
            default:
                std::cout << "This program can be invoked with the following flags:" << std::endl;
                std::cout << "-n [int]: number of requests per patient" << std::endl;
                std::cout << "-b [int]: size of request buffer" << std::endl;
                std::cout << "-w [int]: number of worker threads" << std::endl;
                std::cout << "-m 2: use output2.txt instead of output.txt for all file output" << std::endl;
                std::cout << "-h: print this message and quit" << std::endl;
                std::cout << "Example: ./client_solution -n 10000 -b 50 -w 120 -m 2" << std::endl;
                std::cout << "If a given flag is not used, a default value will be given" << std::endl;
                std::cout << "to its corresponding variable. If an illegal option is detected," << std::endl;
                std::cout << "behavior is the same as using the -h flag." << std::endl;
                exit(0);
        }
    }

    int pid = fork();
	if (pid > 0) {
        struct timeval start_time;
        struct timeval finish_time;
        int64_t start_usecs;
        int64_t finish_usecs;
        std::ofstream ofs;
        if(USE_ALTERNATE_FILE_OUTPUT) ofs.open("output2.txt", std::ios::out | std::ios::app);
        else ofs.open("output.txt", std::ios::out | std::ios::app);

        std::cout << "n == " << n << std::endl;
        std::cout << "b == " << b << std::endl;
        std::cout << "w == " << w << std::endl;

        std::cout << "CLIENT STARTED:" << std::endl;
        std::cout << "Establishing control channel... " << std::flush;
        RequestChannel *chan = new RequestChannel("control", RequestChannel::CLIENT_SIDE);
        std::cout << "done." << std::endl;

        /*
            This time you're up a creek.
            What goes in this section of the code?
            Hint: it looks a bit like what went here
            in PA3, but only a *little* bit.
        */
		std::vector<int> john_frequency_count(10, 0);
		std::vector<int> jane_frequency_count(10, 0);
		std::vector<int> joe_frequency_count(10, 0);
		
		/* CHANGES START */
		bounded_buffer b_buffer = bounded_buffer(b);
		bounded_buffer buffer_john = bounded_buffer(50000);
		bounded_buffer buffer_jane = bounded_buffer(50000);
		bounded_buffer buffer_joe = bounded_buffer(50000);
		
		/* REQUEST HERE */
		pthread_t thread1;
		pthread_t thread2;
		pthread_t thread3;
		thread_args* arg1 = new thread_args(&b_buffer, "John", n);
		thread_args* arg2 = new thread_args(&b_buffer, "Jane", n);
		thread_args* arg3 = new thread_args(&b_buffer, "Joe", n);
		pthread_create(&thread1, NULL, request_thread_function, arg1);
        pthread_create(&thread2, NULL, request_thread_function, arg2);
		pthread_create(&thread3, NULL, request_thread_function, arg3);
		
		std::cout << "created 3 request threads" << endl;

		vector<RequestChannel*> vec_req;
		
		/* MAKE WORKER THREADS HERE */
		pthread_t worker_threads[w];
		
		for(int i=0; i<w; ++i){
			std::string s = chan->send_request("newthread");
			RequestChannel *workerChannel = new RequestChannel(s, RequestChannel::CLIENT_SIDE);
			vec_req.push_back(workerChannel);
		}
		
		pthread_mutex_t m;
		pthread_mutex_init(&m, NULL);
		
		for(int i=0; i<w; ++i){
			w_thread_args* w_arg = new w_thread_args(&m, vec_req[i], &b_buffer, &buffer_john,
				&buffer_jane, &buffer_joe);
			pthread_create(&worker_threads[i], NULL, worker_thread_function, w_arg);
		}
		
		cout << "Finished creating threads.\n";
		
		/* MAKE STAT THREADS HERE */
		pthread_mutex_t m2;
		pthread_mutex_init(&m2, NULL);
		bool finished = false;
		pthread_t s_thread1;
		pthread_t s_thread2;
		pthread_t s_thread3;
		s_thread_args* s_arg1 = new s_thread_args(n, &m2, &buffer_john, &john_frequency_count, &finished);
		s_thread_args* s_arg2 = new s_thread_args(n, &m2, &buffer_jane, &jane_frequency_count, &finished);
		s_thread_args* s_arg3 = new s_thread_args(n, &m2, &buffer_joe, &joe_frequency_count, &finished);
		pthread_create(&s_thread1, NULL, stat_thread_function, s_arg1);
		pthread_create(&s_thread2, NULL, stat_thread_function, s_arg2);
		pthread_create(&s_thread3, NULL, stat_thread_function, s_arg3);
		
		/* SET UP ALARM HERE */
		auto t1 = Clock::now();
		signal(SIGALRM, signal_handler);
		alarm(1);
		while(!finished){
			system("clear");
			std::string histogram_table = make_histogram_table("John Smith", "Jane Smith", "Joe Smith", &john_frequency_count, &jane_frequency_count, &joe_frequency_count);
			cout << histogram_table << endl;
			if(finished)	break;
			sleep(1);
		}
		system("clear");
		std::string histogram_table = make_histogram_table("John Smith", "Jane Smith", "Joe Smith", &john_frequency_count, &jane_frequency_count, &joe_frequency_count);
		cout << histogram_table << endl;
		auto t2 = Clock::now();
		std::cout << "Delta t2-t1: " 
				  << std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count()/1000000000.000
				  << " seconds" << std::endl;
		
		
		/* START TIMER HERE */
		//auto t1 = Clock::now();
		
		/* THREAD JOINS */
		pthread_join(thread1, NULL);
		pthread_join(thread2, NULL);
		pthread_join(thread3, NULL);
		
        fflush(NULL);
        for(int i = 0; i < w; ++i)
            b_buffer.push_back("quit");
		
		for(int i=0; i<w; ++i)
			pthread_join(worker_threads[i], NULL);
		
		pthread_join(s_thread1, NULL);
		pthread_join(s_thread2, NULL);
		pthread_join(s_thread3, NULL);
		
		/* END TIMER HERE */
		
		
		//std::string histogram_table = make_histogram_table("John Smith", "Jane Smith", "Joe Smith", &john_frequency_count, &jane_frequency_count, &joe_frequency_count);
		//cout << histogram_table << endl;
		//cout << "Time taken: " << duration << endl;
		pthread_mutex_destroy(&m);
		pthread_mutex_destroy(&m2);
		
		/* CHANGES END */

        ofs.close();
        usleep(10000);
        std::string finale = chan->send_request("quit");
    }
	else if (pid == 0)
		execl("dataserver", NULL);
}
