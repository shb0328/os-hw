  
#ifndef __RANDOM_H__
#define __RANDOM_H__ 

#include "Types.h"
#include "AssemblyUtility.h"
#include "Task.h"

QWORD random_generator();
QWORD cal_ticket(BYTE priority);
//QWORD rand();
QWORD cal_stride(BYTE priority);
void srand(QWORD seed);

#endif
