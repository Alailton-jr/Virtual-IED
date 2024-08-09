#ifndef PDIR_HPP
#define PDIR_HPP

#include <vector>
#include <time.h>
#include <math.h>
#include <pthread.h>

void *pdir_thread(void *arg);

struct pdir_config{

    enum direction_e{
        Forward,
        Reverse
    };
    enum dir_mode_e{
        Quadrant
    };
    enum pioc_type_e {Phase, Neutral};

    double pickup;
    double time_dial;
    int pickup_flag, trip_flag;
    double torque;
    dir_mode_e dir_mode;
    direction_e direction;
    pioc_type_e type;
    std::vector<double>* module;
    std::vector<double>* angle;

    int stop, running;
    pthread_t thd;
    pthread_cond_t* prot_cond;
    pthread_mutex_t* prot_mutex;
    pthread_cond_t* sniffer_cond;
    pthread_mutex_t* sniffer_mutex;
};


#endif // PDIR_HPP


