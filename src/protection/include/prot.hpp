#ifndef PDISC_HPP
#define PDISC_HPP

#include "ptoc.hpp"
#include "pioc.hpp"
#include "general_definition.hpp"

#include <vector>
#include <pthread.h>

void* pioc_phase_thread(void* arg);
void* pioc_neutral_thread(void* arg);
void* ptoc_phase_thread(void* arg);
void* ptoc_neutral_thread(void* arg);


class ProtectionClass{
public:
    std::vector<pioc_config> pioc_phase;
    std::vector<pioc_config> pioc_neutral;
    std::vector<ptoc_config> ptoc_phase;
    std::vector<ptoc_config> ptoc_neutral;

    pthread_cond_t prot_cond;
    pthread_mutex_t prot_mutex;

    pthread_cond_t* sniffer_cond;
    pthread_mutex_t* sniffer_mutex;

    std::vector<double> *phasor_mod;
    std::vector<double> *phasor_ang;

    std::thread test;

    int priority;

    ProtectionClass(){
        pthread_mutex_init(&this->prot_mutex, NULL);
        pthread_cond_init(&this->prot_cond, NULL);
    }

    ~ProtectionClass(){
        pthread_mutex_destroy(&this->prot_mutex);
        pthread_cond_destroy(&this->prot_cond);
    }

    void init(){}

    void startThread(){
        this->priority = Protection_ThreadPriority;
        struct sched_param param;
        param.sched_priority = this->priority;

        // PIOC Phase
        for (int i=0;i<this->pioc_phase.size();i++){
            this->pioc_phase[i].sniffer_mutex = this->sniffer_mutex;
            this->pioc_phase[i].sniffer_cond = this->sniffer_cond;
            this->pioc_phase[i].prot_cond = &this->prot_cond;
            this->pioc_phase[i].prot_mutex = &this->prot_mutex;
            this->pioc_phase[i].module = this->phasor_mod;

            pthread_create(&this->pioc_phase[i].thd, NULL, pioc_phase_thread, static_cast<void*>(&this->pioc_phase[i]));
            pthread_setschedparam(this->pioc_phase[i].thd, SCHED_FIFO, &param);
        }

        // PIOC Neutral
        for (int i=0;i<this->pioc_neutral.size();i++){
            this->pioc_neutral[i].sniffer_mutex = this->sniffer_mutex;
            this->pioc_neutral[i].sniffer_cond = this->sniffer_cond;
            this->pioc_neutral[i].prot_cond = &this->prot_cond;
            this->pioc_neutral[i].prot_mutex = &this->prot_mutex;
            this->pioc_neutral[i].module = this->phasor_mod;


            pthread_create(&this->pioc_neutral[i].thd, NULL, pioc_neutral_thread, static_cast<void*>(&this->pioc_neutral[i]));
            pthread_setschedparam(this->pioc_neutral[i].thd, SCHED_FIFO, &param);
        }

        // PTOC Phase
        for (int i=0;i<this->ptoc_phase.size();i++){
            this->ptoc_phase[i].sniffer_mutex = this->sniffer_mutex;
            this->ptoc_phase[i].sniffer_cond = this->sniffer_cond;
            this->ptoc_phase[i].prot_cond = &this->prot_cond;
            this->ptoc_phase[i].prot_mutex = &this->prot_mutex;
            this->ptoc_phase[i].module = this->phasor_mod;

            pthread_create(&this->ptoc_phase[i].thd, NULL, ptoc_phase_thread, static_cast<void*>(&this->ptoc_phase[i]));
            pthread_setschedparam(this->ptoc_phase[i].thd, SCHED_FIFO, &param);
        }

        // PTOC Neutral
        for (int i=0;i<this->ptoc_neutral.size();i++){
            this->ptoc_neutral[i].sniffer_mutex = this->sniffer_mutex;
            this->ptoc_neutral[i].sniffer_cond = this->sniffer_cond;
            this->ptoc_neutral[i].prot_cond = &this->prot_cond;
            this->ptoc_neutral[i].prot_mutex = &this->prot_mutex;
            this->ptoc_neutral[i].module = this->phasor_mod;

            pthread_create(&this->ptoc_neutral[i].thd, NULL, ptoc_neutral_thread, static_cast<void*>(&this->ptoc_neutral[i]));
            pthread_setschedparam(this->ptoc_neutral[i].thd, SCHED_FIFO, &param);
        }
    }

    void stopThread(){

        for (int i=0;i<this->pioc_phase.size();i++){
            this->pioc_phase[i].stop = 1;
            while (this->pioc_phase[i].running){
                pthread_cond_broadcast(this->pioc_phase[i].sniffer_cond);
            }
        }

        for (int i=0;i<this->pioc_neutral.size();i++){
            this->pioc_neutral[i].stop = 1;
            while (this->pioc_neutral[i].running){
                pthread_cond_broadcast(this->pioc_neutral[i].sniffer_cond);
            }
        }

        for (int i=0;i<this->ptoc_phase.size();i++){
            this->ptoc_phase[i].stop = 1;
            while (this->ptoc_phase[i].running){
                pthread_cond_broadcast(this->ptoc_phase[i].sniffer_cond);
            }
        }

        for (int i=0;i<this->ptoc_neutral.size();i++){
            this->ptoc_neutral[i].stop = 1;
            while (this->ptoc_neutral[i].running){
                pthread_cond_broadcast(this->ptoc_neutral[i].sniffer_cond);
            }
        }

        // Join
        for (int i=0;i<this->pioc_phase.size();i++){
            pthread_join(this->pioc_phase[i].thd, NULL);
        }
        for (int i=0;i<this->pioc_neutral.size();i++){
            pthread_join(this->pioc_neutral[i].thd, NULL);
        }
        for (int i=0;i<this->ptoc_phase.size();i++){
            pthread_join(this->ptoc_phase[i].thd, NULL);
        }
        for (int i=0;i<this->ptoc_neutral.size();i++){
            pthread_join(this->ptoc_neutral[i].thd, NULL);
        }

    }

};


#endif