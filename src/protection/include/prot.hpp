#ifndef PDISC_HPP
#define PDISC_HPP

#include "ptoc.hpp"
#include "pioc.hpp"
#include "pdis.hpp"
#include "ptov.hpp"
#include "ptuv.hpp"
#include "pdir.hpp"
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
    std::vector<ptuv_config> ptuv;
    std::vector<ptov_config> ptov;    
    std::vector<pdir_config> pdir;


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

            std::cout << "Starting PIOC Thread " << i << std::endl;

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

            std::cout << "Starting PTOC Thread " << i << std::endl;

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

            std::cout << "Starting PDIS Thread " << i << std::endl;

            pthread_create(&this->pdis[i].thd, NULL, pdis_thread, static_cast<void*>(&this->pdis[i]));
            pthread_setschedparam(this->pdis[i].thd, SCHED_FIFO, &param);
        }

        // PTUV
        for (int i=0;i<this->ptuv.size();i++){
            this->ptuv[i].sniffer_mutex = this->sniffer_mutex;
            this->ptuv[i].sniffer_cond = this->sniffer_cond;
            this->ptuv[i].prot_cond = &this->prot_cond;
            this->ptuv[i].prot_mutex = &this->prot_mutex;
            this->ptuv[i].module = this->phasor_mod;
            this->ptuv[i].stop = 0;

            std::cout << "Starting PTUV Thread " << i << std::endl;

            pthread_create(&this->ptuv[i].thd, NULL, ptuv_thread, static_cast<void*>(&this->ptuv[i]));
            pthread_setschedparam(this->ptuv[i].thd, SCHED_FIFO, &param);
        }

        // PTOV
        for (int i=0;i<this->ptov.size();i++){
            this->ptov[i].sniffer_mutex = this->sniffer_mutex;
            this->ptov[i].sniffer_cond = this->sniffer_cond;
            this->ptov[i].prot_cond = &this->prot_cond;
            this->ptov[i].prot_mutex = &this->prot_mutex;
            this->ptov[i].module = this->phasor_mod;
            this->ptov[i].stop = 0;

            std::cout << "Starting PTOV Thread " << i << std::endl;

            pthread_create(&this->ptov[i].thd, NULL, ptov_thread, static_cast<void*>(&this->ptov[i]));
            pthread_setschedparam(this->ptov[i].thd, SCHED_FIFO, &param);
        }

        // PDIR
        for (int i=0;i<this->pdir.size();i++){
            this->pdir[i].sniffer_mutex = this->sniffer_mutex;
            this->pdir[i].sniffer_cond = this->sniffer_cond;
            this->pdir[i].prot_cond = &this->prot_cond;
            this->pdir[i].prot_mutex = &this->prot_mutex;
            this->pdir[i].module = this->phasor_mod;
            this->pdir[i].angle = this->phasor_ang;
            this->pdir[i].stop = 0;

            std::cout << "Starting PDIR Thread " << i << std::endl;

            pthread_create(&this->pdir[i].thd, NULL, pdir_thread, static_cast<void*>(&this->pdir[i]));
            pthread_setschedparam(this->pdir[i].thd, SCHED_FIFO, &param);
        }

    }

    void stopThread(){

        // PIOC
        for (int i=0;i<this->pioc.size();i++){
            this->pioc[i].stop = 1;
            while(this->pioc[i].running){
                pthread_cond_broadcast(this->pioc[i].sniffer_cond);
            }
            std::cout << "Stopping PIOC Thread " << i << std::endl;
        }

        //PTOC
        for (int i=0;i<this->ptoc.size();i++){
            this->ptoc[i].stop = 1;
            while(this->ptoc[i].running){
                pthread_cond_broadcast(this->ptoc[i].sniffer_cond);
            }
            std::cout << "Stopping PTOC Thread " << i << std::endl;
        }

        // PDIS
        for (int i=0;i<this->pdis.size();i++){
            this->pdis[i].stop = 1;
            while(this->pdis[i].running){
                pthread_cond_broadcast(this->pdis[i].sniffer_cond);
            }
            std::cout << "Stopping PDIS Thread " << i << std::endl;
        }

        // PTUV
        for (int i=0;i<this->ptuv.size();i++){
            this->ptuv[i].stop = 1;
            while(this->ptuv[i].running){
                pthread_cond_broadcast(this->ptuv[i].sniffer_cond);
            }
            std::cout << "Stopping PTUV Thread " << i << std::endl;
        }

        // PTOV
        for (int i=0;i<this->ptov.size();i++){
            this->ptov[i].stop = 1;
            while(this->ptov[i].running){
                pthread_cond_broadcast(this->ptov[i].sniffer_cond);
            }
            std::cout << "Stopping PTOV Thread " << i << std::endl;
        }

        // PDIR
        for (int i=0;i<this->pdir.size();i++){
            this->pdir[i].stop = 1;
            while(this->pdir[i].running){
                pthread_cond_broadcast(this->pdir[i].sniffer_cond);
            }
            std::cout << "Stopping PDIR Thread " << i << std::endl;
        }

        // Join
        for (int i=0;i<this->pioc.size();i++){ // PIOC
            pthread_join(this->pioc[i].thd, NULL);
        }
        for (int i=0;i<this->ptoc.size();i++){ // PTOC
            pthread_join(this->ptoc[i].thd, NULL);
        }
        for (int i=0;i<this->pdis.size();i++){ // PDIS
            pthread_join(this->pdis[i].thd, NULL);
        }
        for (int i=0;i<this->ptuv.size();i++){ // PTUV
            pthread_join(this->ptuv[i].thd, NULL);
        }
        for (int i=0;i<this->ptov.size();i++){ // PTOV
            pthread_join(this->ptov[i].thd, NULL);
        }
        for (int i=0;i<this->pdir.size();i++){ // PDIR
            pthread_join(this->pdir[i].thd, NULL);
        }

        

    }

};


#endif