/*
 * extrahardware.h
 *
 * Created: 16/07/2019 2:57:02 PM
 *  Author: teddy
 */ 

#pragma once

#include <avr/io.h>
#include <libmodule.h>

namespace extrahardware {
	class SegDisplay : public libmodule::userio::IC_LTD_2601G_11 {
	public:
		static SegDisplay *currentinstance;
		
		void handle_isr_tx();
		void handle_isr_tcb();

		SegDisplay();
	private:
		static constexpr uint32_t config_baudrate = 10000;
		static constexpr uint32_t config_frequency_digitswitch = 120;
		bool pushed_off = true;
		uint8_t nextdigit = 0;
	};
}
