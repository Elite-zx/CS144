#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"
#include <queue>
class Timer
{
public:
  Timer( uint64_t RTO = 0 ) : rest_time( RTO ), on_off( false ) {}

  void startTimer( uint64_t RTO )
  {
    on_off = true;
    rest_time = RTO;
  }

  void stopTimer() { on_off = false; }

  bool passageOfTime( uint64_t ms_since_last_tick ) // timeout or not based on ms_since_last_tick
  {
    rest_time -= ms_since_last_tick;
    return on_off && rest_time <= 0;
  }

  bool isActivated() { return on_off; }

private:
  int64_t rest_time;
  bool on_off;
};

class TCPSender
{
  Wrap32 isn_;
  uint64_t _initial_RTO; // self_explaination
  uint64_t RTO;
  std::queue<TCPSenderMessage> seg_ready_que {};       // message ready to be sent
  std::queue<TCPSenderMessage> seg_outstanding_que {}; // message to be sent by maybe_send
  uint64_t _consecutive_retransmissions_cnt { 0 };
  uint64_t rwnd;       // reCeiver WiNDow size, which is flow controlling signal from receiver
  uint64_t next_seqno; // next absolute seqno to be sent
  uint64_t _sequence_numbers_in_flight_cnt;
  Timer _timer;
  enum TCPSender_status
  {
    CLOSED,
    SYN_SENT,
    SYN_ACKED,
    FIN_SENT,
    FIN_ACKED
  } _status;

public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( uint64_t initial_RTO_ms, std::optional<Wrap32> fixed_isn );

  /* Push bytes from the outbound stream */
  void push( Reader& outbound_stream );

  /* fill tcp segmenet header*/
  void fill_seg_header( TCPSenderMessage seg );

  /* Send a TCPSenderMessage if needed (or empty optional otherwise) */
  std::optional<TCPSenderMessage> maybe_send();

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage send_empty_message() const;

  /* Receive an act on a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Time has passed by the given # of MilliSeconds since the last time the tick() method was called. */
  void tick( uint64_t ms_since_last_tick );

  /* Accessors for use in testing */
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
};

