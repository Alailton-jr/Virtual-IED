// Protection Over Voltage (PTOV) module

#include "ptov.hpp"
#include "trip_time_opt.hpp"

struct ptov_pickup_plan{
    void (*_execute)(ptov_pickup_plan* plan);
    void execute(){
        _execute(this);
    }

    double pickup;
    double time_delay;
    std::vector<double>* module;
    std::vector<int32_t> pickup_flag;
    int32_t flag_changed;
};

void LINEAR_pickup(ptov_pickup_plan* plan){
    plan->flag_changed = 0;
    for (int i = 0; i < 3; i++) {
        if ((*plan->module)[i+4] > plan->pickup) {
            if (plan->pickup_flag[i] == 0) plan->flag_changed = 1;
            plan->pickup_flag[i] = 1;
        } else {
            if (plan->pickup_flag[i] == 1) plan->flag_changed = 1;
            plan->pickup_flag[i] = 0;
        }
    }
}

ptov_pickup_plan create_ptov_pickup_plan(ptov_config* conf){
    ptov_pickup_plan plan;
    plan.module = conf->module;
    plan.pickup = conf->pickup;
    plan.time_delay = conf->time_dial;

    plan._execute = &LINEAR_pickup;
    plan.pickup_flag.resize(3);
    return plan;
}

void* ptov_thread(void *arg){

    auto conf = reinterpret_cast<ptov_config*>(arg);
    conf->running = 1;

    ptov_pickup_plan pickup_plan = create_ptov_pickup_plan(conf);
    trip_time_plan trip_plan = create_trip_time_plan(
        &pickup_plan.pickup_flag,
        conf->time_dial,
        trip_time_plan::CONST_TIME,
        pickup_plan.pickup_flag.size()
    );

    while (!conf->stop){
        pthread_mutex_lock(conf->sniffer_mutex);

        pickup_plan.execute();
        trip_plan.execute();

        conf->pickup_flag = pickup_plan.pickup_flag[0] | pickup_plan.pickup_flag[1] | pickup_plan.pickup_flag[2];
        conf->trip_flag = trip_plan.trip_flag[0] | trip_plan.trip_flag[1] | trip_plan.trip_flag[2];

        if (pickup_plan.flag_changed || trip_plan.flag_changed){
            pthread_cond_broadcast(conf->prot_cond); // Used by GOOSE Sender
        }

        if(conf->pickup_flag != 0 && conf->trip_flag != 1){
            pthread_cond_timedwait(conf->sniffer_cond, conf->sniffer_mutex, &trip_plan.time2Wait);
        }else{
            pthread_cond_wait(conf->sniffer_cond, conf->sniffer_mutex);
        }

        pthread_mutex_unlock(conf->sniffer_mutex);
    }

    conf->running = 0;
    return nullptr;
}


