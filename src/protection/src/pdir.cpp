// Protection Over Voltage (Pdir) module

#include "pdir.hpp"
#include "trip_time_opt.hpp"

struct pdir_pickup_plan{
        void (*_execute)(pdir_pickup_plan* plan);
        void execute(){
            _execute(this);
        }

    double pickup;
    double time_delay;
    double torque;
    int32_t direction;
    std::vector<double>* module;
    std::vector<double>* angle;
    std::vector<int32_t> pickup_flag;
    int32_t flag_changed;
};

void QUAD_PHASE_pickup(pdir_pickup_plan* plan){
    plan->flag_changed = 0;
    for (int i = 0; i < 3; i++) {

        // 0  1  2  3  4  5  6  7
        // Ia Ib Ic In Va Vb Vc Vn
        // i = 0 (Ia) -> 5 - 6 (Vb - Vc)
        // i = 1 (Ib) -> 6 - 4 (Vc - Va)
        // i = 2 (Ic) -> 4 - 5 (Va - Vb)

        double angRef = 0; 

        if (i == 0) {
            angRef = atan2(
                (*plan->module)[5] * sin((*plan->angle)[5]) - (*plan->module)[6] * sin((*plan->angle)[6]),
                (*plan->module)[5] * cos((*plan->angle)[5]) - (*plan->module)[6] * cos((*plan->angle)[6])
            );
            angRef = plan->torque + angRef;
        } else if (i == 1) {
            angRef = atan2(
                (*plan->module)[6] * sin((*plan->angle)[6]) - (*plan->module)[4] * sin((*plan->angle)[4]),
                (*plan->module)[6] * cos((*plan->angle)[6]) - (*plan->module)[4] * cos((*plan->angle)[4])
            );
            angRef = plan->torque + angRef;
        } else {
            angRef = atan2(
                (*plan->module)[4] * sin((*plan->angle)[4]) - (*plan->module)[5] * sin((*plan->angle)[5]),
                (*plan->module)[4] * cos((*plan->angle)[4]) - (*plan->module)[5] * cos((*plan->angle)[5])
            );
            angRef = plan->torque + angRef;
        }

        int32_t dir_flag;
        if (!plan->direction){
            dir_flag = abs((*plan->angle)[i] - angRef) < M_PI_2;
        }else{
            dir_flag = abs((*plan->angle)[i] - angRef) > M_PI_2;
        }

        if ( ((*plan->module)[i] > plan->pickup) && dir_flag ) {
            if (plan->pickup_flag[i] == 0) plan->flag_changed = 1;
            plan->pickup_flag[i] = 1;
        } else {
            if (plan->pickup_flag[i] == 1) plan->flag_changed = 1;
            plan->pickup_flag[i] = 0;
        }
    }
}

pdir_pickup_plan create_pdir_pickup_plan(pdir_config* conf){
    pdir_pickup_plan plan;
    plan.module = conf->module;
    plan.angle = conf->angle;
    plan.pickup = conf->pickup;
    plan.time_delay = conf->time_dial;
    plan.direction = conf->direction == pdir_config::Forward ? 0 : 1;

    switch (conf->type)
    {
    case pdir_config::Phase:
        switch (conf->dir_mode)
        {
        case pdir_config::Quadrant:
            plan._execute = &QUAD_PHASE_pickup;
            break;
        default:
            plan._execute = &QUAD_PHASE_pickup;
            break;
        }
        break;
    default:
        break;
    }

    plan.pickup_flag.resize(3);
    return plan;
}

void* pdir_thread(void *arg){

    auto conf = reinterpret_cast<pdir_config*>(arg);
    conf->running = 1;

    pdir_pickup_plan pickup_plan = create_pdir_pickup_plan(conf);
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


