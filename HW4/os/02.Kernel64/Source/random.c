#include "random.h"

static QWORD next = 1;

QWORD random_generator()
{
    QWORD seed;
    QWORD hi, lo;

    seed = kReadTSC();

    lo = 16807 * (seed & 0xFFFF);
    hi = 16807 * (seed >> 16);

    lo += (hi & 0x7FFF) << 16;
    lo += hi >> 15;

    if (lo > 0x7FFFFFFF)
        lo -= 0x7FFFFFFF;

    return (seed = (QWORD)lo);
}

QWORD cal_ticket(BYTE priority) //total 100000 (45000 + 25000 + 15000 + 10000 + 5000)
{
    switch (priority)
    {
    case TASK_FLAGS_HIGHEST:
        return 45000;
    case TASK_FLAGS_HIGH:
        return 25000;
    case TASK_FLAGS_MEDIUM:
        return 15000;
    case TASK_FLAGS_LOW:
        return 10000;
    case TASK_FLAGS_LOWEST:
        return 5000;
    }
}

// BIGNUM = 9000000
// stride = BIGNUM/cal_ticket(priority)
QWORD cal_stride(BYTE priority) 
{
    switch (priority)
    {
    case TASK_FLAGS_HIGHEST:
        return 20; 
    case TASK_FLAGS_HIGH:
        return 36;
    case TASK_FLAGS_MEDIUM:
        return 60;
    case TASK_FLAGS_LOW:
        return 90;
    case TASK_FLAGS_LOWEST:
        return 180;
    }
}