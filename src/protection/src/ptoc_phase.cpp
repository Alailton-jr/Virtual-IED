
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

    int send_cond;
    double minTime, trip_time;
    struct timespec time2wait;

    while (!conf->stop){
        minTime = -1;
        send_cond = 0;
        pthread_mutex_lock(conf->sniffer_mutex);
        for (idx_current = 0; idx_current<3;idx_current++){
            if((*conf->module)[idx_current] > conf->pickup){
                if (pickup_flag[idx_current] == 0){
                    send_cond = 1;
                }
                pickup_flag[idx_current] = 1;
                clock_gettime(CLOCK_MONOTONIC ,&t_end[idx_current]);
                time[idx_current] = (t_end[idx_current].tv_sec - t_ini[idx_current].tv_sec) + (t_end[idx_current].tv_nsec - t_ini[idx_current].tv_nsec) / 1e9;
                trip_time = td * ( a + ( (b) / ( pow(( (*conf->module)[idx_current] / conf->pickup ), c) - 1  ) ) );
                if(time[idx_current] > trip_time){
                    if (trip_flag[idx_current] == 0){
                        send_cond = 1;
                    }
                    trip_flag[idx_current] = 1;
                }
                time[idx_current] = trip_time - time[idx_current];
                if (minTime == -1 || time[idx_current] < minTime){
                    minTime = time[idx_current];
                    time2wait.tv_sec = t_end[idx_current].tv_sec;
                    time2wait.tv_nsec = t_end[idx_current].tv_nsec;
                }
            }else{
                clock_gettime(CLOCK_MONOTONIC ,&t_ini[idx_current]);
                if (pickup_flag[idx_current] != 0 || trip_flag[idx_current] != 0){
                    send_cond = 1;
                }
                pickup_flag[idx_current] = 0;
                trip_flag[idx_current] = 0;
            }
            conf->trip_flag = trip_flag[0] | trip_flag[1] | trip_flag[2];
            conf->pickup_flag = pickup_flag[0] | pickup_flag[1] | pickup_flag[2];
        }
        if (send_cond == 1){
            pthread_cond_broadcast(conf->prot_cond);
        }
        if (conf->pickup_flag != 0){
            time2wait.tv_sec += (long)minTime;
            time2wait.tv_nsec += modf(minTime, &minTime)*1e9;
            pthread_cond_timedwait(conf->sniffer_cond, conf->sniffer_mutex, &time2wait);
        }else{
            pthread_cond_wait(conf->sniffer_cond, conf->sniffer_mutex);
        }
        pthread_mutex_unlock(conf->sniffer_mutex);
    }
    conf->running = 0;
    return nullptr;
}

