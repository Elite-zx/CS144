#include "tcp_receiver.hh"
#include <cstdint>

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  // Your code here.
  if ( !after_handshaking ) // no handshaking
  {
    if ( message.SYN ) {
      after_handshaking = true; // after handshaking
      zero_point = Wrap32 { message.seqno };
    } else {
      return;
    }
  }

  uint64_t checkpoint = inbound_stream.bytes_pushed() + 1; // first unassembled index
  uint64_t absolute_seqno = Wrap32( message.seqno ).unwrap( zero_point, checkpoint );
  uint64_t first_index = absolute_seqno + message.SYN + -1; // since zero, remove SYN if exist
  reassembler.insert( first_index, message.payload, message.FIN, inbound_stream );
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  // Your code here.
  TCPReceiverMessage msg;

  /*window_size*/
  uint64_t _available_capacity = inbound_stream.available_capacity();
  msg.window_size = _available_capacity > UINT16_MAX ? UINT16_MAX : _available_capacity;

  /*ackno*/
  if ( !after_handshaking ) {
    msg.ackno = nullopt; // or {} instead
  } else {
    uint64_t next_expected_seq = inbound_stream.bytes_pushed() + 1;
    if ( inbound_stream.is_closed() ) // FIN
      ++next_expected_seq;
    msg.ackno = Wrap32::wrap( next_expected_seq, zero_point ); // wrap
  }
  return msg;
}
