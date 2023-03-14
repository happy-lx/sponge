#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity)
:_stream (string(capacity, 0)), _capacity(capacity), _free_space(capacity), _deq_ptr(0), _enq_ptr(0), \
 _bytes_writen (0), _bytes_read(0), _end(false)
 {}

size_t ByteStream::write(const string &data) {
    auto data_len = data.size();
    if(data_len >= _free_space) {
        data_len = _free_space;
    }
    if(data_len > 0) {
        for(size_t i=0; i<data_len; i++) {
            _stream[_enq_ptr] = data[i];
            _enq_ptr = (_enq_ptr + 1) % _capacity;
        }
        _free_space -= data_len;
        _bytes_writen += data_len;
    }
    return data_len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    string result = "";
    size_t deq_ptr_copy = _deq_ptr;
    for(size_t i=0; i<len; i++) {
        result.push_back(_stream[deq_ptr_copy]);
        deq_ptr_copy = (deq_ptr_copy + 1) % _capacity;
    }
    return result;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    // TODO : _bytes_read ?
    _free_space += len;
    _deq_ptr = (_deq_ptr + len) % _capacity;
    _bytes_read += len;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
string ByteStream::read(const size_t len) {
    string result = peek_output(len);
    pop_output(len);
    return result;
}

void ByteStream::end_input() {
    _end = true;
}

bool ByteStream::input_ended() const {
    return _end;
}

size_t ByteStream::buffer_size() const {
    return (_capacity - _free_space);
}

bool ByteStream::buffer_empty() const {
    return (_capacity == _free_space);
}

bool ByteStream::eof() const {
    return (buffer_empty() && _end);
}

size_t ByteStream::bytes_written() const {
    return _bytes_writen;
}

size_t ByteStream::bytes_read() const {
    return _bytes_read;
}

size_t ByteStream::remaining_capacity() const {
    return _free_space;
}
