
#include "pdis.hpp"



void* pdist_Thread(void* arg){

    auto& conf = static_cast<pdis_config> (arg);
    conf.running = 1;

    while (conf.stop){

    }

}
