#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity), \
 _stream(deque<char>(capacity, 0)), _valids(deque<bool>(capacity, false)), _stream_bytes(0),\
 _global_index(0), _eof(false)
 {}

/*
 * NOTE: * means a byte
 * NOTE: ! means the **next** byte after the last byte in the stream which not actually exist
 * what is in the Reassembler:
 * +----------------------------------------------------------+
 * +   ***********************!                               +
 * +   |                      |                               +
 * +   global_index           global_index + _bytes_remain()  +
 * +----------------------------------------------------------+
 * two cases to handle
 * 1. index of incoming substring is less than global_index
 *  1.1 it's ok to push the overlapped substring
 *                 *****************!
 *                 |                |
 *                 index            index + length
 *                               ***********************!
 *                               |                      |
 *                               global_index           global_index + _bytes_remain()
 *  1.2 can not push substring
 *                 *****************!
 *                 |                |
 *                 index            index + length
 *                                              ***********************!
 *                                              |                      |
 *                                              global_index           global_index + _bytes_remain()
 * 2. index of incoming substring is greater than or equal to global_index
 *  2.1 it's ok to push the overlapped substring
 *                 *****************!
 *                 |                |
 *                 index            index + length
 *              ***********************!
 *              |                      |
 *              global_index           global_index + _bytes_remain()
 * 2.2 can not push substring
 *                                          *****************!
 *                                          |                |
 *                                          index            index + length
 *              ***********************!
 *              |                      |
 *              global_index           global_index + _bytes_remain()
 */

// TODO: these helper functions are too simple, implement them inside .h to make them inline
size_t StreamReassembler::_bytes_remain() const{
   return _capacity - _output.buffer_size();
}
bool StreamReassembler::_can_push_substring(const uint64_t index, const size_t length) const{
    if(index < _global_index && (index + length) < (_global_index + 1)) {
        // case 1.2
        return false;
    }else {
        if(index < _global_index) {
            // case 1.1
            return _bytes_remain() > 0;
        }else {
            // case 2
            return _bytes_remain() > 0 && (index - _global_index) < _bytes_remain();
        }
    }
}

size_t StreamReassembler::_bytes_can_push(const uint64_t index, const size_t length) const {
    if(!_can_push_substring(index, length)) {
        return 0;
    }
    // the max bytes limited by _bytes_remain
    auto max_push_bytes = size_t{0};
    if(index < _global_index) {
        max_push_bytes = _bytes_remain();
    }else {
        max_push_bytes = _bytes_remain() - (index - _global_index);
    }
    // the max bytes limited by length
    auto max_length = size_t{0};
    if(index < _global_index) {
        max_length = index + length - _global_index;
    }else {
        max_length = length;
    }
    // determine the final max bytes
    return static_cast<size_t>(min(max_push_bytes, max_length));
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    size_t data_length = data.size();
    size_t push_bytes = _bytes_can_push(index, data_length);
    // the index start from in data
    auto push_start = size_t{0};
    if(index < _global_index) {
        push_start = _global_index - index;
    }else {
        push_start = 0;
    }
    // eof can be set when: not eof now and **we can push the entire string this round**
    if(!_eof) 
        if((push_start + push_bytes) == (data_length))
            _eof = eof;

    if(push_bytes == 0) {
        // check if we can push any stream to ByteStream?
        _push_to_bytestream();
        // update eof
        _check_eof();
        return;
    }
    string segment_data = data.substr(push_start, push_bytes);
    vector<bool> valids(push_bytes, true);
    // the copy start offset in inner stream
    auto offset = size_t{0};
    if(index < _global_index) {
        offset = 0;
    }else {
        offset = index - _global_index;
    }
    copy(segment_data.begin(), segment_data.end(), _stream.begin() + offset);
    // _stream_bytes_next is _stream_bytes_now + how many bytes we push this round - how many bytes exists originally
    _stream_bytes += push_bytes - \
    accumulate(_valids.begin() + offset, _valids.begin() + offset + push_bytes, 0, [](int init, bool val) -> int {return init + (val == true);});
    copy(valids.begin(), valids.end(), _valids.begin() + offset);
    // check if we can push any stream to ByteStream?
    _push_to_bytestream();
    // update eof
    _check_eof();
}

void StreamReassembler::_push_to_bytestream() {
    if(_valids[0] == false) {
        return;
    }
    string segment_string;
    while(_valids[0]) {
        segment_string.push_back(_stream[0]);
        _valids.pop_front();
        _stream.pop_front();
        _valids.push_back(false);
        _stream.push_back(0);
        _global_index++;
    }
    auto bytes_written = _output.write(segment_string);
    assert(bytes_written == segment_string.size());
    _stream_bytes -= bytes_written;
}

size_t StreamReassembler::unassembled_bytes() const { return _stream_bytes; }

bool StreamReassembler::empty() const { return _stream_bytes == 0; }
