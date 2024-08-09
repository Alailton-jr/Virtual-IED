#ifndef PTOV_HPP
#define PTOV_HPP

#include <vector>
#include <time.h>
#include <math.h>
#include <pthread.h>

void *ptov_thread(void *arg);

struct ptov_config{
    double pickup;
    double time_dial;
    int pickup_flag, trip_flag;
    std::vector<double>* module;

    int stop, running;
    pthread_t thd;
    pthread_cond_t* prot_cond;
    pthread_mutex_t* prot_mutex;
    pthread_cond_t* sniffer_cond;
    pthread_mutex_t* sniffer_mutex;
};


#endif // PTOV_HPP


