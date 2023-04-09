#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const {
    return _sender.stream_in().remaining_capacity();
}

size_t TCPConnection::bytes_in_flight() const {
    return _sender.bytes_in_flight();
}

size_t TCPConnection::unassembled_bytes() const {
    return _receiver.unassembled_bytes();
}

size_t TCPConnection::time_since_last_segment_received() const {
    return _time_since_last_segment_received;
}

void TCPConnection::segment_received(const TCPSegment &seg) {
    if(!_active) {
        return;
    }
    // clear the timer
    _time_since_last_segment_received = 0;

    _receiver.segment_received(seg);

    if(seg.header().rst) {
        _close(false);
        return;
    }

    // if we haven't receive the syn, just ignore the segment
    if(!_receiver.ackno().has_value()) {
        return;
    }

    if(seg.header().ack) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
    }

    // the window may open for the sender, so try to fill the window
    _sender.fill_window();

    if(seg.length_in_sequence_space() > 0) {
        // occupies any seq number
        // at least, we make one reply
        if(_sender.segments_out().size() == 0) {
             _sender.send_empty_segment();
        }

    }else {
        // handle the "keep-alive" segments
        if(_receiver.ackno().has_value() && seg.header().seqno == _receiver.ackno().value() - 1) {
            _sender.send_empty_segment();
        }
    }
    _send();

    _check_whether_to_linger(seg);
    if(_check_prerequisites() && !_linger_after_streams_finish) {
        _close(true);
    }
}

bool TCPConnection::active() const {
    return _active;
}

size_t TCPConnection::write(const string &data) {
    if(!_active) {
        return 0;
    }
    size_t len =  _sender.stream_in().write(data);
    _sender.fill_window();
    _send();
    return len;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    if(!_active) {
        return;
    }
    _time_since_last_segment_received += static_cast<uint64_t>(ms_since_last_tick);

    _sender.tick(ms_since_last_tick);
    if(_sender.consecutive_retransmissions() > _cfg.MAX_RETX_ATTEMPTS) {
        // this is now a hopeless connection, kill it
        _clear_sender();
        _sender.send_empty_segment();
        _sender.segments_out().back().header().rst = true;
        _close(false);
    }
    _send();
    if(_check_prerequisites() && !_linger_after_streams_finish) {
        _close(true);
    }else if(_check_prerequisites() && _linger_after_streams_finish && _time_since_last_segment_received >= (10 * _cfg.rt_timeout)) {
        _close(true);
    }
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    // we might be able to send FIN now
    _sender.fill_window();
    _send();
}

void TCPConnection::connect() {
    if(!_active) {
        return;
    }
    _sender.fill_window();
    _send();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            // Dose this really work?
            _sender.send_empty_segment();
            _sender.segments_out().back().header().rst = true;
            _close(false);
            _send();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
