/*
 * BMS50A.cpp
 *
 * Created: 12/04/2019 12:49:05 PM
 * Author : teddy
 */ 

#include <avr/io.h>
#include <avr/wdt.h>
#include <libmodule.h>
#include <generalhardware.h>
#include "segfont.h"
#include "sensors.h"
#include "bmsui.h"
#include "config.h"

void libmodule::hw::panic() { while(true); }

libmodule::userio::ic_ldt_2601g_11_fontdata::Font segfont::english_font = {&(english_serial::pgm_arr[0]), english_len};

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

int main(void)
{
    //Set clock prescaler to 2 (should therefore run at 8MHz)
	CCP = CCP_IOREG_gc;
	CLKCTRL.MCLKCTRLB = CLKCTRL_PEN_bm;

	//Set watchdog to 0.032s
	//WDT.CTRLA = WDT_PERIOD_32CLK_gc;

	libmodule::ui::segdpad::Common ui_common;

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

	//Setup display (inverted because of op-amps)
	libmicavr::PortOut output_segs_ser(PORTC, 0, true);
	libmicavr::PortOut output_segs_rclk(PORTC, 1, true);
	libmicavr::PortOut output_segs_srclk(PORTC, 2, true);
	libmicavr::PortOut output_segs_select(PORTC, 3, true);
	//Ensures that the first digit is selected (since the toggle command is used)
	output_segs_select.set(true);

	libmodule::userio::IC_74HC595 segs_shiftreg;
	segs_shiftreg.set_digiout_data(&output_segs_ser);
	segs_shiftreg.set_digiout_clk(&output_segs_srclk);
	segs_shiftreg.set_digiout_latch(&output_segs_rclk);
	
	ui_common.segs.set_digiout_anode(0, &output_segs_select);
	ui_common.segs.set_74hc595(&segs_shiftreg);
	ui_common.segs.set_font(segfont::english_font);
	ui_common.segs.set_pwminterval(1000 / 120);

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
	timer = config::ticks_main_system_refresh;
	timer.start();
	
	//Start timer daemons and enable interrupts
	libmodule::time::start_timer_daemons<1000>();
	sei();
	
	
	//libmodule::Timer1k timer_relay_test;
	//timer_relay_test = 250;
	//timer_relay_test.start();
	////Left true and right false turns it off
	//output_relay_left.set(true);
	//output_relay_right.set(false);


	//libmodule::Timer1k caltimer;
	//caltimer = 250;
	//caltimer.start();
	


	//libmodule::ui::segdpad::List ui_list;
	//ui_list.ui_common = &ui_common;
	//ui_list.m_items.resize(6 + 5);
	//for(uint8_t i = 0; i < 6; i++)  ui_list.m_items[i] = ui::statdisplay::cellvoltage[i];
	//ui_list.m_items[6]  = ui::statdisplay::averagecellvoltage;
	//ui_list.m_items[7]  = ui::statdisplay::batteryvoltage;
	//ui_list.m_items[8]  = ui::statdisplay::batterypresent;
	//ui_list.m_items[9]  = ui::statdisplay::temperature;
	//ui_list.m_items[10] = ui::statdisplay::current;

	ui::Main ui_main(&ui_common);

	while(true) {
		//wdt_reset();
		if(timer) {
			timer = config::ticks_main_system_refresh;
			timer.start();
			ui_common.dpad.left.update();
			ui_common.dpad.right.update();
			ui_common.dpad.up.update();
			ui_common.dpad.down.update();
			ui_common.dpad.centre.update();
			//Read sensor values for this frame
			bms::snc::cycle_read();

			ui_main.ui_management_update();
			//ui_list.ui_management_update();
			
			sys_bms.update();

			libmicavr::ADCManager::next_cycle();
		}
		//if(caltimer) {
		//	bms::snc::current1A->calibrate();
		//	caltimer.reset();
		//}
		ui_common.dp_right_blinker.update();
		ui_common.segs.update();
	}
}
