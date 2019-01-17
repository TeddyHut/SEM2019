/*
* timer.h
*
* Created: 4/12/2018 10:08:34 AM
*  Author: teddy
*/

#pragma once

#include <avr/io.h>

#include <libmodule/utility.h>
#include <libmodule/timercommon.h>

namespace libmodule {
namespace time {

//Specialization for 1000Hz timers. Implemented using RTC.
template <>
class TimerBase<1000> : public utility::InstanceList<TimerBase<1000>> {
	template <size_t ...>
	friend void start_timer_daemons();
	friend void isr_rtc();
protected:
	virtual void tick() = 0;
private:
	static void start_daemon();
	static void handle_isr();
};

} //time
} //libmodule
