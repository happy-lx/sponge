#include "retransmission_timer.hh"

RetransmissionTimer::RetransmissionTimer(const uint16_t retx_timeout) : _started(false) , \
_time_now(0), _consecutive_cnt(0), _rto(retx_timeout) {}

void RetransmissionTimer::check_and_start() {
    if(!_started) {
        _started = true;
        _time_now = 0;
        _consecutive_cnt = 0;
    }
}

bool RetransmissionTimer::tick(const size_t ms_since_last_tick) {
    _time_now += ms_since_last_tick;
    if((_time_now >= _rto) && _started) {
        return true;
    }
    return false;
}

void RetransmissionTimer::stop() {
    _started = false;
}

void RetransmissionTimer::backoff() {
    _rto <<= 1;
}

void RetransmissionTimer::restart() {
    _started = true;
    _time_now = 0;
}

size_t RetransmissionTimer::get_consecutive_cnt() const {
    return _consecutive_cnt;
}

void RetransmissionTimer::set_consecutive_cnt(const size_t con_cnt) {
    _consecutive_cnt = con_cnt;
}

size_t RetransmissionTimer::get_rto() const {
    return _rto;
}

void RetransmissionTimer::set_rto(const size_t rto) {
    _rto = rto;
}