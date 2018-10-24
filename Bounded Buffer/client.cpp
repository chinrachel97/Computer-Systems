#include <signal.h>
#include <iomanip>
#include <cassert>
#include <cstring>
#include <iostream>
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
#include "reqchannel.h"
#include "BoundedBuffer.h"
#include <math.h>
#include <vector>
#include <map>
using namespace std;

BoundedBuffer * reqs;
BoundedBuffer * resps [3];// 1 bb for each person
int *hists[3];
RequestChannel * control;
int n, b, w;

string names[3]  = {"John Smith", "Jane Smith", "Joe Smith"};

string data_for_names[3]  = {"data John Smith", "data Jane Smith", "data Joe Smith"};
map<string, int> data_to_index;
map<string, int> name_to_index;

void print_time_diff(struct timeval * tp1, struct timeval * tp2, long* sec, long* musec) {
  /* Prints to stdout the difference, in seconds and museconds, between two
     timevals. */
  (*sec) = tp2->tv_sec - tp1->tv_sec;
  (*musec) = tp2->tv_usec - tp1->tv_usec;
  if ((*musec) < 0) {
    (*musec) += 1000000;
    (*sec)--;
  }
  printf(" [sec = %ld, musec = %ld] ", *sec, *musec);
}



// string make_histogram(string name, int *data) {
//     string results = "Frequency count for " + name + ":\n";
//     for(int i = 0; i < 10; ++i) {
//         results += to_string(i * 10) + "-" + to_string((i * 10) + 9) + ": " + to_string(data-[i]) + "\n";
//     }
//     return results;
// }

string make_histogram_table(string *names, int **hists) {
	stringstream tablebuilder;
	tablebuilder << setw(25) << right << names[0];
	tablebuilder << setw(15) << right << names[1];
	tablebuilder << setw(15) << right << names[2] << endl;
    int sums [3] = {0};
	for (int i = 0; i < 10; ++i) {
		tablebuilder << setw(10) << left
		        << string(
		                to_string(i * 10) + "-"
		                        + to_string((i * 10) + 9));
		tablebuilder << setw(15) << right
		        << to_string(hists[0][i]);
		tablebuilder << setw(15) << right
		        << to_string(hists[1][i]);
		tablebuilder << setw(15) << right
		        << to_string(hists[2][i]) << endl;
        
        sums [0] += hists [0][i];
        sums [1] += hists [1][i];
        sums [2] += hists [2][i];
        
	}
	tablebuilder << setw(10) << left << "Total";
	tablebuilder << setw(15) << right << sums [0];
	tablebuilder << setw(15) << right << sums [1];
	tablebuilder << setw(15) << right << sums [2] << endl;

	return tablebuilder.str();
}
void* RT(void* arg) {
    string req = * (string*)arg;
    for(int i = 0; i < n; i++) {
    	reqs->push(req);
	}
	return NULL;
}
//Each worker thread needs a pointer to the request_return buffer and a pointer to the request buffer
//Each worker also needs a pointer to the control channel so that it can start each channel of its own
void* WT(void* arg) {
    RequestChannel* channel = new RequestChannel (control->send_request ("newthread"), RequestChannel::CLIENT_SIDE);
    while(true) {
        string request = reqs->pop();
        string response = channel->send_request(request);
        if (request == "quit")
            break;
        int index = data_to_index[request];
        //cout << "[REQ] " << request << " [ID] " << index << endl;
        resps [index]->push(response);
     }
     delete channel;
}

void* event_handler_thread_function(void* arg) {
    vector<string> state (w);
    int send_counter = 0, recv_counter = 0;
    RequestChannel** rc = new RequestChannel* [w];
    for (int i=0; i< w; i++){
        string name = control->send_request ("newthread");
        rc [i] = new RequestChannel (name, RequestChannel::CLIENT_SIDE);
        state[i] = reqs->pop();
        rc [i]->cwrite (state [i]);
        send_counter ++;
    }
    
    fd_set readset, backup;  // making the fd_set
    int fdmax;
    for (int i=0; i<w; i++)
    {
        fdmax = max (fdmax, rc[i]->read_fd());
        FD_SET (rc [i]->read_fd (), &readset);
    }
    backup = readset; // keep a backup

    while(recv_counter < 3*n) {
        readset = backup; //restore from backup because select() destroys the set
        int k = select (fdmax+1, &readset, 0,0,0);
        for (int i=0; i<w; i++){
            if (FD_ISSET(rc[i]->read_fd(), &readset)){
                string resp = rc[i]->cread();
				string request = state [i];
                recv_counter ++;
                int index = data_to_index[request];
				resps [index]->push(resp);

                if (send_counter < 3*n){
                    string more_req = reqs->pop();
                    state [i] = more_req;
                    rc [i]->cwrite (more_req);
                    send_counter ++;
                }
            }//end if

        }// end for

    }//end while

    for (int i=0; i<w; i++){
        rc [i]->send_request ("quit");
        delete rc [i];
    }
    return NULL;
}



//Each stat thread needs a pointer to the return_request buffer and pointers to the stat vectors
void* ST(void* arg) {
    string name = *(string*)arg;
    //cout << name << endl;
    int personid = data_to_index [name];
    //cout << personid << endl;
    for(int i = 0; i < n; i++){
        int resp = stoi (resps [personid]->pop ()); 
        hists [personid][resp /10] ++; 
    }
}
/*--------------------------------------------------------------------------*/
/* MAIN FUNCTION */
/*--------------------------------------------------------------------------*/
int main(int argc, char * argv[]) {

    n = 10000; //default number of requests per "patient"
    b = 20; //default size of request_buffer
    w = 50; //default number of worker threads
    int opt = 0;
    while ((opt = getopt(argc, argv, "n:b:w:")) != -1) {
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
        }
    }

    int pid = fork();
	if (pid > 0) {
        cout << "n == " << n << endl;
        cout << "b == " << b << endl;
        cout << "w == " << w << endl;

        control = new RequestChannel("control", RequestChannel::CLIENT_SIDE);
        reqs = new BoundedBuffer (b);
        for (int i=0; i<3; i++)
        {
            name_to_index [names [i]] = i;
            data_to_index [data_for_names [i]] = i;
            
            resps [i] = new BoundedBuffer (ceil (b/3.0));
            hists [i] = new int [10];
            memset (hists [i], 0, 10 * sizeof (int));
        }
        struct timeval start, stop;
        long sec, musec;
        gettimeofday(&start, NULL);
        pthread_t request_threads [3];
        pthread_t stat_threads [3];
        pthread_t worker_threads [w];
        for (int i=0; i<3; i++){
            pthread_create(&request_threads [i], NULL, RT, &data_for_names [i]);
            pthread_create(&stat_threads [i], NULL, ST, &data_for_names [i]);

        }
        // for (int i=0; i<w; i++){
            // pthread_create (&worker_threads [i], 0, WT, 0);
        // }
		pthread_t eH;
		pthread_create (&eH, 0, event_handler_thread_function, 0);

        for(int i = 0; i < 3; ++i) {
            pthread_join (request_threads[i], 0);
        }
		pthread_join (eH, 0);
        
        for(int i = 0; i < 3; ++i) {
            pthread_join (stat_threads[i], 0);
        }
        
        cout << "all done." << endl;

        gettimeofday(&stop, NULL);
		cout << "Amout of time taken: ";
		print_time_diff(&start, &stop, &sec, &musec);
		string histogram_table = make_histogram_table(names, hists);
        cout << endl;
        //cout << "Results for n == " << n << ", w == " << w << endl;

		cout << histogram_table << endl;

        string finale = control->send_request("quit");
        cout << "Finale: " << finale << endl;
        delete control;
    }
	else if (pid == 0){
        execl("dataserver", NULL);
    }
}
