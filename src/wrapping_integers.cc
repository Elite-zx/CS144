#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  return Wrap32 { static_cast<uint32_t>( ( n + zero_point.raw_value_ ) % ( 1ul << 32 ) ) };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  uint64_t seq_range = 1ul << 32;
  int mul = ( checkpoint + zero_point.raw_value_ ) / seq_range;
  return seq_range * mul + this->raw_value_ - zero_point.raw_value_;

  // Your code here.
}
