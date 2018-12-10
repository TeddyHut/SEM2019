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

	hw::LED led_red(hw::PINPORT::LED_RED, hw::PINPOS::LED_RED);
	hw::LED led_green(hw::PINPORT::LED_GREEN, hw::PINPOS::LED_GREEN);
	
	hw::Button button_test(hw::PINPORT::BUTTON_TEST, hw::PINPOS::BUTTON_TEST);
	hw::Button button_horn(hw::PINPORT::BUTTON_HORN, hw::PINPOS::BUTTON_HORN);

	hw::ButtonTimer<Stopwatch1k> buttontimer_test(button_test);
	hw::ButtonTimer<Stopwatch1k> buttontimer_horn(button_horn);

	hw::Blinker::Pattern pattern_oneshot_4;
	pattern_oneshot_4.count = 4;
	pattern_oneshot_4.ontime = 100;
	pattern_oneshot_4.offtime = 300;
	pattern_oneshot_4.resttime = 3000;
	pattern_oneshot_4.inverted = false;
	pattern_oneshot_4.repeat = false;

	//Enable interrupts
	sei();

	while(true) {
		led_red.update();
		led_green.update();
		buttontimer_test.update();
		buttontimer_horn.update();
		if(buttontimer_test.pressedFor(500)) {
			led_red.blinkPattern(pattern_oneshot_4);
		}
	}
}
