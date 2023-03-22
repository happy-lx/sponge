#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    if(seg.header().syn) {
        _isn = std::optional<WrappingInt32>{seg.header().seqno};
    }
    if(!_isn) {
        return;
    }
    auto end = seg.header().fin;
    auto index = size_t{0};
    if(seg.header().syn) {
        index = 0;
    }else {
        index = unwrap(seg.header().seqno, _isn.value(), _reassembler.get_global_index()) - 1;
    }
    _reassembler.push_substring(seg.payload().copy(), index, end);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if(_isn) {
        return wrap(_reassembler.get_global_index() + 1, _isn.value()) + (_reassembler.stream_out().input_ended() ? 1 : 0);
    }else {
        return std::nullopt;
    }
}

size_t TCPReceiver::window_size() const {
    return _capacity - _reassembler.stream_out().buffer_size();
}
