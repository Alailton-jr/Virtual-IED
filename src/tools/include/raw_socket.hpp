#ifndef RAW_SOCKET_HPP
#define RAW_SOCKET_HPP

#include <memory>
#include <cstdint>
#include <cstring>
#include <cerrno>
#include <iostream>
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

#include "general_definition.hpp"

// todo: Add the ability to set the socket options


class RawSocket{

public:
    int32_t socket_id;
    struct sockaddr_ll bind_addr;
    int32_t if_index;
    msghdr msg_hdr;
    struct iovec iov;

public:
    RawSocket(){
        create_socket(0, 0, 0, 0, 0);
        config_MsgHdr();
    }

    ~RawSocket(){
        close(socket_id);
    }

private:

    void create_socket(uint8_t pushFrames2driver, uint8_t txRing, uint8_t timestamping, uint8_t fanout, uint8_t rxTimeout){
        socket_id = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
        if(socket_id < 0){
            std::cerr << "Failed to create socket" << std::endl;
            exit(1);
        }

        if_index = if_nametoindex(IF_NAME);
        memset(&bind_addr, 0, sizeof(bind_addr)); 
        bind_addr.sll_family   = AF_PACKET;
        bind_addr.sll_protocol = htons(ETH_P_ALL);
        bind_addr.sll_ifindex  = if_index;

        if (pushFrames2driver){ // Bypass the kernel qdisc layer and push frames directly to the driver
            static const int32_t sock_qdisc_bypass = 1;
            if (setsockopt(socket_id, SOL_PACKET, PACKET_QDISC_BYPASS, &sock_qdisc_bypass, sizeof(sock_qdisc_bypass)) == -1) {
                std::cerr << "Failed to set PACKET_QDISC_BYPASS" << std::endl;
                // exit(1);
            }
        }
        
        if (txRing){ // Enable the transmit ring
            static const int32_t sock_tx_ring = 1;
            if (setsockopt(socket_id, SOL_PACKET, PACKET_TX_RING, &sock_tx_ring, sizeof(sock_tx_ring)) == -1) {
                std::cerr << "Failed to set PACKET_TX_RING" << std::endl;
                // exit(1);
            }
        }

        if (timestamping){ // Enable packet timestamping
            static const int32_t sock_timestamp = SOF_TIMESTAMPING_RX_HARDWARE | SOF_TIMESTAMPING_RAW_HARDWARE;
            if (setsockopt(socket_id, SOL_SOCKET, SO_TIMESTAMPING, &sock_timestamp, sizeof(sock_timestamp)) == -1) {
                std::cerr << "Failed to set SO_TIMESTAMPING" << std::endl;
                // exit(1);
            }
        }

        if (fanout){ // Enable packet fanout
            static const int32_t sock_fanout = PACKET_FANOUT_HASH;
            if (setsockopt(socket_id, SOL_PACKET, PACKET_FANOUT, &sock_fanout, sizeof(sock_fanout)) == -1) {
                std::cerr << "Failed to set PACKET_FANOUT" << std::endl;
                // exit(1);
            }
        }

        if (rxTimeout){ // Set the receive timeout
            struct timeval timeout;
            timeout.tv_sec  = 4;
            timeout.tv_usec = 0;
            if (setsockopt(socket_id, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
                std::cerr << "Failed to set SO_RCVTIMEO" << std::endl;
                // exit(1);
            }
        }
    
    }

    void config_MsgHdr(){
        memset(&msg_hdr, 0, sizeof(msg_hdr));
        memset(&iov, 0, sizeof(iov));

        // Bind the socket to the interface
        msg_hdr.msg_name = &bind_addr; 
        msg_hdr.msg_namelen = sizeof(bind_addr); 

        // Will be set later
        iov.iov_base = nullptr; 
        iov.iov_len = 0;

        // Set the message header
        msg_hdr.msg_iov = &iov;
        msg_hdr.msg_iovlen = 1;
        msg_hdr.msg_control = NULL;
        msg_hdr.msg_controllen = 0;

    }

};



#endif