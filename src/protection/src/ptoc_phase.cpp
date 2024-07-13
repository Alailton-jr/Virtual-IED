
#include "ptoc.hpp"


void* ptoc_phase_thread(void* arg){

    auto conf = static_cast<ptoc_config*> (arg);
    conf->running = 1;

    int pickup_flag[3];
    int trip_flag[3];
    struct timespec t_ini[3], t_end[3];
    double time[3];

    int idx_current;

    for (idx_current = 0; idx_current<3;idx_current++){
        pickup_flag[idx_current] = 0;
        trip_flag[idx_current] = 0;
        clock_gettime(CLOCK_MONOTONIC ,&t_ini[idx_current]);
    }

    double a, b, c, td = conf->time_dial;
    get_inverse_time_curve_coef(conf->curve, a, b, c);

    while (!conf->stop){

        for (idx_current = 0; idx_current<3;idx_current++){
            if((*conf->module)[idx_current] > conf->pickup){
                pickup_flag[idx_current] = 1;
                clock_gettime(CLOCK_MONOTONIC ,&t_end[idx_current]);
                time[idx_current] = (t_end[idx_current].tv_sec - t_ini[idx_current].tv_sec) + (t_end[idx_current].tv_nsec - t_ini[idx_current].tv_nsec) / 1e9;
                if(time[idx_current] > td * ( a + ( (b) / ( pow(( (*conf->module)[idx_current] / conf->pickup ), c) - 1  ) ) )){
                    trip_flag[idx_current] = 1;
                }
            }else{
                clock_gettime(CLOCK_MONOTONIC ,&t_ini[idx_current]);
                pickup_flag[idx_current] = 0;
                trip_flag[idx_current] = 0;
            }
            conf->trip_flag = trip_flag[0] | trip_flag[1] | trip_flag[2];
            conf->pickup_flag = pickup_flag[0] | pickup_flag[1] | pickup_flag[2];
        }
    }
    return nullptr;
}

