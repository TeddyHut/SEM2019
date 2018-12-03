/*
 * utility.h
 *
 * Created: 3/12/2018 1:08:00 PM
 *  Author: teddy
 */ 

#pragma once

#include <avr/io.h>

namespace hw {
	namespace PINPOS {
		enum e {
			LED_RED = 4,
			LED_GREEN = 5,
		};
	}
	namespace PINMASK {
		enum e {
			LED_RED = 1 << PINPOS::LED_RED,
			LED_GREEN = 1 << PINPOS::LED_GREEN,
		};
	}
	//Turn both LEDs on and break into debugger
	inline void panic() {
		PORTA.DIRSET = PINMASK::LED_RED | PINMASK::LED_GREEN;
		PORTA.OUTSET = PINMASK::LED_RED | PINMASK::LED_GREEN;
		asm("BREAK");
		while(true);
	}
}
