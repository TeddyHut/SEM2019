/*
 * TestMaster.cpp
 *
 * Created: 27/11/2018 10:20:32 AM
 * Author : teddy
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <board_horn.h>
#include <libmodule/timer.h>
#include <libmodule/userio.h>
#include "runtime/module.h"

using sample_t = uint16_t;

int main(void)
{
	//Disable CCP protection (4 instructions)
	CCP = CCP_IOREG_gc;
	////Enable prescaler and set to 16, therefore a 1MHz CPU clock
	CLKCTRL.MCLKCTRLB = 0;
	//CLKCTRL.MCLKCTRLB = /*CLKCTRL_PDIV_16X_gc |*/ CLKCTRL_PEN_bm;

	//Turn on pullups for TWI pins
	PORTB.PIN0CTRL |= PORT_PULLUPEN_bm;
	PORTB.PIN1CTRL |= PORT_PULLUPEN_bm;

	libmodule::time::start_timer_daemons<1000>();

	libtiny816::LED led_red_inst(libtiny816::hw::PINPORT::LED_RED, libtiny816::hw::PINPOS::LED_RED);
	libmodule::userio::BlinkerTimer1k led_red(&led_red_inst);
	libtiny816::LED led_green_inst(libtiny816::hw::PINPORT::LED_GREEN, libtiny816::hw::PINPOS::LED_GREEN);

	libtiny816::Button button_test_inst(libtiny816::hw::PINPORT::BUTTON_TEST, libtiny816::hw::PINPOS::BUTTON_TEST);
	libtiny816::Button button_horn_inst(libtiny816::hw::PINPORT::BUTTON_HORN, libtiny816::hw::PINPOS::BUTTON_HORN);


	libmodule::userio::ButtonTimer1k buttontimer_horn(&button_horn_inst);

	enum class State {
		None,
		Scan,
		Test,
	} programstate = State::Scan, previousstate = State::None;

	enum class Mode {
		Horn = 1,
		SpeedMonitor = 2,
		_size,
		
	} modulemode = Mode::SpeedMonitor;

	libmodule::userio::Blinker::Pattern pattern;
	pattern.ontime = 100;
	pattern.offtime = 250;
	pattern.resttime = 3000;
	pattern.repeat = true;
	pattern.count = static_cast<uint8_t>(modulemode);

	rt::twi::ModuleScanner modulescanner(hw::inst::twiMaster0);
	rt::module::Master *currentmodule = nullptr;
		
	libmodule::Stopwatch1k stopwatch;
	uint8_t previoussamplepos = 0;

	//Set TWI bus state to idle
	TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;

	//Enable interrupts
	sei();
	//_delay_ms(5);

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
				led_red.run_pattern(pattern);
				modulescanner.scan(1, 127, false);
				break;

			case State::Test:
				//Update pattern
				pattern.inverted = false;
				led_red.run_pattern(pattern);
				//Allocate memory
				switch(modulemode) {
				case Mode::Horn:
					currentmodule = new rt::module::Horn(hw::inst::twiMaster0, modulescanner.foundModule().addr);
					break;
				case Mode::SpeedMonitor:
					currentmodule = new rt::module::SpeedMonitorManager<sample_t>(hw::inst::twiMaster0, modulescanner.foundModule().addr);
					break;
				}
				if(currentmodule == nullptr) libmodule::hw::panic();
				break;
			}

			previousstate = programstate;
		}

		//---Normal program---
		switch(programstate) {

		case State::Scan:
		{
			modulescanner.update();
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
			//Set the module LED based on the horn button
			currentmodule->set_led(button_horn_inst.get());
			switch(modulemode) {
			case Mode::Horn: {
				auto hornmodule = static_cast<rt::module::Horn *>(currentmodule);
				hornmodule->set_state(button_test_inst.get());
				led_green_inst.set(button_test_inst.get());
				}
				break;
			case Mode::SpeedMonitor: {
				auto monitor = static_cast<rt::module::SpeedMonitorManager<sample_t> *>(currentmodule);
				monitor->update();

				if(!monitor->m_ready) break;

				if(monitor->get_sample_pos(0) != previoussamplepos) {
					stopwatch.ticks = 0;
					stopwatch.start();
					previoussamplepos = monitor->get_sample_pos(0);
				}

				uint32_t total = 0;
				uint8_t valid_samples = 0;
				for(uint8_t i = 0; i < monitor->m_samplecount + 1; i++) {
					uint16_t sample;
					if(i == monitor->m_samplecount) {
						sample = stopwatch.ticks;
						if(sample <= total / valid_samples) sample = 0;
					}
					else sample = monitor->get_sample(0, i);
					if(sample > 0) {
						total += sample;
						valid_samples++;
					}
				}
				//Not allowed to divide by 0
				if(valid_samples != 0) {
					//Set the LED to whether the average reaches the trigger
					led_green_inst.set(total / valid_samples <= monitor->get_rps_constant(0));
				}
				else led_green_inst.set(false);

				}
				break;
			}
			break;
		}

		//---Mode change--- (number of LED blinks represents mode)
		//Puts the program back into the scan state
		if(buttontimer_horn.pressedFor(1500)) {
			modulemode = static_cast<Mode>(static_cast<uint8_t>(modulemode) + 1);
			if(modulemode == Mode::_size)
				modulemode = Mode::Horn;
			pattern.count = static_cast<uint8_t>(modulemode);
			led_red.run_pattern(pattern);
			programstate = State::Scan;
		}

		led_red.update();
		buttontimer_horn.update();
	}
}
