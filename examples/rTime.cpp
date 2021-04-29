#include <chrono>
#include "rTime.h"

//static rTime time; 

rTime * rTime_init(rTime * r){
   // initialize the class member
   r->init();
   return r; 
}

uint64_t rTime_getMilliSec(rTime * r)
{
   return r->getMilliSec(); 
}

