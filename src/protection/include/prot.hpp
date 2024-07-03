#ifndef PDISC_HPP
#define PDISC_HPP

#include "ptoc.hpp"

typedef struct protection_config{
    ptoc_config_t ptoc_config;
    int pdisc_level;
    int pdisc_type;
} protection_config_t;



#endif