

#include "sniffer.hpp"

#include <chrono>
#include <vector>
#include <numeric>
#include <iostream>
#include <thread>

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

#include <fftw3.h>
#include <math.h>

#include <fstream>
#include <sstream>

std::vector<std::vector<int32_t>> buffer;
int32_t idx_buffer;
int32_t windows_step;
fftw_complex *fft_in, *fft_out;
fftw_plan fft_plan;
int idx_angRef = 4;
sampledValue_info* sv_info_p;
int updateFlag = 0;

std::vector<double>* module;
std::vector<double>* ang;

pthread_mutex_t* mutex;
pthread_cond_t* cond;

std::vector<uint8_t*> registeredMACs;

int debug_count=0;

void process_buffer(){
    int idx;
    for (int i=0; i<sv_info_p->noChannels; i++){ // Channels

        if(sv_info_p->channel_conf[i] == -1) continue;

        for (int j=0; j<sv_info_p->smpRate; j++){
            idx = (idx_buffer - sv_info_p->smpRate) + j;
            if (idx < 0) idx += buffer[0].size();
            fft_in[j][0] = buffer[i][idx];
            fft_in[j][1] = 0;
        }
        
        fftw_execute(fft_plan);
        (*module)[sv_info_p->channel_conf[i]] = 1.4142135623730951 * sqrt(fft_out[1][0]*fft_out[1][0] + fft_out[1][1]*fft_out[1][1]) / sv_info_p->smpRate;
        (*ang)[sv_info_p->channel_conf[i]] = atan2(fft_out[1][1], fft_out[1][0]);
    }
    
    // save_to_csv("Test.csv");
    // std::ofstream file("Test2.csv");
    // for (int i=0; i<sv_info_p->noChannels; i++){
    //     for (int j=0; j<sv_info_p->smpRate; j++){
    //         idx = (idx_buffer - sv_info_p->smpRate) + j;
    //         if (idx < 0) idx += buffer[0].size();
    //         file << buffer[i][idx];
    //         if (j < sv_info_p->smpRate - 1) file << ",";
    //     }
    //     file << "\n";
    // }
    // file.close();
    // if(debug_count == 8) exit(0);

    double angRef = (*ang)[idx_angRef];
    for (int i=0; i<sv_info_p->noChannels; i++){
        (*ang)[i] = (*ang)[i] - angRef;
    }

    // Debug
    debug_count++;
    if (debug_count > 1000){
        for (int i=0; i<sv_info_p->noChannels; i++){
            if ((*ang)[i] > M_PI){
                (*ang)[i] = 2*M_PI - (*ang)[i];
                std::cout << (*module)[i] << "|_" << (*ang)[i]*180/M_PI << std::endl;
            }else if ((*ang)[i] < -M_PI){
                (*ang)[i] = 2*M_PI + (*ang)[i];
                std::cout << (*module)[i] << "|_" << (*ang)[i]*180/M_PI << std::endl;
            }else{
                std::cout << (*module)[i] << "|_" << (*ang)[i]*180/M_PI << std::endl;
            }
            
        }
        debug_count = 0;
        std::cout << " " << std::endl;
    }

    pthread_cond_broadcast(cond);

    return;
}

struct task_arg{
    uint8_t* pkt;
    ssize_t pkt_len;
    sampledValue_info info;
};

void process_SV_packet(uint8_t* frame, ssize_t frameSize, sampledValue_info sv, int i){

    i += (frame[i+11] == 0x82) ? 17 : (frame[i+11] == 0x81) ? 16 : 15; // Skip SV Header until seqAsdu
    
    int noAsdu = frame[i-1]; // Number of ASDUs

    i += (frame[i+1] == 0x82) ? 4 : (frame[i+1] == 0x81) ? 3 : 2; // Skip until first asdu

    for (int k = 0; k < noAsdu; k++) { // Decode Each Asdu
        i += (frame[i+1] == 0x82) ? 4 : (frame[i+1] == 0x81) ? 3 : 2;
        
        while (frame[i] != 0x87) { // Skip all the fields until Sequence of Data
            /*
                * 0x80 -> svId
                * 0x81 -> datSet Name
                * 0x82 -> smpCnt
                * 0x83 -> confRev
                * 0x84 -> refrTm
                * 0x85 -> sympSync
                * 0x86 -> smpRate
                * 0x87 -> Sequence of Data
                * 0x88 -> SmpMod
                * 0x89 -> gmIdentity
            */
            if (frame[i] == 0x80) { // If tag is svId 0x80

            }
            // if (frame[i] == 0x82) {

            // }
            i += frame[i+1] + 2; // Skip field Tag, Length and Value
            if (i >= frameSize) return;
        }
        updateFlag = 1;
        int j = 0;
        while (j < frame[i+1]) { // Decode the raw values
            buffer[j/8][idx_buffer] = static_cast<int32_t>((frame[i+5+j]) | (frame[i+4+j]*256) | (frame[i+3+j]*65536) | (frame[i+2+j]*16777216));
            j += 8;
        }
        idx_buffer++;
        if (idx_buffer > buffer[0].size()) {
            idx_buffer = 0;
        }
        if(idx_buffer % windows_step == 0){
            process_buffer();
        }
        i += frame[i+1] + 2;
    }

}

void process_GOOSE_packet(uint8_t* frame, ssize_t frameSize, int i){

    i += (frame[i+11] == 0x82) ? 17 : (frame[i+11] == 0x81) ? 16 : 15; // Skip SV Header until seqAsdu
    
}

void process_pkt(task_arg* arg) {

    // Todo: Chech for PRP Packets, do not duplicate the data from them

    uint8_t* frame = arg->pkt;
    ssize_t frameSize = arg->pkt_len;
    sampledValue_info sv = arg->info;

    // -------- Process the frame -------- //

    // Check if the mac exist in the registeredMACs
    int mac_found = 0;
    for (int i=0; i<registeredMACs.size(); i++){
        if (memcmp(frame, registeredMACs[i], 6) == 0){
            mac_found = 1;
            break;
        }
    }
    if (!mac_found) return;

    // uint16_t smpCount;
    int j = 0;
    int i = (frame[12] == 0x81 && frame[13] == 0x00) ? 16 : 12; // Skip Ethernet and vLAN

    if ((frame[i] == 0x88 && frame[i+1] == 0xba)){ // Check if packet is SV
        process_SV_packet(frame, frameSize, sv, i);
    }else if ((frame[i] == 0x88 && frame[i+1] == 0xba)){
        process_GOOSE_packet(frame, frameSize, i);
    }else return;

}

void* watchdog_thread(void* arg){
    using namespace std::chrono;
    auto sv_conf = static_cast<SnifferClass*>(arg);
    while (!sv_conf->stop) {
        if (updateFlag) updateFlag = 0;
        else {
            for(int i=0;i<sv_info_p->noChannels;i++){
                (*module)[i] = 0;
                (*ang)[i] = 0; 
                idx_buffer = 0;
            }
            pthread_cond_broadcast(cond);
        }
        std::this_thread::sleep_for(microseconds(10*208));
    }
    return nullptr;
}

void* SnifferThread(void* arg){
    using namespace std::chrono;
    auto sniffer_conf = static_cast<SnifferClass*>(arg);

    

    sniffer_conf->running = 1;
    mutex = &sniffer_conf->sniffer_mutex;
    cond = &sniffer_conf->sniffer_cond;

    // Buffer for SV data captured
    buffer = std::vector(
        sniffer_conf->sv_info.noChannels, 
        std::vector<int32_t>( 2 * sniffer_conf->sv_info.smpRate )
    );

    sv_info_p = &sniffer_conf->sv_info;
    idx_buffer = 0;
    windows_step = (int32_t) (sniffer_conf->sv_info.smpRate * WINDOW_STEP);
    module = &sniffer_conf->phasor_mod;
    ang = &sniffer_conf->phasor_ang;
    registeredMACs.push_back(sniffer_conf->sv_info.mac_dst);

    // Create and Allocate FFTW3
    fft_in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * sv_info_p->smpRate);
    fft_out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * sv_info_p->smpRate);
    fft_plan = fftw_plan_dft_1d(sv_info_p->smpRate, fft_in, fft_out, FFTW_FORWARD, FFTW_ESTIMATE);

    pthread_t watchDog_thd;
    pthread_create(&watchDog_thd, nullptr, watchdog_thread, arg);

    // Create a raw socket
    RawSocket raw_socket;
    ThreadPool<void(task_arg*)> pool(sniffer_conf->noThreads, sniffer_conf->noTasks, sniffer_conf->priority);

    // Variables used for decoding and the Thread Pool 
    uint8_t args_buff[Sniffer_NoTasks+1][Sniffer_RxSize];
    ssize_t rx_bytes;
    raw_socket.iov.iov_len = Sniffer_RxSize;

    int32_t idx_task = 0;

    while (!sniffer_conf->stop) {

        raw_socket.msg_hdr.msg_iov->iov_base = args_buff[idx_task];
        rx_bytes = recvmsg(raw_socket.socket_id, &raw_socket.msg_hdr, 0);

        if (rx_bytes < 0 || rx_bytes > Sniffer_RxSize) {
            std::cerr << "Failed to receive message" << std::endl;
            continue;
        }
        // if (memcmp(args_buff[idx_task], sniffer_conf->sv_info.mac_dst, 6) != 0){
        //     continue; // Check if packet is for this IED
        // }
            
        // Submit task to thread pool
        pool.submit(
            process_pkt,
            std::shared_ptr<task_arg*> (
                new task_arg*(new task_arg{
                    .pkt = args_buff[idx_task],
                    .pkt_len = rx_bytes,
                    .info = sniffer_conf->sv_info 
                }), 
                [](task_arg** p) { delete *p; delete p;}
            )
        );

        ++idx_task;
        if (idx_task > Sniffer_NoTasks) 
            idx_task = 0;
    }

    fftw_free(fft_in);
    fftw_free(fft_out);
    fftw_destroy_plan(fft_plan);

    pthread_join(watchDog_thd, nullptr);
    sniffer_conf->running = 0;

    return nullptr;
}