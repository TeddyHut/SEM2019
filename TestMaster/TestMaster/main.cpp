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
#include "runtime/module.h"

int main(void)
{
	////Disable CCP protection (4 instructions)
	CCP = CCP_IOREG_gc;
	////Enable prescaler and set to 16, therefore a 1MHz CPU clock
	CLKCTRL.MCLKCTRLB = 0;
	//CLKCTRL.MCLKCTRLB = /*CLKCTRL_PDIV_16X_gc |*/ CLKCTRL_PEN_bm;

	//Turn on pullups for TWI pins
	PORTB.PIN0CTRL |= PORT_PULLUPEN_bm;
	PORTB.PIN1CTRL |= PORT_PULLUPEN_bm;

	hw::start_timer_daemons<1000>();

	hw::LED led_red(hw::PINPORT::LED_RED, hw::PINPOS::LED_RED);
	hw::LED led_green(hw::PINPORT::LED_GREEN, hw::PINPOS::LED_GREEN);
	
	hw::Button button_test(hw::PINPORT::BUTTON_TEST, hw::PINPOS::BUTTON_TEST);
	hw::Button button_horn(hw::PINPORT::BUTTON_HORN, hw::PINPOS::BUTTON_HORN);

	hw::ButtonTimer<Stopwatch1k> buttontimer_horn(button_horn);

	enum class State {
		None,
		Scan,
		Test,
	} programstate = State::Scan, previousstate = State::None;

	enum class Mode {
		Horn = 1,
		Motor = 2,
		_size,
		
	} modulemode = Mode::Horn;

	hw::Blinker::Pattern pattern;
	pattern.ontime = 100;
	pattern.offtime = 250;
	pattern.resttime = 3000;
	pattern.repeat = true;
	pattern.count = static_cast<uint8_t>(modulemode);

	rt::twi::ModuleScanner modulescanner(hw::inst::twiMaster0);
	rt::module::Master *currentmodule = nullptr;

	//Set TWI bus state to idle
	TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;

	//Enable interrupts
	sei();


	while(true) {
		//---If the program state needs to change---
		if(programstate != previousstate) {
			switch(programstate) {

			case State::Scan:
				//If a module needs to be deallocated
				if(currentmodule != nullptr) {
					currentmodule->~Master();
					free(currentmodule);
					currentmodule = nullptr;
				}
				pattern.inverted = true;
				led_red.blinkPattern(pattern);
				modulescanner.scan(1, 127, false);
				break;

			case State::Test:
				//Allocate memory
				switch(modulemode) {
				case Mode::Horn:
					currentmodule = new rt::module::Horn(hw::inst::twiMaster0, modulescanner.foundModule().addr);
					if(currentmodule == nullptr) hw::panic();
					break;
				}
			}

			previousstate = programstate;
		}

		//---Normal program---
		switch(programstate) {

		case State::Scan:
		{
			modulescanner.update();
			volatile auto val = modulescanner.found();
			if(modulescanner.found() & 0x80) {
				programstate = State::Test;
			}
			break;
		}

		case State::Test:
			if(currentmodule == nullptr) break;
			currentmodule->update();
			//If the connection was lost, go back to scanning
			if(currentmodule->buffermanager.m_consecutiveCycleErrors > 15) {
				programstate = State::Scan;
				break;
			}
			switch(modulemode) {
			case Mode::Horn:
				auto hornmodule = static_cast<rt::module::Horn *>(currentmodule);
				hornmodule->set_state(button_horn.held);
				led_green.setState(button_horn.held);
				break;
			}
			break;
		}

		//Mode change (number of LED blinks represents mode)
		if(buttontimer_horn.pressedFor(250)) {
			modulemode = static_cast<Mode>(static_cast<uint8_t>(modulemode) + 1);
			if(modulemode == Mode::_size)
				modulemode = Mode::Horn;
			pattern.count = static_cast<uint8_t>(modulemode);
			led_red.blinkPattern(pattern);
			programstate = State::Scan;
		}

		led_red.update();
		led_green.update();
		button_test.update();
		button_horn.update();
		buttontimer_horn.update();
	}
}
