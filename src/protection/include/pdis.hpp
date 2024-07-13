#ifndef PDIS_HPP
#define PDIS_HPP


#include <complex>

struct pdis_config{
    complex<double> z_pickup;
    double time_delay;
    uint8_t stop, running;
    uint8_t pickup_flag, trip_flag;
}





#endif // PDIS_HPP
