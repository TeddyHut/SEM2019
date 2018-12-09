/*
 * TestMaster.cpp
 *
 * Created: 27/11/2018 10:20:32 AM
 * Author : teddy
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "hardware/timer.h"
#include "hardware/userio.h"

int main(void)
{
	////Disable CCP protection (4 instructions)
	CCP = CCP_IOREG_gc;
	////Enable prescaler and set to 16, therefore a 1MHz CPU clock
	CLKCTRL.MCLKCTRLB = CLKCTRL_PDIV_16X_gc | CLKCTRL_PEN_bm;

	hw::start_timer_daemons<1000>();
	//Enable interrupts
	sei();

	hw::LED blinker(PORTA, 4);
	hw::Blinker::Pattern pattern;
	pattern.count = 5;
	pattern.ontime = 100;
	pattern.offtime = 200;
	pattern.resttime = 3000;
	pattern.inverted = true;
	pattern.repeat = true;;
	blinker.blinkPattern(pattern);
	while(true) {
		blinker.update();
		//_delay_ms(1000 / 60);
	}
}
