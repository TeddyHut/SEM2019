/*
 * timer.cpp
 *
 * Created: 4/12/2018 10:11:53 AM
 *  Author: teddy
 */ 

#include "timer.h"

#include <avr/interrupt.h>

ISR(RTC_CNT_vect) {
	hw::isr_rtc();
}

void hw::isr_rtc()
{
	TimerBase<1000>::handle_isr();
}


void hw::TimerBase<1000>::handle_isr()
{
	for(il_count_t i = 0; i < il_count; i++) {
		static_cast<TimerBase *>(il_instances[i])->tick();
	}
}

void hw::TimerBase<1000>::start_daemon()
{
	//Set period value to 1 (since clock runs at 1kHz)
	RTC.PERL = 1;
	//Enable overflow interrupt
	RTC.INTCTRL = RTC_OVF_bm;
	//Set clock source for RTC as 1kHz signal from OSCULP32K
	RTC.CLKSEL = RTC_CLKSEL_INT1K_gc;
	//Enable RTC
	RTC.CTRLA = RTC_RTCEN_bm;
}
