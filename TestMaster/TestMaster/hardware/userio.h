/*
 * userio.h
 *
 * Created: 3/12/2018 3:58:14 PM
 *  Author: teddy
 */ 

#pragma once

#include <avr/io.h>

#include "timer.h"

namespace hw {
	class GPIOOut {
		//Sets the state of the pin
		virtual void setState(bool const state) = 0;
	};
	class Blinker {
	public:
		struct Pattern {
			uint16_t ontime;
			uint16_t offtime;
			uint16_t resttime;
			uint8_t count;
			bool repeat : 1;
			bool inverted : 1;
		};

		//Run the blink pattern specified
		virtual void blinkPattern(Pattern const &pattern) = 0;
		//Get the current pattern
		virtual Pattern currentPattern() const = 0;
	};
	class LED : public GPIOOut, public Blinker {
	public:
		enum class Mode : uint8_t {
			Solid,
			Blink,
		};
		void setState(bool const state) override;
		void blinkPattern(Pattern const &pattern) override;
		Pattern currentPattern() const override;
		Mode currentMode() const;
		//Call once per frame (used for blinker)
		void update();

		//Pin refers to the bit position, not the mask
		LED(PORT_t &port, uint8_t const pin);
	private:
		//Having this generates a warning because --pack-structs is specified globally
		PORT_t &pm_hwport;
		uint8_t const pm_hwpin;
		Pattern pm_pattern;
		Mode pm_mode = Mode::Solid;
		Timer1k pm_timer;
		struct {
			uint8_t count;
			//State could just be calculated from count 
			bool state : 1;
		} pm_patternstate;

		//Does not change the mode
		void transparent_setState(bool const state);
	};
}
