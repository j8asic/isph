#include "timer.h"
using namespace isph;

Timer::Timer()
{
   Start();
}

Timer::~Timer()
{

}

void Timer::Start()
{
   t_init = std::chrono::steady_clock::now();
}

double Timer::Time()
{
   auto t_end = std::chrono::steady_clock::now();
   auto diff = t_end - t_init;
   return std::chrono::duration<double, std::milli>(diff).count() ;
}
