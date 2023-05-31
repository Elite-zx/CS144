#include "reassembler.hh"
#include "byte_stream.hh"
#include <chrono>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <math.h>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  // Your code here.
  process_substr( first_index, data, output );
  cout << "next_index: " << next_index << endl;
  if ( first_index == next_index ) {
    string next_substring = unordered_bytes_buffer[first_index];
    cout << "push to buffer!" << endl;
    output.push( next_substring );
    unordered_bytes_size -= next_substring.size();
    unordered_bytes_buffer.erase( first_index ); // key_type
  }
  cout << "unordered_bytes_size: " << unordered_bytes_size << endl << endl;
  if ( is_last_substring ) // all bytes arrived
  {
    byte_stream_end = true;
    eof_index = first_index + data.size();
  }
  if ( byte_stream_end && output.bytes_pushed() == eof_index ) // all bytes pushed
  {
    cout << "All done!" << endl << endl << endl;
    output.close();
  }
}

void Reassembler::process_substr( uint64_t& first_index, string& data, Writer& output )
{
  /* within the stream’s available capacity():
   * [first_unassembled index,first unacceptable index]*/

  if ( !data.size() ) // empty input , will make last_index invalid (unsigned int -> -1 -> max int)
  {
    cout << "empty input!" << endl;
    return;
  }
  size_t begin_of_storage = output.bytes_pushed();                            // first_unassembled index
  size_t end_of_storage = begin_of_storage + output.available_capacity() - 1; // first unacceptable index -1
  size_t last_index = first_index + data.size() - 1;
  next_index = begin_of_storage;

  cout << "begin of storage: " << begin_of_storage << endl;
  cout << "end_of_storage: " << end_of_storage << endl;
  cout << "last_index: " << last_index << endl;
  cout << "first_index: " << first_index << endl;

  if ( begin_of_storage > end_of_storage ) // buffer is full, waiting for ReadAll
  {
    cout << "buffer is full and storage of reassembler is empty!" << endl;
    return;
  }
  if ( first_index > end_of_storage || last_index < begin_of_storage ) // out of range
  {
    cout << "index out of range!" << endl;
    return;
  } else if ( first_index < begin_of_storage && last_index >= begin_of_storage ) {
    cout << "keep tail!" << endl;
    data = data.substr( begin_of_storage - first_index ); // keep tail
    first_index = begin_of_storage;                       // update first_index
  } else if ( first_index >= begin_of_storage && last_index > end_of_storage ) {
    cout << "keep head!" << endl;
    data = data.substr( 0, end_of_storage - first_index + 1 ); // keep head
  }

  // cout << "string process is done!" << endl;
  // cout << data << endl;
  remove_overlap( first_index, data );
  // cout << "is not  overlap error" << endl;
  unordered_bytes_buffer[first_index] = data;
  unordered_bytes_size += data.size();
  return;
}

void Reassembler::remove_overlap( uint64_t& first_index, std::string& data )
{
  if ( !unordered_bytes_buffer.size() )
    return;

  for ( auto iter = unordered_bytes_buffer.begin(); iter != unordered_bytes_buffer.end(); ) {
    size_t last_index = first_index + data.size() - 1;
    size_t begin_index = iter->first;
    size_t end_index = iter->first + iter->second.size() - 1;
    cout << "begin_index: " << begin_index << " "
         << "end_index: " << end_index << endl;
    cout << "first_index: " << first_index << " "
         << "last_index: " << last_index << endl;

    if ( last_index + 1 == begin_index ) {
      cout << "exactly append!" << endl;
      data += iter->second;
      unordered_bytes_size -= iter->second.size();
      iter = unordered_bytes_buffer.erase( iter ); // not use erase(key), it will casue iterator invalid
    }
    // Overlap between head and tail.
    else if ( first_index <= begin_index && begin_index <= last_index ) { // = reason : "bc" "b"
      cout << "append tail!" << endl;
      if ( end_index <= last_index ) { // completely covered
        unordered_bytes_size -= unordered_bytes_buffer[begin_index].size();
        iter = unordered_bytes_buffer.erase( iter );
      } else {
        data += iter->second.substr( last_index - begin_index + 1 );
        unordered_bytes_size -= unordered_bytes_buffer[begin_index].size();
        iter = unordered_bytes_buffer.erase( iter );
      }
    }

    else if ( begin_index <= first_index && first_index <= end_index ) { // = reason: "bc" "c"
      cout << "append head!" << endl;
      if ( last_index <= end_index ) {
        first_index = begin_index;
        data = iter->second;
        unordered_bytes_size -= iter->second.size();
        iter = unordered_bytes_buffer.erase( iter );
      } else {
        data = iter->second.substr( 0, first_index - begin_index ) + data;
        unordered_bytes_size -= iter->second.size();
        iter = unordered_bytes_buffer.erase( iter );
        first_index = begin_index;
      }
    } else {
      ++iter;
    }
    cout << "after removing overlapping---"
         << "first_index: " << first_index << " last_index: " << first_index + data.size() - 1;
  }
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return unordered_bytes_size;
}
