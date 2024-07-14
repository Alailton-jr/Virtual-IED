
#include "pioc.hpp"

void* pioc_phase_thread(void* arg){

    auto conf = static_cast<pioc_config*> (arg);
    conf->running = 1;

    int pickup_flag[3];
    int trip_flag[3];
    struct timespec t_ini[3], t_end[3], time2wait;
    double time[3], minTime;

    int idx_current;

    for (idx_current = 0; idx_current<3;idx_current++){
        pickup_flag[idx_current] = 0;
        trip_flag[idx_current] = 0;
        clock_gettime(CLOCK_MONOTONIC ,&t_ini[idx_current]);
    }
    int send_cond = 0;
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
                if(time[idx_current] > conf->time_dial){
                    if (trip_flag[idx_current] == 0){
                        send_cond = 1;
                    }
                    trip_flag[idx_current] = 1;
                }
                if (minTime == -1 || time[idx_current] < minTime){
                    minTime = time[idx_current];
                }
            }else{
                clock_gettime(CLOCK_MONOTONIC, &t_ini[idx_current]);
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
            minTime = conf->time_dial - minTime;
            time2wait.tv_sec = t_end[0].tv_sec + (long)minTime;
            time2wait.tv_nsec = t_end[0].tv_nsec + modf(minTime, &minTime)*1e9;
            pthread_cond_timedwait(conf->sniffer_cond, conf->sniffer_mutex, &time2wait);
        }else{
            pthread_cond_wait(conf->sniffer_cond, conf->sniffer_mutex);
        }
        pthread_mutex_unlock(conf->sniffer_mutex);

    }
    conf->running = 0;
    return nullptr;
}

