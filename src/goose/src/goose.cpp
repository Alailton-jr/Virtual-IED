#include "goose.hpp"

// TODO: Add more data types for goose to send

std::string getMacAddress(const std::string& interface) {
    int sockfd;
    struct ifreq ifr;

    // Create a socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return "";
    }

    // Get the MAC address
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ-1);
    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
        perror("ioctl");
        close(sockfd);
        return "";
    }

    // Close the socket
    close(sockfd);

    // Convert the MAC address to a readable format
    unsigned char* mac = reinterpret_cast<unsigned char*>(ifr.ifr_hwaddr.sa_data);
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    return std::string(macStr);
}

void sendPacketRaw(std::vector<uint8_t>& packet, const std::string& interface, int n, IECGoose& goose) {
    
    int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    struct sockaddr_ll bind_addr;

    int if_index = if_nametoindex(interface.c_str()); // Get the interface index
    int bind_addrSize = sizeof(bind_addr); // Set the bind address size
    memset(&bind_addr, 0, sizeof(bind_addr)); // Clear the bind address
    bind_addr.sll_family   = AF_PACKET; // Set the bind address family to AF_PACKET -> Ethernet
    bind_addr.sll_protocol = htons(ETH_P_ALL); // Set the bind address protocol to ETH_P_ALL -> not promiscuous
    bind_addr.sll_ifindex  = if_index; // Set the bind address interface index


    int32_t tx_bytes;
    struct msghdr msg_hdr;
    struct iovec iov;
    memset(&msg_hdr, 0, sizeof(msg_hdr));
    memset(&iov, 0, sizeof(iov));
    msg_hdr.msg_name = &bind_addr;
    msg_hdr.msg_namelen = bind_addrSize;
    msg_hdr.msg_iov = &iov;
    msg_hdr.msg_iovlen = 1;


    auto gooseStart = packet.size();
    auto pkt_goose = goose.getEncoded();
    auto idx_sqNum = goose.getParamPos("sqNum") + gooseStart;
    auto idx_stNum = goose.getParamPos("stNum") + gooseStart;
    auto idx_t = goose.getParamPos("t") + gooseStart;

    auto idx_allData = goose.getParamPos("allData") + gooseStart;

    packet.insert(packet.end(), pkt_goose.begin(), pkt_goose.end());

    iov.iov_base = (void*)packet.data();
    iov.iov_len = packet.size();

    struct timespec t0;

    for (int i=0;i<n;i++){

        packet[idx_sqNum] = (i>>24)&0xFF;
        packet[idx_sqNum+1] = (i>>16)&0xFF;
        packet[idx_sqNum+2] = (i>>8)&0xFF;
        packet[idx_sqNum+3] = i&0xFF;

        packet[idx_stNum] = (i>>24)&0xFF;
        packet[idx_stNum+1] = (i>>16)&0xFF;
        packet[idx_stNum+2] = (i>>8)&0xFF;
        packet[idx_stNum+3] = i&0xFF;

        packet[idx_allData+2] = i;

        clock_gettime(CLOCK_REALTIME, &t0);
        auto tEncoded = UtcTime::staticGetEncoded(t0.tv_sec, t0.tv_nsec);


        packet[idx_t] = tEncoded[0];
        packet[idx_t+1] = tEncoded[1];
        packet[idx_t+2] = tEncoded[2];
        packet[idx_t+3] = tEncoded[3];
        packet[idx_t+4] = tEncoded[4];
        packet[idx_t+5] = tEncoded[5];
        packet[idx_t+6] = tEncoded[6];
        packet[idx_t+7] = tEncoded[7];

        usleep(100000);
        sendmsg(sockfd, &msg_hdr, 0);
    }
    close(sockfd);
}

void* continuous_sender(void* arg){

    auto conf = *reinterpret_cast<std::shared_ptr<sender_config>*>(arg);

    uint16_t sqNum = 1;
    long next_time = conf->minTime*1e6;
    long max_time = conf->maxTime*1e6;
    Timer timer;
    timer.start_period(1e6);
    while (!conf->stop){
        conf->pkt[conf->idx_sqNum] = (sqNum>>24)&0xFF;
        conf->pkt[conf->idx_sqNum+1] = (sqNum>>16)&0xFF;
        conf->pkt[conf->idx_sqNum+2] = (sqNum>>8)&0xFF;
        conf->pkt[conf->idx_sqNum+3] = sqNum&0xFF;

        timer.wait_period(next_time);
        if (conf->stop){
            break;
        }

        pthread_mutex_lock(conf->mutex);
        conf->raw_socket->iov.iov_base = (void*)conf->pkt.data();
        conf->raw_socket->iov.iov_len = conf->pkt.size();
        sendmsg(conf->raw_socket->socket_id, &conf->raw_socket->msg_hdr, 0);
        pthread_mutex_unlock(conf->mutex);
        sqNum++;
        next_time *= 2;
        if (next_time > max_time){
            next_time = max_time;
        }
    }
    return nullptr;
}

void* single_goose(void* arg){
    auto go_conf = reinterpret_cast<Goose_Config*>(arg);

    std::vector<uint8_t> base_pkt;

    Ethernet eth(go_conf->srcMac, go_conf->dstMac);
    auto encoded_eth = eth.getEncoded();
    base_pkt.insert(base_pkt.end(), encoded_eth.begin(), encoded_eth.end());

    if (go_conf->vlanId != -1){
        Virtual_LAN vlan(go_conf->vlanId, go_conf->vlanPcp, go_conf->vlanDei);
        auto encoded_vlan = vlan.getEncoded();
        base_pkt.insert(base_pkt.end(), encoded_vlan.begin(), encoded_vlan.end());
    }
    struct timespec t0;
    clock_gettime(CLOCK_REALTIME, &t0);

    IECGoose goose(
        go_conf->appId,
        go_conf->gocbRef,
        go_conf->maxTime*2,
        go_conf->dataSet,
        UtcTime(t0.tv_sec, t0.tv_nsec),
        1,
        1,
        go_conf->confRev,
        go_conf->data_type.size(),
        go_conf->data_type
    );

    auto encoded_goose = goose.getEncoded();

    int idx_goose_start = base_pkt.size();
    int idx_sqNum = goose.getParamPos("sqNum") + idx_goose_start;
    int idx_stNum = goose.getParamPos("stNum") + idx_goose_start;
    int idx_t = goose.getParamPos("t") + idx_goose_start;
    int idx_allData = goose.getParamPos("allData") + idx_goose_start;
    
    base_pkt.insert(base_pkt.end(), encoded_goose.begin(), encoded_goose.end());

    uint16_t stNum = 0;

    std::vector<Data> current_data;
    current_data.reserve(go_conf->data_type.size());

    // Initialization
    for (int i=0;i<go_conf->data_pointers.size();i++){
        switch (go_conf->data_type[i].type)
        {
            case Data::Type::Boolean: {
                bool* ptr = static_cast<bool*>(go_conf->data_pointers[i]);
                Data temp = Data(Data::Type::Boolean);
                temp.boolean = *ptr;
                current_data.push_back(temp);
                break;
            }
            default:{
                std::cout << "Programming Error" << std::endl;
                break;
            }
        }
    }

    int value_changed = 1, offSet = 0;
    std::shared_ptr<sender_config> sender;

    while (!*go_conf->stop){

        if (value_changed){

            if (sender != nullptr){
                sender->stop = 1;
                pthread_detach(sender->thd);
                sender.reset();
            }

            clock_gettime(CLOCK_REALTIME, &t0);
            auto tEncoded = UtcTime::staticGetEncoded(t0.tv_sec, t0.tv_nsec);
            
            stNum++;
            base_pkt[idx_stNum] = (stNum>>24)&0xFF;
            base_pkt[idx_stNum+1] = (stNum>>16)&0xFF;
            base_pkt[idx_stNum+2] = (stNum>>8)&0xFF;
            base_pkt[idx_stNum+3] = stNum&0xFF;

            base_pkt[idx_sqNum] = 0;
            base_pkt[idx_sqNum+1] = 0;
            base_pkt[idx_sqNum+2] = 0;
            base_pkt[idx_sqNum+3] = 0;

            base_pkt[idx_t] = tEncoded[0];
            base_pkt[idx_t+1] = tEncoded[1];
            base_pkt[idx_t+2] = tEncoded[2];
            base_pkt[idx_t+3] = tEncoded[3];
            base_pkt[idx_t+4] = tEncoded[4];
            base_pkt[idx_t+5] = tEncoded[5];
            base_pkt[idx_t+6] = tEncoded[6];
            base_pkt[idx_t+7] = tEncoded[7];

            offSet = 2;
            for (int i=0;i<go_conf->data_pointers.size();i++){
                switch (go_conf->data_type[i].type)
                {
                    case Data::Type::Boolean: {
                        bool* ptr = static_cast<bool*>(go_conf->data_pointers[i]);
                        base_pkt[idx_allData+offSet] = *ptr;
                        offSet += 3; // TAG[1] + LEN[1] + VAL[1]
                        break;
                    }
                    default:{
                        std::cout << "Programming Error" << std::endl;
                        break;
                    }
                }
            }

            pthread_mutex_lock(go_conf->mutex);
            go_conf->raw_socket->iov.iov_base = (void*)base_pkt.data();
            go_conf->raw_socket->iov.iov_len = base_pkt.size();
            sendmsg(go_conf->raw_socket->socket_id, &go_conf->raw_socket->msg_hdr, 0);
            pthread_mutex_unlock(go_conf->mutex);

            sender = std::make_shared<sender_config>();

            sender->pkt = base_pkt;
            sender->idx_sqNum = idx_sqNum;
            sender->minTime = go_conf->minTime;
            sender->maxTime = go_conf->maxTime;
            sender->raw_socket = go_conf->raw_socket;
            sender->stop = 0;
            sender->mutex = go_conf->mutex;

            pthread_create(&sender->thd, NULL, continuous_sender, static_cast<void*>(&sender));
        }

        // Check if the values have changed
        pthread_mutex_lock(go_conf->prot_mutex);
        value_changed = 0;
        for (int i=0;i<go_conf->data_pointers.size();i++){
            switch (go_conf->data_type[i].type)
            {
                case Data::Type::Boolean: {
                    bool* ptr = static_cast<bool*>(go_conf->data_pointers[i]);
                    if (current_data[i].boolean != *ptr){
                        current_data[i].boolean = *ptr;
                        value_changed = 1;
                    }
                    break;
                }
                default:{
                    std::cout << "Programming Error" << std::endl;
                    break;
                }
            }
        }
        pthread_cond_wait(go_conf->prot_cond, go_conf->prot_mutex);
        pthread_mutex_unlock(go_conf->prot_mutex);
    }
    return nullptr;
}

void* run_goose_sender(void* arg){
    auto conf = reinterpret_cast<GooseClass*>(arg);

    conf->running = 1;
    conf->stop = 0;

    // Open Socket
    RawSocket rawSocket;
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);

    for (int i=0; i<conf->goose_config.size(); i++){
        conf->goose_config[i].stop = &conf->stop;
        conf->goose_config[i].mutex = &mutex;
        conf->goose_config[i].raw_socket = &rawSocket;
        conf->goose_config[i].prot_cond = conf->prot_cond;
        conf->goose_config[i].prot_mutex = conf->prot_mutex;
        pthread_create(&conf->goose_config[i].thd, NULL, single_goose, static_cast<void*>(&conf->goose_config[i]));
    }

    for (int i=0; i<conf->goose_config.size(); i++){
        pthread_join(conf->goose_config[i].thd, NULL);
    }
    pthread_mutex_destroy(&mutex);

    conf->running = 0;
    return nullptr;
}

// Debug and test
int main_goose() {

    int flag1, flag2, flag3;

    auto go1 = Goose_Config{
        .srcMac = getMacAddress("eth0"),
        .dstMac = "01:a0:f4:08:2f:77",
        .vlanId = 100,
        .vlanPcp = 4,
        .appId = 4000,
        .gocbRef = "GEDeviceF650/LLN0$GO$gcb01",
        .minTime = 100,
        .maxTime = 2000,
        .dataSet = "GEDeviceF650/LLN0$GOOSE1",
        .confRev = 1,
        .data_pointers = {&flag1, &flag2, &flag3},
        .data_type = {
            Data(Data::Type::Boolean),
            Data(Data::Type::Boolean),
            Data(Data::Type::Boolean),
        },
    };

    auto go2 = Goose_Config{
        .srcMac = getMacAddress("eth0"),
        .dstMac = "01:a0:f4:08:2f:78",
        .vlanId = 100,
        .vlanPcp = 4,
        .appId = 4000,
        .gocbRef = "GEDeviceF650/LLN0$GO$gcb01",
        .minTime = 100,
        .maxTime = 2000,
        .dataSet = "GEDeviceF650/LLN0$GOOSE1",
        .confRev = 1,
        .data_pointers = {&flag1, &flag2, &flag3},
        .data_type = {
            Data(Data::Type::Boolean),
            Data(Data::Type::Boolean),
            Data(Data::Type::Boolean),
        },
    };

    GooseClass sender_conf;
    sender_conf.goose_config.push_back(go1);
    sender_conf.goose_config.push_back(go2);

    flag1 = flag2 = flag3 = 0;

    pthread_create(&sender_conf.thd, NULL, run_goose_sender, (void*)&sender_conf);

    for (int i=0;i<20;i++){
        flag1 = i%2;
        flag2 = i%3;
        flag3 = i%4;
        sleep(1);
    }

    sender_conf.stop = 1;

    pthread_join(sender_conf.thd, NULL);

    return 0;
}
