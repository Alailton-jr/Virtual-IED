
#ifndef PIOC_HPP
#define PIOC_HPP

#include <string>
#include <iostream>
#include <vector>

#include <time.h>
#include <math.h>

void* pioc_thread(void* arg);

struct pioc_config{
    enum pioc_type_e {Phase, Neutral};

    double pickup;
    double time_dial;
    int pickup_flag, trip_flag;
    pioc_type_e type;
    std::vector<double>* module;

    int stop, running;
    pthread_t thd;
    pthread_cond_t* prot_cond;
    pthread_mutex_t* prot_mutex;
    pthread_cond_t* sniffer_cond;
    pthread_mutex_t* sniffer_mutex;
};


#endif // PIOC_HPP


