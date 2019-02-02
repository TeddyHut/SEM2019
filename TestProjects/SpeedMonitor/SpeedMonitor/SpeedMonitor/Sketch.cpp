#include <Arduino.h>
#include <libmodule.h>
#include <libarduino_m328.h>

/* This project is called SpeedMonitor Test
 * It requires the following hardware:
 *	- Arduino running using an ATmega328 or similar
 *	- An LED connected to a digital pin called "led_red"
 *	- An LED connected to a digital pin called "led_green"
 *	- A button connected to a digital pin called "button_test"
 *	- A button connected to a digital pin called "button_sample"
 *	- A button connected to a digital pin called "switch_mode" (optional)
 *
 * This program is a test for the SpeedMonitor communication.
 * It functions as follows:
 * Connected Mode:
 *		In connected mode, pushing the sample button will add a time sample to the circular buffer for the master.
 *		If the master is not connected, or the "switch_mode" is held, the module will go into manual mode.
 * Manual Mode:
 *		In manual mode, if the average of the 4 samples is less than 250ms, it will turn on led_green.
 * Test Mode:
 *		To enter test mode, hold down button_test for 1 second.
 *		To exit test mode, press button_test
 *		In test mode, if the sample button is pushed, the led_green will toggle states.
 */

#define MANUAL_AVGTIMEMS_TRIGGER 250

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

/* Below sets a SpeedMonitor type alias.
 * Just like you can create a variable as a number (e.g. int x;), C/C++ allow us to define custom types with custom data and functions.
 * Inside libmodule, in the namespace 'module', there is a custom type called "SpeedMonitor", but it needs more information (which we give it here).
 * So, just like you could write "int x;" to create an int called 'x', we can write "SpeedMonitor x;" to create a SpeedMonitor object called 'x'.
 *
 * 1 SpeedMonitor handles 1 sensor/circular buffer. More than one SpeedMonitor object can be created.
 * The SpeedMonitor type has the following syntax:
 *	SpeedMonitor<sample count, sample type>
 *		sample count: The number of samples
 *		sample type: The type of the samples
 * For example, SpeedMonitor<4, uint32_t> would be a SpeedMonitor with 4 samples, with each sample an unsigned 32 bit integer.
 * By making an alias, from then on we can just type "Monitor" instead of "module::SpeedMonitor<4, uint32_t>"
 * It is important to note that this does -not- create a SpeedMonitor object we can use, just creates another way to write the 'type'
 */

using Monitor = module::SpeedMonitor<4, uint16_t>;

/* Set SpeedMonitorManager type alias.
 * A MonitorManager combines all the individual SpeedMonitor objects so that they can communicate as one module.
 * Since a single module could be measuring from multiple sensors, we can create multiple SpeedMonitors. However, we need just 1 Manager.
 * The SpeedMonitorManager has the following syntax:
 *	SpeedMonitorManager<SpeedMonitor type, number of monitors>
 *		SpeedMonitor type: The type of SpeedMontior. In this case, we put in our type alias "Monitor" from before.
 *		number of monitors: The number of SpeedMonitors the Manager should manage. In this case there is just 1 (the test button)
 * The manager needs to know this information so that it can allocate the right amount of memory to handle/distribute to the SpeedMonitors.
 */

using Manager = module::SpeedMonitorManager<Monitor, 1>;

/* Create a module Client object called 'client' (note the lowercase on the instance).
 * While the module object (e.g. Manager) handles communication with the master, it does -not- handle changing to different modes, managing LEDs, etc.
 * This is done by the 'Client' object.
 * In summary:
 *		Monitor: Handles a single circular buffer/sensor. Multiple can be created (e.g. one for wheel, one for motor)
 *		Manager: Allocates memory/manages the Monitors. It also handles communication of the information with the Master
 *		Client: Handles the 'user interface' of the module. e.g. Blinking of LEDs, reading buttons, determining modes based on the environment
 */

module::Client client;

/* Create a module manager object called 'manager'.
 * This object requires extra information on instantiation (this is -not- the same as the extra information for the type above).
 * Since Manager handles communication with the master, it needs to be given an TWI (I2C) interface it can use to do so.
 * This allows the same code to be used on different hardware with different I2C interfaces.
 * Since this I2C interface is hardware specific, it can be found in the libarduino_m328 library (which also has its own namespace).
 * The syntax is the following:
 *	Manager [name](TWI Hardware Interface);
 *
 * Inside libarduino_m328 there is an interface called twiSlave0, which we pass to Manager
 */

//Manager manager(libarduino_m328::twiSlave0);
libmodule::module::SpeedMonitorManager<libmodule::module::SpeedMonitor<4, uint16_t>, 1> manager(libarduino_m328::twiSlave0);
//^^^ This is the same thing, but with the fully qualified name.

//Create an instance of monitor
//Monitor monitor_test;
libmodule::module::SpeedMonitor<4, uint16_t> monitor_test;


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

	/* This is where we set parameters for the module (in this case manager).
	 *	timeout is the amount of time that the module should wait after receiving no communication from the master before switching to manual mode
	 *	twiaddr is the I2C address of the module
	 *	signature is a number unique to this model of module
	 *	id is a number unique to this board of this model
	 *	name is the name of the module, a max of 8 characters
	 *	operational is whether the module is functioning correctly - if there is an error, this should be set to false
	 * Note that I just chose random numbers for twiaddr and signature
	 */

	manager.set_timeout(1000);
	manager.set_twiaddr(0x10);
	manager.set_signature(0x56);
	manager.set_id(1);
	manager.set_name("SpdMnitr");
	manager.set_operational(true);


	/* The manager needs to have the speed monitor registered to it.
	 * The syntax is the following:
	 *	manager.register_speedMonitor(position, speedmonitor);
	 *		position - The index of the SpeedMonitor in the manager, starting at 0
	 *		speedmonitor - The address of the speedmonitor to be registered
	 */

	manager.register_speedMonitor(0, &monitor_test);
	
	/* Although the objects that client needs have been created, they have not yet been given to client.
	 * The module (manager) needs to be registered because client needs to use the information from the master to
	 *		- Determine whether the master is connected
	 *		- Set the red LED to whatever the master has specified
	 *
	 * When registering these objects to the client, we use the "address of" operator '&'.
	 * Usually, when passing arguments to a function, a -copy- of the argument is made.
	 * By passing an address, it allows client to reference the -same- object we have here (could also pass by reference, but for another time).
	 */
	 client.register_module(&manager);
	 client.register_output_led_red(&led_red);
	 client.register_input_button_test(&button_test);
	 client.register_input_switch_mode(&switch_mode);

	 monitor_test.set_rps_constant(MANUAL_AVGTIMEMS_TRIGGER);
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

Stopwatch1k stopwatch;

libarduino_m328::DigitalIn button_sample(10);
DigitalInputStates button_sample_states(&button_sample);
DigitalInputStates button_test_states(&button_test);
libarduino_m328::DigitalOut led_green(8);

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
	button_sample_states.update();

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
		//This project doesn't require any initialization codes for different modes
	}

	//No matter what mode, if the button is pushed then add a sample
	if(button_sample_states.pressed) {
		monitor_test.push_sample(stopwatch.ticks);
		stopwatch.ticks = 0;
		//Don't count the first sample
		stopwatch.start();
	}

	//When in manual mode, check whether the average press time is less than a certain amount
	if(currentmode == ClientMode::Manual || currentmode == ClientMode::Connected) {
		uint32_t total = 0;
		uint8_t valid_samples = 0;
		for(uint8_t i = 0; i < monitor_test.sample_count + 1; i++) {
			uint16_t sample = monitor_test.get_sample(i);
			if(i == monitor_test.sample_count) {
				sample = stopwatch.ticks;
				if(sample <= total / valid_samples)
					sample = 0;
			}
			if(sample > 0) {
				total += sample;
				valid_samples++;
			}
		}
		//Not allowed to divide by 0
		if(valid_samples != 0) {
			//Set the LED to whether the average reaches the trigger
			led_green.set(total / valid_samples <= MANUAL_AVGTIMEMS_TRIGGER);
		}
		else
			led_green.set(false);
	}

	if(currentmode == ClientMode::Test) {
		if(button_sample_states.pressed) {
			led_green_state = !led_green_state;
			led_green.set(led_green_state);
		}
		if(button_test_states.released) {
			client.test_finish();
		}
	}
	previousmode = currentmode;
}
