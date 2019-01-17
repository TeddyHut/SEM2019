/*
 * Horn.cpp
 *
 * Created: 8/01/2019 9:46:39 PM
 * Author : teddy
 */ 

#include <avr/io.h>

#include <libtiny816/board_horn.h>
#include <libmodule/libmodule.h>
#include "hardware/twi.h"

int main(void)
{
    //Disable CCP protection (4 instructions)
    CCP = CCP_IOREG_gc;
    ////Enable prescaler and set to 16, therefore a 1MHz CPU clock
    CLKCTRL.MCLKCTRLB = 0;
    //CLKCTRL.MCLKCTRLB = /*CLKCTRL_PDIV_16X_gc |*/ CLKCTRL_PEN_bm;

    libmodule::time::start_timer_daemons<1000>();

    libtiny816::LED led_red(libtiny816::hw::PINPORT::LED_RED, libtiny816::hw::PINPOS::LED_RED);
    libtiny816::Button button_test(libtiny816::hw::PINPORT::BUTTON_TEST, libtiny816::hw::PINPOS::BUTTON_TEST);
	libtiny816::Button button_mode(libtiny816::hw::PINPORT::SW_MODE, libtiny816::hw::PINPOS::SW_MODE);
    
    libmodule::module::Horn horn(hw::inst::twiSlave0);
	libmodule::module::Client client;
	client.register_module(&horn);
	client.register_input_button_test(&button_test);
	client.register_input_switch_mode(&button_mode);
	client.register_output_led_red(&led_red);
	
	horn.set_timeout(1000);
	horn.set_twiaddr(0x02);
	horn.set_signature(0xAA);
	horn.set_id(01);
	horn.set_name("Horn");
	horn.set_operational(true);


	libtiny816::LED led_green_inst(libtiny816::hw::PINPORT::LED_GREEN, libtiny816::hw::PINPOS::LED_GREEN);
	libtiny816::LED out_horn_inst(libtiny816::hw::PINPORT::OUT_HORN, libtiny816::hw::PINPOS::OUT_HORN);
	libmodule::userio::BlinkerTimer1k led_green(&led_green_inst);
	libmodule::userio::BlinkerTimer1k out_horn(&out_horn_inst);
    
    libtiny816::Button button_horn_inst(libtiny816::hw::PINPORT::BUTTON_HORN, libtiny816::hw::PINPOS::BUTTON_HORN);
    libtiny816::Button button_wheel_inst(libtiny816::hw::PINPORT::BUTTON_WHEEL, libtiny816::hw::PINPOS::BUTTON_WHEEL);
	libmodule::utility::InStates<bool> button_horn(&button_horn_inst);
	libmodule::utility::InStates<bool> button_wheel(&button_wheel_inst);

    //Enable interrupts
    sei();
	auto previousmode = client.get_mode();
	while(true) {
		client.update();
		button_horn.update();
		button_wheel.update();

		auto currentmode = client.get_mode();
		if(currentmode == libmodule::module::Client::Mode::Test) {
			if(currentmode != previousmode) {
				libmodule::userio::BlinkerTimer1k::Pattern pattern;
				pattern.inverted = false;
				pattern.repeat = false;
				pattern.count = 2;
				pattern.ontime = 500;
				pattern.offtime = 500;
				pattern.resttime = 0;
				out_horn.run_pattern(pattern);
				led_green.run_pattern(pattern);
			}
			//If test finishes, notify client
			if(out_horn.currentMode() == libmodule::userio::BlinkerTimer1k::Mode::Solid)
				client.test_finish();
		}
		if(currentmode == libmodule::module::Client::Mode::Manual) {
			out_horn.set_state(button_horn.held || button_wheel.held);
			led_green.set_state(button_horn.held || button_wheel.held);
		}
		if(currentmode == libmodule::module::Client::Mode::Connected) {
			out_horn.set_state(horn.get_state_horn());
			led_green.set_state(horn.get_state_horn());
		}
		previousmode = currentmode;
		
		led_green.update();
		out_horn.update();
	}
}

