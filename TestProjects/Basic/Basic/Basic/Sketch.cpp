#include <Arduino.h>
#include <libmodule.h>
#include <libarduino_m328.h>

using namespace libmodule;

module::Client client;
module::Horn slave(libarduino_m328::twiSlave0); //I use horn here because it's the most simple one

libarduino_m328::DigitalOut led_red(LED_BUILTIN);
libarduino_m328::DigitalIn button_test(9);
libarduino_m328::DigitalIn switch_mode(6);

ClientMode previousmode = ClientMode::None;

void setup() {
	time::start_timer_daemons<1000>();
	
	slave.set_timeout(1000);
	slave.set_twiaddr(0x01);
	slave.set_signature(0x10);
	slave.set_id(0);
	slave.set_name("Basic");
	slave.set_operational(true);

	client.register_module(&slave);
	client.register_output_led_red(&led_red);
	client.register_input_button_test(&button_test);
	client.register_input_switch_mode(&switch_mode);
}

void loop() {
	client.update();

	ClientMode currentmode = client.get_mode();

	if(currentmode != previousmode) {
		if(currentmode == ClientMode::Manual) {
			//Place manual mode initialization code here
		}
		else if(currentmode == ClientMode::Connected) {
			//Place connected mode initialization code here
		}
		else if(currentmode == ClientMode::Test) {
			//Place test mode initialization code here
		}
		previousmode = currentmode;
	}

	if(currentmode == ClientMode::Manual) {
		//Place manual mode code here
	}
	else if(currentmode == ClientMode::Connected) {
		//Place connected mode code here
	}
	else if(currentmode == ClientMode::Test) {
		//Place test mode code here

		//To blink for the next test, call client.test_blink();
		//To exit test mode, call client.test_finish();
	}
}
