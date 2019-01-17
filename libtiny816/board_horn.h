/*
 * board_horn.h
 *
 * Created: 16/01/2019 1:08:00 PM
 *  Author: teddy
 */ 

#pragma once

#include <avr/io.h>
#include <libmodule/utility.h>

namespace libmodule {
	namespace hw {
		//Turn both LEDs on and break into debugger
		void panic();
	}
}

namespace libtiny816 {
	class LED : public libmodule::utility::Output<bool> {
	public:
		void set(bool const state) override;
		//Pin refers to the bit position, not the mask
		LED(PORT_t &port, uint8_t const pin);
	private:
		//Having this generates a warning because --pack-structs is specified globally
		PORT_t &pm_hwport;
		uint8_t const pm_hwpin;
	};
	class Button : public libmodule::utility::Input<bool> {
	public:
		bool get() const override;
		Button(PORT_t &port, uint8_t pin, bool const onlevel = false, bool const pullup = true);
	private:
		PORT_t &pm_hwport;
		uint8_t const pm_hwpin;
	};
	namespace hw {
		namespace PINPORT {
			extern PORT_t &LED_RED;
			extern PORT_t &LED_GREEN;
			extern PORT_t &BUTTON_TEST;
			extern PORT_t &BUTTON_HORN;
			extern PORT_t &BUTTON_WHEEL;
			extern PORT_t &SW_MODE;
			extern PORT_t &OUT_HORN;
		}
		namespace PINPOS {
			enum e {
				LED_RED = 4,
				LED_GREEN = 5,
				BUTTON_TEST = 6,
				BUTTON_HORN = 2,
				BUTTON_WHEEL = 3,
				SW_MODE = 7,
				OUT_HORN = 4,
			};
		}
		namespace PINMASK {
			enum e {
				LED_RED = 1 << PINPOS::LED_RED,
				LED_GREEN = 1 << PINPOS::LED_GREEN,
				BUTTON_TEST = 1 << PINPOS::BUTTON_TEST,
				BUTTON_HORN = 1 << PINPOS::BUTTON_HORN,
				BUTTON_WHEEL = 1 << PINPOS::BUTTON_WHEEL,
				SW_MODE = 1 << PINPOS::SW_MODE,
				OUT_HORN = 1 << PINPOS::OUT_HORN,
			};
		}
	}
}