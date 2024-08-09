#ifndef TRIP_TIME_OPT_HPP
#define TRIP_TIME_OPT_HPP

#include <time.h>
#include <math.h>
#include <vector>
#include <string>

struct trip_time_plan {
    enum trip_time_types{
        CONST_TIME,
        INVERSE_TIME
    };

    void (*_execute)(trip_time_plan* plan);

    void execute() {
        _execute(this);
    }

    std::vector<int32_t>* pickup_flag;
    struct timespec t_ini[3], t_end[3], time2Wait;
    double minTime;
    double time_delay;
    double time_dial;
    std::string curve;
    double curve_a, curve_b, curve_c;
    std::vector<double>* module;
    double pickup;
    std::vector<int32_t> trip_flag;
    int32_t flag_changed;
    int32_t trip_flag_size;
};

trip_time_plan create_trip_time_plan(std::vector<int32_t>* pickup_flag, double time_delay, trip_time_plan::trip_time_types type, int trip_flag_size);

trip_time_plan create_trip_time_plan(std::vector<int32_t>* pickup_flag, double time_dial, std::string curve, trip_time_plan::trip_time_types type, int trip_flag_size, std::vector<double>* module, double pickup);



#endif // TRIP_TIME_OPT_HPP

 




