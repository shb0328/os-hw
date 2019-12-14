#include "random.h"

QWORD random_generator(){
    QWORD seed;
    QWORD hi, lo;

    seed = kReadTSC();

    lo = 16807 * ( seed & 0xFFFF );
    hi = 16807 * ( seed >> 16 );

    lo += ( hi & 0x7FFF ) << 16;
    lo += hi >> 15;

    if ( lo > 0x7FFFFFFF ) lo -= 0x7FFFFFFF;

    return ( seed = ( QWORD ) lo );
}