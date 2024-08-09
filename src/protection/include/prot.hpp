#ifndef PDISC_HPP
#define PDISC_HPP

#include "ptoc.hpp"
#include "pioc.hpp"
#include "pdis.hpp"
#include "general_definition.hpp"

#include <vector>
#include <pthread.h>

void* pioc_phase_thread(void* arg);
void* pioc_neutral_thread(void* arg);
void* ptoc_phase_thread(void* arg);
void* ptoc_neutral_thread(void* arg);


class ProtectionClass{
public:
    std::vector<pioc_config> pioc;
    std::vector<ptoc_config> ptoc;
    std::vector<pdis_config> pdis;

    pthread_cond_t prot_cond;
    pthread_mutex_t prot_mutex;

    pthread_cond_t* sniffer_cond;
    pthread_mutex_t* sniffer_mutex;

    std::vector<double> *phasor_mod;
    std::vector<double> *phasor_ang;

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

        // PIOC
        for (int i=0;i<this->pioc.size();i++){
            this->pioc[i].sniffer_mutex = this->sniffer_mutex;
            this->pioc[i].sniffer_cond = this->sniffer_cond;
            this->pioc[i].prot_cond = &this->prot_cond;
            this->pioc[i].prot_mutex = &this->prot_mutex;
            this->pioc[i].module = this->phasor_mod;
            this->pioc[i].stop = 0;

            pthread_create(&this->pioc[i].thd, NULL, pioc_thread, static_cast<void*>(&this->pioc[i]));
            pthread_setschedparam(this->pioc[i].thd, SCHED_FIFO, &param);
        }

        // PTOC
        for (int i=0;i<this->ptoc.size();i++){
            this->ptoc[i].sniffer_mutex = this->sniffer_mutex;
            this->ptoc[i].sniffer_cond = this->sniffer_cond;
            this->ptoc[i].prot_cond = &this->prot_cond;
            this->ptoc[i].prot_mutex = &this->prot_mutex;
            this->ptoc[i].module = this->phasor_mod;
            this->ptoc[i].stop = 0;

            pthread_create(&this->ptoc[i].thd, NULL, ptoc_thread, static_cast<void*>(&this->ptoc[i]));
            pthread_setschedparam(this->ptoc[i].thd, SCHED_FIFO, &param);
        }

        // PDIS
        for (int i=0;i<this->pdis.size();i++){
            this->pdis[i].sniffer_mutex = this->sniffer_mutex;
            this->pdis[i].sniffer_cond = this->sniffer_cond;
            this->pdis[i].prot_cond = &this->prot_cond;
            this->pdis[i].prot_mutex = &this->prot_mutex;
            this->pdis[i].module = this->phasor_mod;
            this->pdis[i].angle = this->phasor_ang;
            this->pdis[i].stop = 0;

            pthread_create(&this->pdis[i].thd, NULL, pdis_thread, static_cast<void*>(&this->pdis[i]));
            pthread_setschedparam(this->pdis[i].thd, SCHED_FIFO, &param);
        }

    }

    void stopThread(){

        for (int i=0;i<this->pioc.size();i++){
            this->pioc[i].stop = 1;
            while(this->pioc[i].running){
                pthread_cond_broadcast(this->pioc[i].sniffer_cond);
            }
        }

        for (int i=0;i<this->ptoc.size();i++){
            this->ptoc[i].stop = 1;
            while(this->ptoc[i].running){
                pthread_cond_broadcast(this->ptoc[i].sniffer_cond);
            }
        }

        for (int i=0;i<this->pdis.size();i++){
            this->pdis[i].stop = 1;
            while(this->pdis[i].running){
                pthread_cond_broadcast(this->pdis[i].sniffer_cond);
            }
        }

        // Join
        for (int i=0;i<this->pioc.size();i++){
            pthread_join(this->pioc[i].thd, NULL);
        }
        for (int i=0;i<this->ptoc.size();i++){
            pthread_join(this->ptoc[i].thd, NULL);
        }
        for (int i=0;i<this->pdis.size();i++){
            pthread_join(this->pdis[i].thd, NULL);
        }

    }

};


#endif