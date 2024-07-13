
#ifndef PTOC_HPP
#define PTOC_HPP

#include <string>
#include <iostream>

#include <time.h>
#include <math.h>
#include <vector>

struct ptoc_config{
    double pickup;
    double time_dial;
    std::string curve;
    int pickup_flag, trip_flag;
    std::vector<double>* module;

    int stop, running;
    pthread_t thd;
    pthread_cond_t* prot_cond;
    pthread_mutex_t* prot_mutex;
    pthread_cond_t* sniffer_cond;
    pthread_mutex_t* sniffer_mutex;
};

inline void get_inverse_time_curve_coef(std::string curve_name, double &a, double& b, double &c){

    if (curve_name == "U1") {
        a = 0.0226;
        b = 0.0104;
        c = 0.02;
    } else if (curve_name == "U2") {
        a = 0.180;
        b = 5.95;
        c = 2.0;
    } else if (curve_name == "U3") {
        a = 0.0963;
        b = 3.88;
        c = 2.0;
    } else if (curve_name == "U4") {
        a = 0.0352;
        b = 5.67;
        c = 2.0;
    } else if (curve_name == "U5") {
        a = 0.00262;
        b = 0.00342;
        c = 0.02;
    } else if (curve_name == "C1") {
        a = 0.0;
        b = 0.14;
        c = 0.02;
    } else if (curve_name == "C2") {
        a = 0.0;
        b = 13.5;
        c = 1.0;
    } else if (curve_name == "C3") {
        a = 0.0;
        b = 80.0;
        c = 2.0;
    } else if (curve_name == "C4") {
        a = 0.0;
        b = 120.0;
        c = 1.0;
    } else if (curve_name == "C5") {
        a = 0.0;
        b = 0.05;
        c = 0.04;
    } else {
        std::cerr << "Curve name '" << curve_name << "' not found!" << std::endl;
        a = 0.0;
        b = 0.0;
        c = 0.0;
    }
}


#endif // PTOC_HPP


