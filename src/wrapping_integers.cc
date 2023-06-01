#include "wrapping_integers.hh"
#include <cstdint>

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  // return Wrap32 { static_cast<uint32_t>( ( n + zero_point.raw_value_ ) % ( 1ul << 32 ) ) };
  return zero_point + static_cast<uint32_t>( n );
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  uint64_t uint32_range = 1ul << 32;
  uint32_t offset = this->raw_value_ - wrap( checkpoint, zero_point ).raw_value_;
  uint64_t ans = checkpoint + offset;

  if ( offset >= 1ul << 31 && ans >= uint32_range ) // ?
    ans -= uint32_range;
  return ans;
  // Your code here.
}
