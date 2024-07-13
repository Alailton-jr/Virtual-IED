

#ifndef SNIFFER_HPP
#define SNIFFER_HPP

#include <string>
#include <cstdint>
#include <cstring>

#include "general_definition.hpp"
#include "raw_socket.hpp"
#include "thread_pool.hpp"

#define WINDOW_STEP 0.2

struct sampledValue_info{
    std::string svID;
    std::vector<int> channel_conf;
    uint8_t mac_dst[6];
    uint16_t vLan_priority;
    uint16_t vLan_id;
    uint16_t smpRate;
    uint16_t noChannels;
    uint16_t frequency;
};

void* SnifferThread(void* arg);

class SnifferClass {
public:
    int running, stop;
    std::vector<double> phasor_mod;
    std::vector<double> phasor_ang;
    int noThreads;
    int noTasks;
    int priority;

    pthread_t thd;
    sampledValue_info sv_info;

    pthread_mutex_t sniffer_mutex;
    pthread_cond_t sniffer_cond;

    SnifferClass(){
        pthread_cond_init(&this->sniffer_cond, NULL);
        pthread_mutex_init(&this->sniffer_mutex, NULL);
    }
    ~SnifferClass(){
        pthread_cond_destroy(&this->sniffer_cond);
        pthread_mutex_destroy(&this->sniffer_mutex);
    }

    void init(){
        this->phasor_mod.reserve(sv_info.noChannels);
        this->phasor_ang.reserve(sv_info.noChannels);
    }

    void startThread(){

        this->phasor_ang.reserve(sv_info.noChannels);
        this->phasor_mod.reserve(sv_info.noChannels);

        this->noThreads = Sniffer_NoThreads;
        this->noTasks = Sniffer_NoTasks;
        this->priority = Sniffer_ThreadPriority;

        pthread_create(&this->thd, NULL, SnifferThread, static_cast<void*>(this));
        struct sched_param param;
        param.sched_priority = this->priority;
        pthread_setschedparam(this->thd, SCHED_FIFO, &param);
    }

    void stopThread(){
        this->stop = 1;
        pthread_join(this->thd, NULL);
    }

};


#endif // SNIFFER_HPP