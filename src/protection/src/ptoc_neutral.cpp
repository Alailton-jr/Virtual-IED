#include "ptoc.hpp"

void* ptoc_neutral_thread(void* arg){

    auto conf = static_cast<ptoc_config*> (arg);
    conf->running = 1;

    struct timespec t_ini, t_end;
    double time;

    conf->pickup_flag = 0;
    conf->trip_flag = 0;
    clock_gettime(CLOCK_MONOTONIC ,&t_ini);

    double a, b, c, td = conf->time_dial, trip_time;
    get_inverse_time_curve_coef(conf->curve, a, b, c);
    int send_cond = 0;
    struct timespec time2wait;
    while (!conf->stop){
        if((*conf->module)[3] > conf->pickup){
            if (conf->pickup_flag == 0){
                send_cond = 1;
            }
            conf->pickup_flag = 1;
            clock_gettime(CLOCK_MONOTONIC, &t_end);
            time = (t_end.tv_sec - t_ini.tv_sec) + (t_end.tv_nsec - t_ini.tv_nsec) / 1e9;
            trip_time = td * ( a + ( (b) / ( pow(( (*conf->module)[3] / conf->pickup ), c) - 1  ) ) );
            if(time > trip_time){
                if (conf->trip_flag == 0){
                    send_cond = 1;
                }
                conf->trip_flag = 1;
            }
        }else{
            clock_gettime(CLOCK_MONOTONIC, &t_ini);
            if (conf->pickup_flag != 0 || conf->trip_flag != 0){
                send_cond = 1;
            }
            conf->pickup_flag = 0;
            conf->trip_flag = 0;
        }
        // if (send_cond == 1){
        //     pthread_cond_broadcast(conf->prot_cond);
        // }

        if (conf->pickup_flag != 0){
            time = trip_time - time;
            time2wait.tv_sec = t_end.tv_sec + (long)time;
            time2wait.tv_nsec = t_end.tv_nsec + modf(time, &time)*1e9;
            pthread_cond_timedwait(conf->sniffer_cond, conf->sniffer_mutex, &time2wait);
        }else {
            pthread_cond_wait(conf->sniffer_cond, conf->sniffer_mutex);
        }
    }
    conf->running = 0;
    return nullptr;
}

