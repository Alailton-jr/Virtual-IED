
#include "api.hpp"

class APIClass {
public:
    int running, stop;

};

void* handle_client(void* arg){
    return nullptr;
}

void* runServer(void *arg){
    auto conf = reinterpret_cast<APIClass*>(arg);
    RawSocket Server = RawSocket();

    std::vector<int> clients;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    while(conf->running){
        int client_socket;
        if ((client_socket = accept(Server.socket_id, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
            std::cerr << "Failed to accept connection" << std::endl;
        }else{
            std::cout << "Connection accepted -- IP: "<< inet_ntoa(address.sin_addr) << std::endl;
            handle_client((void *)(&client_socket));
            clients.push_back(client_socket);
            // pthread_create(&curClient->thread, NULL, handle_client, (void *)curClient);
        }
    }
    return nullptr;
}

    