#include "trip_time_opt.hpp"

inline void get_inverse_time_curve_coef(std::string curve_name, double &a, double& b, double &c){

    if (curve_name == "U1") {
        a = 0.0226;
        b = 0.0104;
        c = 0.02;
    } else if (curve_name == "U2") {
        a = 0.180;
        b = 5.95;
        c = 2.0;
    } else if (curve_name == "U3") {
        a = 0.0963;
        b = 3.88;
        c = 2.0;
    } else if (curve_name == "U4") {
        a = 0.0352;
        b = 5.67;
        c = 2.0;
    } else if (curve_name == "U5") {
        a = 0.00262;
        b = 0.00342;
        c = 0.02;
    } else if (curve_name == "C1") {
        a = 0.0;
        b = 0.14;
        c = 0.02;
    } else if (curve_name == "C2") {
        a = 0.0;
        b = 13.5;
        c = 1.0;
    } else if (curve_name == "C3") {
        a = 0.0;
        b = 80.0;
        c = 2.0;
    } else if (curve_name == "C4") {
        a = 0.0;
        b = 120.0;
        c = 1.0;
    } else if (curve_name == "C5") {
        a = 0.0;
        b = 0.05;
        c = 0.04;
    } else {
        a = 0.0;
        b = 0.0;
        c = 0.0;
    }
}

void INVERSE_TIME_trip(trip_time_plan* plan){
    plan->flag_changed = 0;
    plan->minTime = -1;
    double time;
    for (int i = 0; i < plan->trip_flag_size; i++) {
        if ((*plan->pickup_flag)[i]) {
            clock_gettime(CLOCK_MONOTONIC, &plan->t_end[i]);
            plan->time2Wait = plan->t_end[i];
            time = (plan->t_end[i].tv_sec - plan->t_ini[i].tv_sec) +
                   (plan->t_end[i].tv_nsec - plan->t_ini[i].tv_nsec) / 1e9;

            plan->time_delay = plan->time_dial * ( plan->curve_a + ( (plan->curve_b) / ( pow(( (*plan->module)[i] / plan->pickup ), plan->curve_c) - 1  ) ) );

            if (time > plan->time_delay) {
                if (plan->trip_flag[i] == 0) plan->flag_changed = 1;
                plan->trip_flag[i] = 1;
            } else {
                if (plan->trip_flag[i] == 1) plan->flag_changed = 1;
                plan->trip_flag[i] = 0;
            }
            if (plan->minTime == -1) plan->minTime = time;
            else if (plan->minTime > time) plan->minTime = time;
        } else {
            clock_gettime(CLOCK_MONOTONIC, &plan->t_ini[i]);
            if (plan->trip_flag[i] != 0) plan->flag_changed = 1;
            plan->trip_flag[i] = 0;
        }
    }
    plan->minTime = plan->time_delay - plan->minTime;
    plan->time2Wait.tv_sec += static_cast<long>(plan->minTime);
    plan->time2Wait.tv_nsec += modf(plan->minTime, &plan->minTime) * 1e9;

}

void CONST_TIME_trip(trip_time_plan* plan) {
    plan->flag_changed = 0;
    plan->minTime = -1;
    double time;
    for (int i = 0; i < plan->trip_flag_size; i++) {
        if ((*plan->pickup_flag)[i]) {
            clock_gettime(CLOCK_MONOTONIC, &plan->t_end[i]);
            plan->time2Wait = plan->t_end[i];
            time = (plan->t_end[i].tv_sec - plan->t_ini[i].tv_sec) +
                   (plan->t_end[i].tv_nsec - plan->t_ini[i].tv_nsec) / 1e9;
            if (time > plan->time_delay) {
                if (plan->trip_flag[i] == 0) plan->flag_changed = 1;
                plan->trip_flag[i] = 1;
            } else {
                if (plan->trip_flag[i] == 1) plan->flag_changed = 1;
                plan->trip_flag[i] = 0;
            }
            if (plan->minTime == -1) plan->minTime = time;
            else if (plan->minTime > time) plan->minTime = time;
        } else {
            clock_gettime(CLOCK_MONOTONIC, &plan->t_ini[i]);
            if (plan->trip_flag[i] != 0) plan->flag_changed = 1;
            plan->trip_flag[i] = 0;
        }
    }
    plan->minTime = plan->time_delay - plan->minTime;
    plan->time2Wait.tv_sec += static_cast<long>(plan->minTime);
    plan->time2Wait.tv_nsec += modf(plan->minTime, &plan->minTime) * 1e9;
}

trip_time_plan create_trip_time_plan(std::vector<int32_t>* pickup_flag, double time_delay, trip_time_plan::trip_time_types type, int trip_flag_size){

    trip_time_plan plan;
    plan.pickup_flag = pickup_flag;
    plan.time_delay = time_delay;
    plan.trip_flag.reserve(trip_flag_size);
    plan.trip_flag_size = trip_flag_size;

    switch (type)
    {
    case trip_time_plan::CONST_TIME:
        plan._execute = &CONST_TIME_trip;
        break;
    default:
        plan._execute = &CONST_TIME_trip;
        break;
    }
    return plan;
}

trip_time_plan create_trip_time_plan(std::vector<int32_t>* pickup_flag, double time_dial, std::string curve, trip_time_plan::trip_time_types type, int trip_flag_size, std::vector<double>* module, double pickup){

    trip_time_plan plan;
    plan.pickup_flag = pickup_flag;
    plan.time_dial = time_dial;
    plan.curve = curve;
    plan.pickup = pickup;
    plan.module = module;
    get_inverse_time_curve_coef(curve, plan.curve_a, plan.curve_b, plan.curve_c);
    plan.trip_flag.reserve(trip_flag_size);
    plan.trip_flag_size = trip_flag_size;

    switch (type)
    {
    case trip_time_plan::CONST_TIME:
        plan._execute = &CONST_TIME_trip;
        break;
    case trip_time_plan::INVERSE_TIME:
        plan._execute = &INVERSE_TIME_trip;
        break;
    default:
        plan._execute = &CONST_TIME_trip;
        break;
    }
    return plan;


}


 




