#include <Arduino.h>
#include <libmodule.h>
#include <libarduino_m328.h>

/* This project is called MotorMover Test
 * It requires the following hardware:
 *	- Arduino running using an ATmega328 or similar
 *	- An LED connected to a digital pin called "led_red"
 *	- An LED connected to a digital pin called "led_green"
 *	- A button connected to a digital pin called "button_test"
 *	- A button connected to a digital pin called "button_engage"
 *	- A button connected to a digital pin called "switch_mode" (optional)
 *
 * This program is a test for the MotorMover communication.
 * It functions as follows:
 * Connected Mode:
 *		In connected mode, the module will be controlled by the master.
 *		When engaged, led_green will turn on
 *		If the master is not connected, or the "switch_mode" is held, the module will go into manual mode.
 * Manual Mode:
 *		If button_power is pressed, led_green will toggle
 * Test Mode:
 *		To enter test mode, hold down button_test for 1 second.
 *		To exit test mode, press button_test again (exits on release of second press)
 *		In test mode, led_green will turn on for 1 second, then turn off
 */

/* All the variables/functions found in the "libmodule" library are enclosed in a 'namespace'. Consider it like a container that holds everything in libmodule.
 * To access symbols inside a namespace, you use the "scope operator" '::'.
 * For example, to access the symbol 'Timer1k' inside libmodule, you would use the syntax "libmodule::Timer1k"
 * Without the scope operator, the compiler would not be able to find the symbol 'Timer1k', and there would be an error.
 * Since prefixing everything with 'libmodule' would be a bit tiresome, we can introduce all symbols from 'libmodule' into this program.
 * This is done by "using namespace libmodule;" Now, instead of "libmodule::Timer1k", we can just type "Timer1k"
 */

using namespace libmodule;

/* It is important to note that you can have namespaces within namespaces. libmodule has the following namespaces:
 * libmodule - Everything is in here
 *		utility - Contains useful functions/objects used throughout libmodule
 *		hw - Some hardware specific things
 *		twi - Contains TWI communication/buffer management code
 *		time - Contains software timers/stopwatches
 *		userio - Input/Output/More specific utility functions (e.g. Blinker, ButtonTimer)
 *		module - Contains modules (SpeedMonitor, MotorController, etc)
 *
 * So, to access the module "MotorController", you would usually need to use the following syntax: "libmodule::module::MotorController"
 * However, the symbols from the namespace 'libmodule' have been introduced above, we can just type "module::MotorController"
 */


/* Create a module Client object called 'client' (note the lowercase on the instance).
 *
 * Just like you can create a variable as a number (e.g. int x;), C/C++ allow us to define custom types with custom data and functions.
 * Inside libmodule, in the namespace 'module', there is a custom type called "Client".
 * So, just like you could write "int x;" to create an int called 'x', we can write "Client x;" to create a Client object called 'x'.
 *
 * Client has the following functions available:
 *		void test_blink();
 *		void test_finish();
 *
 *		void register_module(module_ptr);
 *		void register_input_button_test(input_ptr);
 *		void register_input_switch_mode(input_ptr);
 *		void register_output_led_red(output_ptr);
 *
 *		Mode get_mode();
 */

//The Client is created here. Note the usage of the scope resolution operator "::" to access the module namespace in libmodule.
module::Client client;

/* Create a MotorMover module object called 'mover'.
 * The mover has the following functions available:
 *		void update();
 *		void set_timeout(timeout);
 *		void set_twiaddr(addr);
 *		void set_signature(signature);
 *		void set_id(id);
 *		void set_name(name);
 *		void set_operational(state);
 *		bool get_led();
 *		bool get_power();
 *		bool connected();
 *
 *		void set_position_engaged(pos);
 *		void set_position_disengaged(pos);
 *		void set_engaged(state);
 *		MotorMoverMode get_mode();
 *		bool get_binary_engaged();
 *		uint16_t get_continuous_position();
 *		bool get_mechanism_powered();
 *
 * This object requires extra information on instantiation (creation).
 * Since MotorMover handles communication with the master, it needs to be given an TWI (I2C) interface it can use to do so.
 * This allows the same code to be used on different hardware with different I2C interfaces.
 * Since this I2C interface is hardware specific, it can be found in the libarduino_m328 library (which also has its own namespace).
 * The syntax is the following:
 *	MotorMover [name](TWI Hardware Interface);
 *
 * Inside libarduino_m328 there is an interface called twiSlave0, which we pass to Manager
 * While the module object (e.g. mover) handles communication with the master, it does -not- handle changing to different modes, managing LEDs, etc.
 * That is done by the "client" object.
 * In summary:
 *		MotorMover: Allocates memory/handles communication of the information with the Master
 *		Client: Handles the 'user interface' of the module. e.g. Blinking of LEDs, reading buttons, determining modes based on the environment
 */

module::MotorMover mover(libarduino_m328::twiSlave0);

/* As part of the module specification in OneNote, each module should have:
 *	- A red LED
 *	- A pushbutton called "Test"
 *	- A switch called "Mode"
 *
 * Since "Client" implements these aspects of a module, it needs information about how to access these interface devices.
 * To give it this information, we need to create other objects that are compatible with client that give it this information.
 *
 * Since this information is hardware dependent, it can be found in the libarduino_m328 library.
 */

//This creates a digital out object. We need to give it a pin number when creating it. In my case, the red led is on pin 7.
//Note that DigitaOut will take care of setting pinMode
libarduino_m328::DigitalOut led_red(7);

/* This creates a digital in object. It has the following syntax:
 *	DigitalIn [name](pin, pinmode, onlevel);
 *		pin - The pin number of the input
 *		pinmode (optional) - The pinmode. Defaults to INPUT_PULLUP
 *		onlevel (optional) - The level considered on. For example, if when a button is pushed, it pulls the pin low, then the "onlevel" is false (or 0). Defaults to false
 */

libarduino_m328::DigitalIn button_test(9);
//libarduino_m328::DigitalIn button_test(9, INPUT_PULLUP, false);
//^^^ This is the same thing, but without the default parameters.

libarduino_m328::DigitalIn switch_mode(6);

void setup() {
	//This starts the software timer service in libmodule. The angled bracket parameter indicates when timer frequencies to start.
	//Only 1000Hz (1ms) timers are implemented.
	time::start_timer_daemons<1000>();

	/* This is where we set parameters for the module.
	 *	timeout is the amount of time that the module should wait after receiving no communication from the master before switching to manual mode
	 *	twiaddr is the I2C address of the module
	 *	signature is a number unique to this model of module (any signature between 0x30 and 0x38 will be determined as a "MotorMover" module)
	 *	id is a number unique to this board of this model
	 *	name is the name of the module, a max of 8 characters
	 *	operational is whether the module is functioning correctly - if there is an error, this should be set to false
	 * Note that I just chose a random number for twiaddr.
	 */

	mover.set_timeout(1000);
	mover.set_twiaddr(0x10);
	mover.set_signature(0x30);
	mover.set_id(0);
	mover.set_name("MotorMvr");
	mover.set_operational(true);

	
	/* Although the objects that client needs have been created, they have not yet been given to client.
	 * The module needs to be registered because client needs to use the information from the master to
	 *		- Determine whether the master is connected
	 *		- Set the red LED to whatever the master has specified
	 *
	 * When registering these objects to the client, we use the "address of" operator '&'.
	 * Usually, when passing arguments to a function, a -copy- of the argument is made.
	 * By passing an address, it allows client to reference the -same- object we have here (could also pass by reference, but for another time).
	 */
	 client.register_module(&mover);
	 client.register_output_led_red(&led_red);
	 client.register_input_button_test(&button_test);
	 client.register_input_switch_mode(&switch_mode);
}


/* Create a 'ClientMode' enum that will store the client mode from the previous loop
 * A 'ClientMode' enum can take the following values:
 *	- None
 *	- Connected
 *	- Manual
 *	- Test
 * Since ClientMode is a scoped enum, its values have to be accessed using the scope operator '::'.
 * For example, "ClientMode::Connected"
 */

ClientMode previousmode = ClientMode::None;

libarduino_m328::DigitalIn button_engage(10);
DigitalInputStates button_engage_states(&button_engage);
DigitalInputStates button_test_states(&button_test);
libarduino_m328::DigitalOut led_green(8);

//Create 1000Hz (1ms) software Timer to be used for measuring 1 second
Timer1k timer;

//Keep track of the LED state in manual mode
bool led_green_state = false;

void loop() {
	/* This is a non-blocking program. That means that it never stops to wait for something to finish.
	 * For example, to blink an LED, it does -not- do something like:
	 *		led on
	 *		wait 500
	 *		led off
	 *		wait 500
	 *
	 * Instead, it uses software timers to check whether the time has elapsed, and if so, moves onto the next state.
	 * Because of this, most aspects of libmodule have an 'update' function that needs to be called once per loop (or frame if you like games).
	 * This design allows the program to remain responsive even when waiting for something.
	 * Since client manages the user interface, it needs to have an update function called once per frame so that it can process what it needs to process.
	 * Since the module has been registered with the client, the client will then take care up updating the module.
	 */
	client.update();

	button_test_states.update();
	button_engage_states.update();

	/* As documented in OneNote, every module can be in 3 primary different states:
	 *	- Connected (controlled by master)
	 *	- Manual (controlled by hardware input)
	 *	- Test (testing subsystems on module)
	 * The client automatically determines what mode the module should be in, and this can be checked here.
	 * Depending on what mode the module is in, different things should be done in 'loop'.
	 */

	 ClientMode currentmode = client.get_mode();

	 /* Here we can check for whether the mode has changed from the previous loop.
	  * This allows us to make code that will only run when the mode has changed.
	  * There is a difference between code that will run once when a mode changes, and code that will run every loop when in a certain mode.
	  */

	//"if current mode is not equal to previous mode"
	if(currentmode != previousmode) {
		//If just entered TestMode
		if(currentmode == ClientMode::Test) {
			//led_green should turned on for 1 second
			led_green.set(true);

			//Set a timer for 1 second and start it
			timer = 1000;
			timer.start();
		}
		previousmode = currentmode;
	}

	//When in connected mode, set the green LED to whether the system is engaged or not
	if(currentmode == ClientMode::Connected) {
		//Will only turn on if the module is in binary mode, the mechanism is powered, and the system is engaged
		//Note that the result of the expression mover.get_mode() == MotorMoverMode::Binary is either true or false
		//The && operator is for "and", and will also result in either an expression that is true or false.
		//Therefore, led_green will only be turned on if all three conditions below are true
		led_green.set(mover.get_mode() == MotorMoverMode::Binary && mover.get_mechanism_powered() && mover.get_binary_engaged());
	}

	//When in manual mode, toggle the green LED when the engage button is pressed
	if(currentmode == ClientMode::Manual) {
		if(button_engage_states.pressed) {
			//Toggle the state in memory and set the LED (the ! operator is the "invert" or "not" operator)
			led_green_state = !led_green_state;
			led_green.set(led_green_state);
		}
	}

	//When in test mode, toggle the green LED every time the sample button is pressed
	if(currentmode == ClientMode::Test) {
		//Exit test mode when timer has finished
		if(timer.finished) {
			//Turn off green LED
			led_green.set(false);
			//The client needs to be notified of when we want to exit test mode, which is done by calling "test_finish()".
			client.test_finish();
		}
	}
}
