#ifndef MAIN_HPP
#define MAIN_HPP

#include <chrono>
#include <iostream>
#include <thread>
#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

#include "prot.hpp"
#include "thread_pool.hpp"
#include "sniffer.hpp"
#include "prot.hpp"
#include "goose.hpp"


class IED_class{
public:
    SnifferClass sniffer;
    ProtectionClass prot;
    GooseClass goose;

    IED_class(){
        this->goose.prot_mutex = &this->prot.prot_mutex;
        this->goose.prot_cond = &this->prot.prot_cond;

        this->prot.sniffer_cond = &this->sniffer.sniffer_cond;
        this->prot.sniffer_mutex = &this->sniffer.sniffer_mutex;
        
        this->prot.phasor_mod = &this->sniffer.phasor_mod;
        this->prot.phasor_ang = &this->sniffer.phasor_ang;
    }

    void init(){
        this->sniffer.init();
        this->prot.init();
        this->goose.init();
    }

    void runIED(){
        this->sniffer.startThread();
        this->prot.startThread();
        this->goose.startThread();
    }

    void stopIED(){
        this->goose.stopThread();
        this->prot.stopThread();
        this->sniffer.stopThread();
    }

};





#endif // MAIN_HPP