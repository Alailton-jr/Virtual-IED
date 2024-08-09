
#include "pdis.hpp"
#include "trip_time_opt.hpp"
#include <iostream>

struct pdis_pickup_plan {
    void (*_execute)(pdis_pickup_plan* plan);

    void execute() {
        _execute(this);
    }

    double z_med[3];
    double z_pickup;
    int32_t flag_changed;
    std::vector<int32_t> pickup_flag;
    double rTorque;
    std::vector<double>* module;
    std::vector<double>* angle;
};

void OHM_pickup(pdis_pickup_plan* plan) {
    plan->flag_changed = 0;
    for (int i = 0; i < 3; i++) {
        if ((*plan->module)[i] < 1e-6) (*plan->module)[i] = 1e-6;
        plan->z_med[i] = (*plan->module)[i+4] / (*plan->module)[i];
        if (plan->z_med[i] < plan->z_pickup) {
            if (plan->pickup_flag[i] == 0) plan->flag_changed = 1;
            plan->pickup_flag[i] = 1;
        } else {
            if (plan->pickup_flag[i] == 1) plan->flag_changed = 1;
            plan->pickup_flag[i] = 0;
        }
    }
}

void REACT_pickup(pdis_pickup_plan* plan) {
    plan->flag_changed = 0;
    for (int i = 0; i < 3; i++) {
        if ((*plan->module)[i] < 1e-6) (*plan->module)[i] = 1e-6;
        plan->z_med[i] = (*plan->module)[i+4] / (*plan->module)[i];
        if (plan->z_med[i] * sin((*plan->angle)[i+4] - (*plan->angle)[i]) < plan->z_pickup) {
            if (plan->pickup_flag[i] == 0) plan->flag_changed = 1;
            plan->pickup_flag[i] = 1;
        } else {
            if (plan->pickup_flag[i] == 1) plan->flag_changed = 1;
            plan->pickup_flag[i] = 0;
        }
    }
}

void MHO_pickup(pdis_pickup_plan* plan) {
    plan->flag_changed = 0;
    for (int i = 0; i < 3; i++) {
        if ((*plan->module)[i] < 1e-6) (*plan->module)[i] = 1e-6;
        plan->z_med[i] = (*plan->module)[i+4] / (*plan->module)[i];
        if (plan->z_med[i] / cos(plan->rTorque - ((*plan->angle)[i+4] - (*plan->angle)[i])) < plan->z_pickup) {
            if (plan->pickup_flag[i] == 0) plan->flag_changed = 1;
            plan->pickup_flag[i] = 1;
        } else {
            if (plan->pickup_flag[i] == 1) plan->flag_changed = 1;
            plan->pickup_flag[i] = 0;
        }
    }
}

pdis_pickup_plan create_dist_plan(pdis_config* conf){

    pdis_pickup_plan plan;
    plan.module = conf->module;
    plan.angle = conf->angle;
    plan.rTorque = conf->torque;
    plan.z_pickup = conf->pickup;
    plan.pickup_flag.resize(3);
    switch (conf->type){
        case pdis_config::pdis_type_e::Impedance:
            plan._execute = &OHM_pickup;
            break;
        case pdis_config::pdis_type_e::Reactance:
            plan._execute = &REACT_pickup;
            break;
        case pdis_config::pdis_type_e::Admittance:
            plan._execute = &MHO_pickup;
            break;
        default:
            plan._execute = &OHM_pickup;
            break;
    }
    return plan;
}

void* pdis_thread(void* arg){
    pdis_config* conf = reinterpret_cast<pdis_config*>(arg);

    conf->running = 1;

    pdis_pickup_plan pickup_plan = create_dist_plan(conf);
    trip_time_plan trip_plan = create_trip_time_plan(&pickup_plan.pickup_flag, conf->time_dial, trip_time_plan::CONST_TIME, pickup_plan.pickup_flag.size());

    int idx_phase;

    while (!conf->stop){
        pthread_mutex_lock(conf->sniffer_mutex);

        pickup_plan.execute();
        trip_plan.execute();

        conf->pickup_flag = pickup_plan.pickup_flag[0] | pickup_plan.pickup_flag[1] | pickup_plan.pickup_flag[2];
        conf->trip_flag = trip_plan.trip_flag[0] | trip_plan.trip_flag[1] | trip_plan.trip_flag[2];

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
