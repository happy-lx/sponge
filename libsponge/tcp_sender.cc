#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity), _retransmission_timer{retx_timeout}, _now{_isn}
    , _window_size(1), _ackno{_isn}, _syn(true), _fin_sent(false) {}

uint64_t TCPSender::bytes_in_flight() const {
    return accumulate(_outstanding.begin(), _outstanding.end(), 0, \
    [](uint64_t init, auto item) -> uint64_t { return init + item.second.length_in_sequence_space();});
}

void TCPSender::fill_window() {
    auto max_bytes = uint64_t{0};
    while((max_bytes = _get_max_bytes_we_can_send()) != 0) {
        TCPSegment seg;
        seg.header().syn = _syn_need_set();
        seg.header().fin = _fin_need_set();
        seg.header().seqno = _now;
        if(_get_actual_data_bytes() > 0) {
            seg.payload() = Buffer{_stream.read(_get_actual_data_bytes())};
        }
        _segments_out.push(seg);
        _outstanding[_now.raw_value()] = seg;
        _retransmission_timer.check_and_start();
        _now = _now + static_cast<uint32_t>(max_bytes);
        _next_seqno += max_bytes;
        if(_syn) 
            _syn = false;
        if(seg.header().fin)
            _fin_sent = true;
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    // if the ackno is far beyond _next_seqno, it is invalid
    if(_check_valid_ackno(ackno) == false) {
        return;
    }

    // ack any outstanding segments
    vector<uint32_t> can_ack_vec;

    for(auto item: _outstanding) {
        if(_can_ack_this_segment(ackno, item.second)) {
            can_ack_vec.push_back(item.first);
        }
    }
    for(auto ack: can_ack_vec) {
        _outstanding.erase(ack);
    }

    if(_wrap_int_compare(_ackno, ackno) == WIntRelation::Less || _wrap_int_compare(_ackno, ackno) == WIntRelation::Equal) {
        // update new window
        _ackno = ackno;
        _window_size = window_size;
    }
    if(can_ack_vec.size() > 0) {
        // update retransmission timer
        _retransmission_timer.set_rto(_initial_retransmission_timeout);
        if(_outstanding.size() > 0) {
            _retransmission_timer.restart();
        }
        _retransmission_timer.set_consecutive_cnt(0);
    }

    // refill the window
    fill_window();

    // if no outstanding segments, stop the timer
    if(_outstanding.size() == 0) {
        _retransmission_timer.stop();
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    auto expire = _retransmission_timer.tick(ms_since_last_tick);
    if(expire) {
        if(_outstanding.size() > 0) {
            _segments_out.push(_outstanding.begin()->second);
            _retransmission_timer.check_and_start();
        }
        if(_window_size > 0) {
            _retransmission_timer.set_consecutive_cnt(_retransmission_timer.get_consecutive_cnt() + 1);
            _retransmission_timer.backoff();
        }
        _retransmission_timer.restart();
    }
}

unsigned int TCPSender::consecutive_retransmissions() const {
    return _retransmission_timer.get_consecutive_cnt();
}

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = _now;
    _segments_out.push(seg);
}
