#ifndef SPONGE_LIBSPONGE_RETRANSMISSION_TIMER_HH
#define SPONGE_LIBSPONGE_RETRANSMISSION_TIMER_HH

#include "tcp_config.hh"

class RetransmissionTimer{
    private:
        bool _started;
        size_t _time_now;
        size_t _consecutive_cnt;
        size_t _rto;

    
    public:
        RetransmissionTimer(const uint16_t retx_timeout);
        // called when a new segment has been sent
        void check_and_start();
        // called when sender need to tick
        bool tick(const size_t ms_since_last_tick);
        // called when all outstanding data has been acknowledged
        void stop();
        // called when exponential backoff
        void backoff();
        // called when a restart needed
        void restart();

        size_t get_consecutive_cnt() const;
        void   set_consecutive_cnt(const size_t con_cnt);

        size_t get_rto() const;
        void   set_rto(const size_t rto);

};

#endif