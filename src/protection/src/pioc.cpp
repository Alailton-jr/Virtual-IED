
#include "pioc.hpp"
#include "trip_time_opt.hpp"


struct pioc_pickup_plan {
    void (*_execute)(pioc_pickup_plan* plan);
    void execute() {
        _execute(this);
    }

    double pickup;
    double time_delay;
    std::vector<double>* module;
    std::vector<int32_t> pickup_flag;
    int32_t flag_changed;
};

void PHASE_pickup(pioc_pickup_plan* plan){
    plan->flag_changed = 0;
    for (int i = 0; i < 3; i++) {
        if ((*plan->module)[i] > plan->pickup) {
            if (plan->pickup_flag[i] == 0) plan->flag_changed = 1;
            plan->pickup_flag[i] = 1;
        } else {
            if (plan->pickup_flag[i] == 1) plan->flag_changed = 1;
            plan->pickup_flag[i] = 0;
        }
    }
}

void NEUTRAL_pickup (pioc_pickup_plan* plan){
    plan->flag_changed = 0;
    if ((*plan->module)[3] > plan->pickup) {
        if (plan->pickup_flag[0] == 0) plan->flag_changed = 1;
        plan->pickup_flag[0] = 1;
    } else {
        if (plan->pickup_flag[0] == 1) plan->flag_changed = 1;
        plan->pickup_flag[0] = 0;
    }
}

pioc_pickup_plan create_pioc_pickup_plan(pioc_config* conf){

    pioc_pickup_plan plan;
    plan.module = conf->module;
    plan.pickup = conf->pickup;
    plan.time_delay = conf->time_dial;

    switch (conf->type){
        case conf->Phase:
            plan._execute = &PHASE_pickup;
            plan.pickup_flag.resize(3);
            break;
        case conf->Neutral:
            plan._execute = &NEUTRAL_pickup;
            plan.pickup_flag.resize(1);
            break;
        default:
            plan._execute = &PHASE_pickup;
            plan.pickup_flag.resize(3);
            break;
    }
    return plan;
}

void* pioc_thread(void* arg){
    pioc_config* conf = reinterpret_cast<pioc_config*>(arg);

    conf->running = 1;

    pioc_pickup_plan pickup_plan = create_pioc_pickup_plan(conf);
    trip_time_plan trip_plan = create_trip_time_plan(&pickup_plan.pickup_flag, conf->time_dial, trip_time_plan::CONST_TIME, pickup_plan.pickup_flag.size());

    while (!conf->stop){
        pthread_mutex_lock(conf->sniffer_mutex);

        pickup_plan.execute();
        trip_plan.execute();

        if (conf->type == conf->Phase){
            conf->pickup_flag = pickup_plan.pickup_flag[0] | pickup_plan.pickup_flag[1] | pickup_plan.pickup_flag[2];
            conf->trip_flag = trip_plan.trip_flag[0] | trip_plan.trip_flag[1] | trip_plan.trip_flag[2];
        }else{
            conf->pickup_flag = pickup_plan.pickup_flag[0];
            conf->trip_flag = trip_plan.trip_flag[0];
        }

        if (pickup_plan.flag_changed || trip_plan.flag_changed){
            pthread_cond_broadcast(conf->prot_cond);
        }
        if (conf->pickup_flag != 0 && conf->trip_flag != 1){
            pthread_cond_timedwait(conf->sniffer_cond, conf->sniffer_mutex, &trip_plan.time2Wait);
        }else{
            pthread_cond_wait(conf->sniffer_cond, conf->sniffer_mutex);
        }
        pthread_mutex_unlock(conf->sniffer_mutex);
    }

    conf->running = 0;
    return nullptr;
}




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




