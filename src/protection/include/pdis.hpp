#ifndef PDIS_HPP
#define PDIS_HPP

#include <complex>
#include <vector>


struct pdis_config{

    enum pdis_type_e{
        Impedance,
        Reactance,
        Admittance,
        Quadrilateral
    };

    double pickup;
    double torque;
    pdis_type_e type;
    double time_dial;

    int pickup_flag, trip_flag;
    std::vector<double>* module;
    std::vector<double>* angle;

    int stop, running;
    pthread_t thd;
    pthread_cond_t* prot_cond;
    pthread_mutex_t* prot_mutex;
    pthread_cond_t* sniffer_cond;
    pthread_mutex_t* sniffer_mutex;
};

void* pdis_thread(void* arg);


#endif // PDIS_HPP
