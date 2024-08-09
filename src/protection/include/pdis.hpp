#ifndef PDIS_HPP
#define PDIS_HPP


#include <complex>
#include <vector>

enum pdis_type{
    Impedance,
    Reactance,
    Admittance,
    Quadrilateral
};

struct pdis_config{
    double pickup;
    double torque;
    pdis_type type;
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





#endif // PDIS_HPP
