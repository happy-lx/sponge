#ifndef SPONGE_LIBSPONGE_TCP_SENDER_HH
#define SPONGE_LIBSPONGE_TCP_SENDER_HH

#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "retransmission_timer.hh"
#include "wrapping_integers.hh"

#include <functional>
#include <queue>
#include <map>
#include <vector>

//! \brief The "sender" part of a TCP implementation.

//! Accepts a ByteStream, divides it up into segments and sends the
//! segments, keeps track of which segments are still in-flight,
//! maintains the Retransmission Timer, and retransmits in-flight
//! segments if the retransmission timer expires.
class TCPSender {
  private:
    //! our initial sequence number, the number for our SYN.
    WrappingInt32 _isn;

    //! outbound queue of segments that the TCPSender wants sent
    std::queue<TCPSegment> _segments_out{};

    enum WIntRelation {Bigger, Equal, Less};

    //! retransmission timer for the connection
    unsigned int _initial_retransmission_timeout;

    //! outgoing stream of bytes that have not yet been sent
    ByteStream _stream;

    //! the (absolute) sequence number for the next byte to be sent
    uint64_t _next_seqno{0};

    RetransmissionTimer _retransmission_timer;
    WrappingInt32 _now;
    uint16_t _window_size;
    WrappingInt32 _ackno;
    bool _syn;
    bool _fin_sent;
    std::map<uint32_t, TCPSegment> _outstanding{};

    WIntRelation _wrap_int_compare(const WrappingInt32& op1, const WrappingInt32& op2) const {
      auto absolut_op1 = unwrap(op1, _isn, _next_seqno);
      auto absolut_op2 = unwrap(op2, _isn, _next_seqno);

      if(absolut_op1 == absolut_op2) {
        return WIntRelation::Equal;
      }else if(absolut_op1 < absolut_op2) {
        return WIntRelation::Less;
      }else {
        return WIntRelation::Bigger;
      }
    }

    uint64_t _wrap_int_abs(const WrappingInt32& op1, const WrappingInt32& op2) const {
      auto absolut_op1 = unwrap(op1, _isn, _next_seqno);
      auto absolut_op2 = unwrap(op2, _isn, _next_seqno);
      
      return (absolut_op1 > absolut_op2) ? (absolut_op1 - absolut_op2) : (absolut_op2 - absolut_op1);
    }

    bool _fin_valid() const {
      return _stream.input_ended() && !_fin_sent;
    }

    uint64_t _get_max_bytes_we_can_send() const {
      auto _ackno_right_edge = _ackno + static_cast<uint32_t>((_window_size == 0 ? 1 : _window_size));

      auto bytes_max_in_window = uint64_t{0};
      if(_wrap_int_compare(_now, _ackno_right_edge) == WIntRelation::Less) {
        bytes_max_in_window = _wrap_int_abs(_now, _ackno_right_edge);
      }

      auto bytes_max_in_stream = (_syn ? 1 : 0) + _stream.buffer_size() + (_fin_valid() ? 1 : 0);

      auto bytes_max_in_seg = TCPConfig::MAX_PAYLOAD_SIZE + (_syn ? 1 : 0) + (_fin_valid() ? 1 : 0);

      return std::min(bytes_max_in_seg, std::min(bytes_max_in_stream, bytes_max_in_window));
    }

    bool _syn_need_set() const {
      return _syn;
    }

    bool _fin_need_set() const {
      return _get_max_bytes_we_can_send() == (1 + (_syn_need_set() ? 1 : 0) + _stream.buffer_size());
    }

    uint64_t _get_actual_data_bytes() const {
      return _get_max_bytes_we_can_send() - (_syn_need_set() ? 1 : 0) - (_fin_need_set() ? 1 : 0);
    }

    bool _can_ack_this_segment(const WrappingInt32 new_ackno, const TCPSegment &seg) const {
      auto _ackno_right_edge = new_ackno;
      auto _seqno_right_edge = seg.header().seqno + static_cast<uint32_t>(seg.length_in_sequence_space());

      auto compare = _wrap_int_compare(_seqno_right_edge, _ackno_right_edge);
      if(compare == WIntRelation::Equal || compare == WIntRelation::Less) {
        return true;
      }else {
        return false;
      }
    }

    bool _check_valid_ackno(const WrappingInt32 &ackno) const {
      auto compare = _wrap_int_compare(ackno, _now);

      if(compare == WIntRelation::Bigger) {
        return false;
      }else {
        return true;
      }
    }

  public:
    //! Initialize a TCPSender
    TCPSender(const size_t capacity = TCPConfig::DEFAULT_CAPACITY,
              const uint16_t retx_timeout = TCPConfig::TIMEOUT_DFLT,
              const std::optional<WrappingInt32> fixed_isn = {});

    //! \name "Input" interface for the writer
    //!@{
    ByteStream &stream_in() { return _stream; }
    const ByteStream &stream_in() const { return _stream; }
    //!@}

    //! \name Methods that can cause the TCPSender to send a segment
    //!@{

    //! \brief A new acknowledgment was received
    void ack_received(const WrappingInt32 ackno, const uint16_t window_size);

    //! \brief Generate an empty-payload segment (useful for creating empty ACK segments)
    void send_empty_segment();

    //! \brief create and send segments to fill as much of the window as possible
    void fill_window();

    //! \brief Notifies the TCPSender of the passage of time
    void tick(const size_t ms_since_last_tick);
    //!@}

    //! \name Accessors
    //!@{

    //! \brief How many sequence numbers are occupied by segments sent but not yet acknowledged?
    //! \note count is in "sequence space," i.e. SYN and FIN each count for one byte
    //! (see TCPSegment::length_in_sequence_space())
    size_t bytes_in_flight() const;

    //! \brief Number of consecutive retransmissions that have occurred in a row
    unsigned int consecutive_retransmissions() const;

    //! \brief TCPSegments that the TCPSender has enqueued for transmission.
    //! \note These must be dequeued and sent by the TCPConnection,
    //! which will need to fill in the fields that are set by the TCPReceiver
    //! (ackno and window size) before sending.
    std::queue<TCPSegment> &segments_out() { return _segments_out; }
    //!@}

    //! \name What is the next sequence number? (used for testing)
    //!@{

    //! \brief absolute seqno for the next byte to be sent
    uint64_t next_seqno_absolute() const { return _next_seqno; }

    //! \brief relative seqno for the next byte to be sent
    WrappingInt32 next_seqno() const { return wrap(_next_seqno, _isn); }
    //!@}
};

#endif  // SPONGE_LIBSPONGE_TCP_SENDER_HH
