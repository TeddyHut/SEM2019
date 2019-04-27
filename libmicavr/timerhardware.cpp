/*
 * timerhardware.cpp
 *
 * Created: 4/12/2018 10:11:53 AM
 *  Author: teddy
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>

#include "timerhardware.h"

ISR(RTC_PIT_vect) {
	libmodule::time::isr_rtc();
	RTC.PITINTFLAGS = RTC_PI_bm;
}

void libmodule::time::isr_rtc()
{
	TimerBase<1000>::handle_isr();
}

void libmodule::time::TimerBase<1000>::handle_isr()
{
	for(il_count_t i = 0; i < il_instances.size(); i++) {
		static_cast<TimerBase *>(il_instances[i])->tick();
	}
}

void libmodule::time::TimerBase<1000>::start_daemon()
{
	//Set clock source for RTC as 1kHz signal from OSCULP32K
	RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;
	//Enable PIT interrupt
	RTC.PITINTCTRL = RTC_PI_bm;
	//Enable PIT interrupt, every 32 cycles
	RTC.PITCTRLA = RTC_PITEN_bm | RTC_PERIOD_CYC32_gc;
}
