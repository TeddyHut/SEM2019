/*
 * BMS50A.cpp
 *
 * Created: 12/04/2019 12:49:05 PM
 * Author : teddy
 */ 

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <libmodule.h>
#include <generalhardware.h>
#include "segfont.h"
#include "sensors.h"
#include "bmsui.h"
#include "config.h"
#include "extrahardware.h"

//From https://stackoverflow.com/questions/23377948/arduino-avr-atmega-microcontroller-random-resets-jumps-or-variable-data-corrup
//TODO: Understand how this works.
extern void *_end, *__stack;
#define __ALD(x) ((uintptr_t)(x) - ((uintptr_t)(x) & 0x03))
#define __ALU(x)   ((uintptr_t)(x) + ((uintptr_t)(x) & 0x03))
void _stackfill(void) __attribute__((naked)) __attribute__((optimize("O3"))) __attribute__((section (".init1")));
void _stackfill(void)
{
    uint32_t* start = (uint32_t*)__ALU(&_end);
    uint32_t* end   = (uint32_t*)__ALD(&__stack);

    for (uint32_t *pos = start; pos < end; pos++)
        *pos = 0x41424142; // ends up as endless ascii BABA
}

libmodule::userio::ic_ldt_2601g_11_fontdata::Font segfont::english_font = {&(english_serial::pgm_arr[0]), english_len};
extrahardware::SegDisplay segs;

void libmodule::hw::panic() {
	segs.write_characters("Pn", 2, libmodule::userio::IC_LTD_2601G_11::OVERWRITE_LEFT | libmodule::userio::IC_LTD_2601G_11::OVERWRITE_RIGHT);
	while(true);
}

ISR(BADISR_vect) {
	libmodule::hw::panic();
}

/* Todo
 * Make it so that CPU sleeps when not being used (instead of just looping pointlessly)
 * Re-add watchdog
 * Add expectation checking
 * Change shift registers from being software based to hardware/serial based
 * Add power down mode that is auto-activated when cell0 < 3.0V, and an option for leaving the batteries plugged in
 * Add settings menu
 * Add statistics subsystem -> averaging, highest draw/values, etc.
 * Add communications subsystem that uses statistics
 * Buzzer support for new PCB, better current sensing
 */

/* Hardware allocations
 * RTC - Software Timers
 * ADC0 - ADCManager
 */


int main(void)
	{
    //Set clock prescaler to 2 (should therefore run at 8MHz)
	CCP = CCP_IOREG_gc;
	CLKCTRL.MCLKCTRLB = CLKCTRL_PEN_bm;

	//Clear reset flag register
	RSTCTRL.RSTFR = 0x3f;

	//Set watchdog to 0.128s
	CCP = CCP_IOREG_gc;
	WDT.CTRLA = WDT_PERIOD_128CLK_gc;

	//Enable idle sleep mode
	SLPCTRL.CTRLA = SLPCTRL_SMODE_IDLE_gc | SLPCTRL_SEN_bm;

	segs.set_font(segfont::english_font);

	libmodule::ui::segdpad::Common ui_common{segs};

	//Setup Dpad
	libmicavr::PortIn input_dpad_common(PORTA, 1);
	libmicavr::PortOut output_dpad_mux_s0(PORTA, 2);
	libmicavr::PortOut output_dpad_mux_s1(PORTA, 3);

	libmodule::userio::BinaryOutput<uint8_t, 2> binaryoutput_mux_s;
	binaryoutput_mux_s.set_digiout_bit(0, &output_dpad_mux_s0);
	binaryoutput_mux_s.set_digiout_bit(1, &output_dpad_mux_s1);

	libmodule::userio::MultiplexDigitalInput<libmodule::userio::BinaryOutput<uint8_t, 2>> mdi_dpad_left(&input_dpad_common, &binaryoutput_mux_s, 0);
	libmodule::userio::MultiplexDigitalInput<libmodule::userio::BinaryOutput<uint8_t, 2>> mdi_dpad_up(&input_dpad_common, &binaryoutput_mux_s, 1);
	libmodule::userio::MultiplexDigitalInput<libmodule::userio::BinaryOutput<uint8_t, 2>> mdi_dpad_centre(&input_dpad_common, &binaryoutput_mux_s, 2);
	libmodule::userio::MultiplexDigitalInput<libmodule::userio::BinaryOutput<uint8_t, 2>> mdi_dpad_down(&input_dpad_common, &binaryoutput_mux_s, 3);
	libmicavr::PortIn input_dpad_right(PORTA, 0);

	ui_common.dpad.left.set_input(&mdi_dpad_left);
	ui_common.dpad.up.set_input(&mdi_dpad_up);
	ui_common.dpad.centre.set_input(&mdi_dpad_centre);
	ui_common.dpad.down.set_input(&mdi_dpad_down);
	ui_common.dpad.right.set_input(&input_dpad_right);

	libmodule::userio::RapidInput2L1k::Level rapidinput_level_0 = {500, 250};
	libmodule::userio::RapidInput2L1k::Level rapidinput_level_1 = {1500, 100};
	ui_common.dpad.set_rapidInputLevel(0, rapidinput_level_0);
	ui_common.dpad.set_rapidInputLevel(1, rapidinput_level_1);


	ui_common.dp_right_blinker.pm_out = ui_common.segs.get_output_dp_right();

	//Setup sensors, printers, and statdisplays
	bms::snc::setup();
	ui::printer::setup();
	ui::statdisplay::setup();

	//Setup BMS
	libmicavr::PortOut output_relay_left(PORTF, 0);
	libmicavr::PortOut output_relay_right(PORTF, 1);
	bms::BMS sys_bms;
	sys_bms.set_digiout_relayLeft(&output_relay_left);
	sys_bms.set_digiout_relayRight(&output_relay_right);
	ui::bms_ptr = &sys_bms;

	//Setup refresh timer
	libmodule::Timer1k timer;
	//timer = config::ticks_main_system_refresh;
	timer.finished = true;
	timer.start();
	
	ui::Main ui_main(&ui_common);
	
	//Start timer daemons and enable interrupts
	libmodule::time::start_timer_daemons<1000>();
	
	sei();
	while(true) {
		if(timer) {
			wdt_reset();
			timer = config::ticks_main_system_refresh;

			//Update common UI elements
			ui_common.dpad.left.update();
			ui_common.dpad.right.update();
			ui_common.dpad.up.update();
			ui_common.dpad.down.update();
			ui_common.dpad.centre.update();
			//Read sensor values for this frame
			bms::snc::cycle_read();

			ui_main.ui_management_update();
			ui_common.dp_right_blinker.update();
			
			sys_bms.update();

			libmicavr::ADCManager::next_cycle();
		}

		//Go to sleep between cycles (timer (PIT) interrupt should wake up)
		//Depending on state of USART, will either go to power down or to idle mode (see extrahardware.cpp)
		sleep_cpu();
	}
}
