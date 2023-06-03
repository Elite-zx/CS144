#include "tcp_sender.hh"
#include "tcp_config.hh"
#include "tcp_sender_message.hh"

#include <iostream>
#include <random>

using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender( uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) )
  , _initial_RTO( initial_RTO_ms )
  , RTO( initial_RTO_ms )
  , rwnd { 1 }
  , next_seqno { 0 }
  , _sequence_numbers_in_flight_cnt { 0 }
  , _timer {}
  , _status { CLOSED }
{}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return _sequence_numbers_in_flight_cnt;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return _consecutive_retransmissions_cnt;
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
  if ( seg_ready_que.empty() ) // no segment is ready to be send
    return {};

  TCPSenderMessage _msg = seg_ready_que.front();
  seg_ready_que.pop();

  return _msg;
}

void TCPSender::fill_seg_header( TCPSenderMessage seg )
{
  seg.seqno = Wrap32::wrap( next_seqno, isn_ );

  next_seqno += seg.sequence_length(); // update next_seqno
  _sequence_numbers_in_flight_cnt += seg.sequence_length();

  seg_ready_que.push( seg );          // ready to send
  seg_outstanding_que.emplace( seg ); // prepare for retransmission

  if ( !_timer.isActivated() ) {
    // cout << " timer start!" << endl;
    _timer.startTimer( RTO );
  }
}
/*
 *The TCPSender is asked to fill the window from the outbound byte stream: it reads
 *from the stream and generates as many TCPSenderMessages as possible, as long as
 *there are new bytes to be read and space available in the window.
 */

void TCPSender::push( Reader& outbound_stream )
{
  if ( _status == FIN_SENT )
    return;
  if ( _status == CLOSED ) {
    TCPSenderMessage SYN_msg;
    SYN_msg.SYN = true;
    if ( outbound_stream.is_finished() ) //  for weird test: SYN(true) + no payload + FIN(true)
      SYN_msg.FIN = true;

    // payload is empty for SYN seg
    fill_seg_header( SYN_msg );
    _status = SYN_SENT;

  } else if ( _status == SYN_ACKED ) {
    // if rwnd =0, the push method should pretend like the window size isone.
    uint64_t cur_rwnd = rwnd ? rwnd : 1;

    // Do not send any seg if outstanding bytes is larger than receiver window size
    if ( _sequence_numbers_in_flight_cnt >= cur_rwnd )
      return;
    // take outstanding byte into account
    uint64_t max_sent_volume = cur_rwnd - _sequence_numbers_in_flight_cnt;
    uint64_t bytes_sent_cnt { 0 };

    /*fill rwnd, have free space and buffer is not empty */
    while ( bytes_sent_cnt < max_sent_volume && outbound_stream.bytes_buffered() > 0 ) {
      TCPSenderMessage data_msg;
      read( outbound_stream,
            min( TCPConfig::MAX_PAYLOAD_SIZE, max_sent_volume - bytes_sent_cnt ),
            data_msg.payload ); // payload is shared_ptr
      bytes_sent_cnt += data_msg.payload.size();

      /*set FIN flag if stream is empty but rwnd is not full*/
      if ( outbound_stream.is_finished() && bytes_sent_cnt < max_sent_volume ) {
        data_msg.FIN = true;
        _status = FIN_SENT;
      }
      fill_seg_header( data_msg );
    }

    /* send fin msg if stream is over*/
    if ( max_sent_volume - bytes_sent_cnt > 0 && outbound_stream.is_finished() && _status == SYN_ACKED ) {
      TCPSenderMessage FIN_msg;
      FIN_msg.FIN = true;
      _status = FIN_SENT;
      fill_seg_header( FIN_msg );
    }
  }
}

TCPSenderMessage TCPSender::send_empty_message() const
{
  /*simply set the seqno field, the seg occupies no sequence numbers(so next_seqno no need to increment), doesn’t
need to be kept track of as “outstanding” and won’t ever be retransmitted.*/
  TCPSenderMessage empty_msg;
  empty_msg.seqno = Wrap32::wrap( next_seqno, isn_ );
  return empty_msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  if ( !msg.ackno.has_value() ) // no ackno in receive message,which means before handshaking
  {
    // cout << "ack is none!" << endl;
    return;
  }
  Wrap32 _ackno = Wrap32( msg.ackno.value() );
  uint64_t ack = _ackno.unwrap( isn_, next_seqno ); // absolute sequence

  // cout << "ack: " << ack << endl;

  if ( ack > next_seqno ) // after next_seqno, invalid
  {
    // cout << " after next_seqno, invalid" << endl;
    return;
  }

  rwnd = msg.window_size; // Keep track of the receiver’s window

  if ( _status == SYN_SENT && _ackno == Wrap32::wrap( 1, isn_ ) ) // _ackno equal to client_isn + 1
  {
    _status = SYN_ACKED;
  }

  if ( seg_outstanding_que.empty() ) // no segment need to be acked, receiver msg duplicated
    return;

  auto earliest_msg = seg_outstanding_que.front();
  auto earliest_seq_next = earliest_msg.seqno.unwrap( isn_, next_seqno ) + earliest_msg.sequence_length();
  bool target_ack = false;
  // cout << "earliest_seq: " << earliest_seq_next << endl;
  /* an ackno that acknowledges the successful receipt of new data, the ackno reflects an absolute sequence number
bigger than any previous ackno*/
  while ( ack >= earliest_seq_next ) {
    _sequence_numbers_in_flight_cnt -= earliest_msg.sequence_length();
    target_ack = true;
    seg_outstanding_que.pop();

    if ( seg_outstanding_que.empty() ) // no nonacknowledged msg left in sending window
      break;

    /*An ack may be able to identify multiple segments, because the cumulative acknowledgment mechanism*/
    earliest_msg = seg_outstanding_que.front();
    earliest_seq_next = earliest_msg.seqno.unwrap( isn_, next_seqno ) + earliest_msg.sequence_length();
  }

  /*reset timer*/
  if ( target_ack ) {
    RTO = _initial_RTO;                   // Set the RTO back to its “initial value.”
    _consecutive_retransmissions_cnt = 0; // Reset the count of “consecutive retransmissions” back to zero

    if ( !seg_outstanding_que.empty() ) { // restart the retransmission timer
      {
        // cout << "receive target_ack and still have outstanding seg, restart timer" << endl;
        _timer.startTimer( RTO );
      }
    } else {
      // cout << "receive target_ack and no remaining seg, stop timer!" << endl;
      _timer.stopTimer();
    }
  }
}

/*
 *Function: check timeout or not and carry out retransmission operation, parameter is the Time of MilliSeconds since
 *the last time the tick() method was called.
 */
void TCPSender::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  if ( _timer.passageOfTime( ms_since_last_tick ) ) { // the retransmission timer has expired
    // cout << "trigger retransmission!" << endl;
    //  Retransmit the earliest (lowest sequence number) segment
    TCPSenderMessage msg_retransmit = seg_outstanding_que.front();

    if ( rwnd ) {                         // If the window size is nonzero
      ++_consecutive_retransmissions_cnt; // Keep track of the number of consecutive retransmissions
      RTO *= 2;                           // Double the value of RTO. This is called “exponential backoff”
    }

    if ( _consecutive_retransmissions_cnt <= TCPConfig::MAX_RETX_ATTEMPTS ) {
      seg_ready_que.push( msg_retransmit );
      _timer.startTimer( RTO ); // Reset the retransmission timer, just doubled the value of RTO
    } else                      // abort this connection, too many consecutive retransmissions in a row
    {
      // cout << "abort this connection!" << endl;
      _timer.stopTimer();
    }
  } else {
    // cout << "not timeout!" << endl;
  }
}

