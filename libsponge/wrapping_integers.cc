#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    return WrappingInt32{static_cast<uint32_t>((static_cast<uint64_t>(isn.raw_value()) + n) % (1UL << 32))};
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    auto initial_sequence_num = isn.raw_value();
    auto relative_sequence_num = n.raw_value();

    auto offset = static_cast<uint64_t>(relative_sequence_num - initial_sequence_num);
    // auto step = uint64_t{1UL << 32};

    auto checkpoint_bound = (checkpoint & ~(static_cast<uint64_t>(UINT32_MAX)));
    auto checkpoint_upper_bound = (checkpoint & ~(static_cast<uint64_t>(UINT32_MAX))) + uint64_t{1UL << 32};
    auto checkpoint_lower_bound = (checkpoint & ~(static_cast<uint64_t>(UINT32_MAX))) - uint64_t{1UL << 32};

    auto opt1_res = checkpoint_upper_bound + offset;
    auto opt2_res = checkpoint_lower_bound + offset;
    auto opt3_res = checkpoint_bound + offset;

    auto opt1_distance = opt1_res > checkpoint ? (opt1_res - checkpoint) : (checkpoint - opt1_res);
    auto opt2_distance = opt2_res > checkpoint ? (opt2_res - checkpoint) : (checkpoint - opt2_res);
    auto opt3_distance = opt3_res > checkpoint ? (opt3_res - checkpoint) : (checkpoint - opt3_res);

    auto min_distance = min(opt1_distance, min(opt2_distance, opt3_distance));

    return (min_distance == opt1_distance) ? opt1_res : ((min_distance == opt2_distance) ? opt2_res : opt3_res);
}
