#ifndef DRAIN_TIME_H
#define DRAIN_TIME_H


#ifdef __cplusplus

#include <chrono> // time library

class rTime
{  
   typedef std::chrono::time_point<std::chrono::high_resolution_clock> _rTime;

   public:

   rTime()  { init(); }

   void  init()   { t = now(); }    

   uint64_t  getTimePassed()
   {  
      return std::chrono::duration_cast<std::chrono::microseconds>(now()-t).count();
   }

   uint64_t getMilliSec()  
   {
      return std::chrono::duration_cast<std::chrono::milliseconds>(now().time_since_epoch()).count();
   }

   uint64_t getMicroSec()
   {
      return std::chrono::duration_cast<std::chrono::microseconds>(now().time_since_epoch()).count();
   }

   uint64_t getNanoSec()
   {
      return std::chrono::duration_cast<std::chrono::nanoseconds>(now().time_since_epoch()).count();
   }

   private:

   _rTime now()
   {
      return std::chrono::high_resolution_clock::now();
   }
   
   _rTime   t;
};

#else
typedef struct rTime {
   uint64_t time;
} rTime; 
#endif

#ifdef __cplusplus
extern "C" {
#endif 

rTime *  rTime_init(rTime *); 
uint64_t rTime_getMilliSec(rTime *);


#ifdef __cplusplus
}
#endif

#endif
