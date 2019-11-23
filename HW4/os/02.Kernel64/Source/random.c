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

QWORD cal_ticket(BYTE priority)
{
    switch (priority)
    {
    case TASK_FLAGS_HIGHEST:
        return 35000;
    case TASK_FLAGS_HIGH:
        return 25000;
    case TASK_FLAGS_MEDIUM:
        return 20000;
    case TASK_FLAGS_LOW:
        return 15000;
    case TASK_FLAGS_LOWEST:
        return 50000;
    }
}

QWORD cal_stride(BYTE priority) // stride task 1  BIGNUM = 300000
{
    switch (priority)
    {
    case TASK_FLAGS_HIGHEST:
        return 10000; // stride
    case TASK_FLAGS_HIGH:
        return 12000;
    case TASK_FLAGS_MEDIUM:
        return 15000;
    case TASK_FLAGS_LOW:
        return 200000;
    case TASK_FLAGS_LOWEST:
        return 300000;
    }
}
