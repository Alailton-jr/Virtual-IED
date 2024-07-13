#ifndef TIMERS_HPP
#define TIMERS_HPP

#include <time.h>

class Timer{
public:
    struct timespec next_period;

    void increment_period(long period_ns) {
        next_period.tv_nsec += period_ns;
        while (next_period.tv_nsec >= 1000000000) {
            next_period.tv_sec += 1;
            next_period.tv_nsec -= 1000000000;
        }
    }

    void start_period(long period_ns) {
        clock_gettime(CLOCK_MONOTONIC, &next_period);
        increment_period(period_ns);
    }

    void wait_period(long period_ns) {
        if(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_period, NULL) == -1){
            std::cerr << "Error in clock_nanosleep" << std::endl;
        };
        increment_period(period_ns);
    }
};















#endif // TIMERS_HPP