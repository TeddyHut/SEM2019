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

namespace metadata {
	namespace key {
		enum e {
			Horn = 0,
			SpeedMonitor,
			MotorMover,
			MotorController,
			None,
		};
	}
	constexpr uint8_t signatureKey[][2] = {
		{ 0x10, 0x28 }, //Horn
		{ 0x28, 0x30 }, //SpeedMonitor
		{ 0x30, 0x38 }, //MotorMover
		{ 0x38, 0x40 }  //MotorController
	};
}

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

	enum class State {
		None,
		Scan,
		Test,
	} programstate = State::Scan, previousstate = State::None;

	metadata::key::e modulemode;

	libmodule::userio::Blinker::Pattern pattern;
	pattern.ontime = 100;
	pattern.offtime = 250;
	pattern.resttime = 3000;
	pattern.repeat = true;
	pattern.count = 3;

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
			
			case State::Test: {
				//Determine the type of module
				auto signature = modulescanner.foundModule().signature;

				for(modulemode = metadata::key::Horn; modulemode < metadata::key::None; modulemode = static_cast<metadata::key::e>(modulemode + 1)) {
					if(signature >= metadata::signatureKey[modulemode][0] && signature < metadata::signatureKey[modulemode][1])
						break;
				}

				//Allocate memory
				switch(modulemode) {
				default:
				case metadata::key::MotorMover:
				case metadata::key::MotorController:
					led_green_inst.set(true);
					//[[fallthrough]];
				case metadata::key::Horn:
					currentmodule = new rt::module::Horn(hw::inst::twiMaster0, modulescanner.foundModule().addr);
					break;
				case metadata::key::SpeedMonitor:
					currentmodule = new rt::module::SpeedMonitorManager<sample_t>(hw::inst::twiMaster0, modulescanner.foundModule().addr);
					break;
				}
				if(currentmodule == nullptr) libmodule::hw::panic();
				//Update pattern based on module
				pattern.count = modulemode + 1;
				pattern.inverted = false;
				led_red.run_pattern(pattern);
				break;
				}
			case State::Scan:
				//If a module needs to be deallocated
				if(currentmodule != nullptr) {
					currentmodule->~Master();
					free(currentmodule);
					currentmodule = nullptr;
				}
				pattern.count = metadata::key::None + 1;
				pattern.inverted = true;
				led_red.run_pattern(pattern);
				led_green_inst.set(false);
				modulescanner.scan(1, 127, false);
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
			currentmodule->update();
			//If the connection was lost, go back to scanning
			if(currentmodule->buffermanager.m_consecutiveCycleErrors > 15) {
				programstate = State::Scan;
				break;
			}
			//Set the module LED based on the horn button
			currentmodule->set_led(button_horn_inst.get());

			switch(modulemode) {
			default:
				break;
			case metadata::key::Horn: {
				auto hornmodule = static_cast<rt::module::Horn *>(currentmodule);
				hornmodule->set_state(button_test_inst.get());
				led_green_inst.set(button_test_inst.get());
				}
				break;
			case metadata::key::SpeedMonitor: {
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

		led_red.update();
	}
}
