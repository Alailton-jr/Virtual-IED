#include "ptoc.hpp"

void* ptoc_neutral_thread(void* arg){

    auto conf = static_cast<ptoc_config*> (arg);
    conf->running = 1;

    struct timespec t_ini, t_end;
    double time;

    conf->pickup_flag = 0;
    conf->trip_flag = 0;
    clock_gettime(CLOCK_MONOTONIC ,&t_ini);

    double a, b, c, td = conf->time_dial;
    get_inverse_time_curve_coef(conf->curve, a, b, c);

    while (!conf->stop){
        if((*conf->module)[3] > conf->pickup){
            conf->pickup_flag = 1;
            clock_gettime(CLOCK_MONOTONIC ,&t_end);
            time = (t_end.tv_sec - t_ini.tv_sec) + (t_end.tv_nsec - t_ini.tv_nsec) / 1e9;
            if(time > td * ( a + ( (b) / ( pow(( (*conf->module)[3] / conf->pickup ), c) - 1  ) ) )){
                conf->trip_flag = 1;
            }
        }else{
            clock_gettime(CLOCK_MONOTONIC ,&t_ini);
            conf->pickup_flag = 0;
            conf->trip_flag = 0;
        }
    }
    return nullptr;
}

