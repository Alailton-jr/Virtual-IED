
#ifndef PTOC_HPP
#define PTOC_HPP

#include <string>
#include <iostream>

#include <time.h>
#include <math.h>
#include <vector>

struct ptoc_config{
    enum ptoc_type_e {Phase, Neutral};

    double pickup;
    double time_dial;
    std::string curve;
    ptoc_type_e type;
    int pickup_flag, trip_flag;
    std::vector<double>* module;

    int stop, running;
    pthread_t thd;
    pthread_cond_t* prot_cond;
    pthread_mutex_t* prot_mutex;
    pthread_cond_t* sniffer_cond;
    pthread_mutex_t* sniffer_mutex;
};

void* ptoc_thread(void* arg);

#endif // PTOC_HPP


