/*
* timercommon.h
*
* Created: 16/01/2019 1:11:00 AM
*  Author: teddy
*/

#pragma once

#include <stdlib.h>

namespace libmodule {
namespace time {

//Empty specialization
template <size_t TickFrequency_c>
class TimerBase;

//Declare start_timer_daemon for friend statement
template<size_t ...>
void start_timer_daemons();
//TODO: Add start_timer_daemons that takes timer types as arguments and deduces the size_t
//e.g. start_timer_daemons<Timer1k>();
//Declare isr_rtc for friend statement (TODO: test whether inline works here)
void isr_rtc();

} //time
} //libmodule
