/*
 * TestMaster.cpp
 *
 * Created: 27/11/2018 10:20:32 AM
 * Author : teddy
 */ 

#include <avr/io.h>

#include "hardware/timer.h"

int main(void)
{
	//Disable CCP protection (4 instructions)
	CCP = CCP_IOREG_gc;
	//Enable prescaler and set to 16, therefore a 1MHz CPU clock
	CLKCTRL.MCLKCTRLB = CLKCTRL_PDIV_16X_gc | CLKCTRL_PEN_bm;

	hw::start_timer_daemons<1000>();
	Timer1k timer;
}
