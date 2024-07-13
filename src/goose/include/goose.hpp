#ifndef GOOSE_HPP
#define GOOSE_HPP

#include "goose_pkt.hpp"

#include <iostream>
#include <vector>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <linux/if_packet.h> 
#include <linux/net_tstamp.h>
#include <net/if.h>
#include <ifaddrs.h>          
#include <arpa/inet.h>        
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <optional>
#include <unordered_map>
#include <chrono>
#include <time.h>
#include <iomanip>

#include "timers.hpp"
#include "raw_socket.hpp"

struct Goose_Config{
    // Pkt Info
    std::string srcMac;
    std::string dstMac;
    int vlanId = -1;
    int vlanPcp;
    int vlanDei = 0;
    int appId;
    std::string gocbRef;
    std::optional<std::string> goID;
    int minTime, maxTime;
    std::string dataSet;
    int confRev;
    std::vector<void*> data_pointers;
    std::vector<Data> data_type;

    // Thread Info
    pthread_t thd;
    int* stop;
    pthread_cond_t* prot_cond;
    pthread_mutex_t* prot_mutex;

    // Socket info
    RawSocket* raw_socket;
    pthread_mutex_t* mutex;
};

struct sender_config{
    std::vector<uint8_t> pkt;
    int idx_sqNum;
    uint16_t minTime, maxTime;
    RawSocket* raw_socket;

    pthread_t thd;
    int stop;
    pthread_mutex_t* mutex;
};


int main_goose();
void* run_goose_sender(void* arg);

class GooseClass{
public:
    std::vector<Goose_Config> goose_config;
    pthread_cond_t* prot_cond;
    pthread_mutex_t* prot_mutex;

    pthread_t thd;
    int running, stop;

    void init(){}

    void startThread(){
        pthread_create(&this->thd, NULL, run_goose_sender, static_cast<void*>(this));
        return;
    }

    void stopThread(){
        this->stop = 1;
        pthread_mutex_lock(this->prot_mutex);
        pthread_cond_broadcast(this->prot_cond);
        pthread_mutex_unlock(this->prot_mutex);
        pthread_join(this->thd, NULL);
    }
};





#endif // GOOSE_HPP
