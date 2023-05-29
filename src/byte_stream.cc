#include <algorithm>
#include <stdexcept>
#include <string_view>

#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity )
  : capacity_ { capacity }
  , mem_buf {}
  , bytes_pushed_cnt { 0 }
  , bytes_poped_cnt { 0 }
  , is_end { false }
  , is_err { false }
{}

void Writer::push( string data )
{
  // Your code here.
  size_t allowed_size { 0 };
  allowed_size = min( data.length(), capacity_ - mem_buf.size() ); // flow-controlled
  data = data.substr( 0, allowed_size );
  for ( const char& c : data )
    mem_buf.push_back( c );
  bytes_pushed_cnt += allowed_size;
}

void Writer::close()
{
  // Your code here.
  is_end = true;
}

void Writer::set_error()
{
  // Your code here.
  is_err = true;
}

bool Writer::is_closed() const
{
  // Your code here.
  return is_end;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - mem_buf.size();
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return bytes_pushed_cnt;
}

string_view Reader::peek() const
{
  // Your code here.
  return string_view { &mem_buf.front(), 1 }; // const char*
}

bool Reader::is_finished() const
{
  // Your code here.
  return is_end && mem_buf.empty();
}

bool Reader::has_error() const
{
  // Your code here.
  return is_err;
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  len = min( len, mem_buf.size() ); // avoid illegal len
  int _size = len;
  while ( _size-- > 0 )
    mem_buf.pop_front();
  bytes_poped_cnt += len;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return mem_buf.size();
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return bytes_poped_cnt;
}
