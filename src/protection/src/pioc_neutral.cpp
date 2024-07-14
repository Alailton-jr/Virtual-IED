
#include "pioc.hpp"


void* pioc_neutral_thread(void* arg){

    auto conf = reinterpret_cast<pioc_config*> (arg);
    conf->running = 1;

    struct timespec t_ini, t_end;
    double time;

    conf->pickup_flag = 0;
    conf->trip_flag = 0;
    clock_gettime(CLOCK_MONOTONIC ,&t_ini);
    int send_cond = 0;
    struct timespec time2wait;

    while (!conf->stop){
        pthread_mutex_lock(conf->sniffer_mutex);
        send_cond = 0;
        if( (*conf->module)[3] > conf->pickup ){
            if (conf->pickup_flag == 0){
                send_cond = 1;
            }
            conf->pickup_flag = 1;
            clock_gettime(CLOCK_MONOTONIC ,&t_end);
            time = (t_end.tv_sec - t_ini.tv_sec) + (t_end.tv_nsec - t_ini.tv_nsec) / 1e9;
            if(time > conf->time_dial){
                if (conf->trip_flag == 0){
                    send_cond = 1;
                }
                conf->trip_flag = 1;
            }
        }else{
            clock_gettime(CLOCK_MONOTONIC ,&t_ini);
            if (conf->pickup_flag != 0 || conf->trip_flag != 0){
                send_cond = 1;
            }
            conf->pickup_flag = 0;
            conf->trip_flag = 0;
        }
        if (send_cond == 1){
            pthread_cond_broadcast(conf->prot_cond);
        }

        if (conf->pickup_flag != 0){
            time = conf->time_dial - time;
            time2wait.tv_sec = t_end.tv_sec + (long)time;
            time2wait.tv_nsec = t_end.tv_nsec + modf(time, &time)*1e9;
            pthread_cond_timedwait(conf->sniffer_cond, conf->sniffer_mutex, &time2wait);
        }else {
            pthread_cond_wait(conf->sniffer_cond, conf->sniffer_mutex);
        }
        pthread_mutex_unlock(conf->sniffer_mutex);
    }
    conf->running = 0;
    return nullptr;
}

