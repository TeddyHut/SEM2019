/*
 * module.h
 *
 * Created: 12/01/2019 6:36:33 PM
 *  Author: teddy
 */ 

#pragma once

#include "metadata.h"
#include "utility.h"
#include "userio.h"
#include "twislave.h"

namespace libmodule {
	namespace module {
		//Module format: {5E, 8A, sig, id, name[8], status, settings, ...}
		
		//Handles communication/interpreting communication
		class Slave  {
		public:
			void update();
		
			void set_timeout(size_t const timeout);
			void set_twiaddr(uint8_t const addr);
			bool connected() const;

			void set_signature(uint8_t const signature);
			void set_id(uint8_t const id);
			void set_name(char const name[]);

			void set_operational(bool const state);

			//These cannot be const because of the buffer callback (bitGet() is not const)
			bool get_led();
			bool get_power();

			Slave(twi::TWISlave &twislave, utility::Buffer &buffer);
		protected:
			utility::Buffer &buffer;
			twi::SlaveBufferManager buffermanager;
		};
		class Horn : public Slave {
		public:
			bool get_state_horn() const;
			Horn(twi::TWISlave &twislave);
		private:
			utility::StaticBuffer<metadata::com::offset::_size> buffer;
		};

		//Handles the common client/module code that is not communication (modes, leds, buttons)
		class Client  {
		public:
			void update();

			enum class Mode {
				None,
				Connected,
				Manual,
				Test,
			};
			Mode get_mode() const;

			//If in test mode, will blink the LED (indicates moving onto the next test)
			void test_blink();
			//Leaves test mode
			void test_finish();

			void register_module(Slave *const slave);
			void register_input_button_test(utility::Input<bool> const *const input);
			void register_input_switch_mode(utility::Input<bool> const *const input);
			void register_output_led_red(utility::Output<bool> *const output);
		private:
			//Gets the current mode assuming that the device is not in test mode (does not call update functions)
			Mode get_nontest_mode() const;
			//True if all pointers are not nullptr
			inline bool populated() const;
			Mode pm_mode = Mode::None;
			Slave *pm_slave = nullptr;
			utility::Input<bool> const *pm_sw_mode = nullptr;
			userio::ButtonTimer1k pm_btntimer_test;
			userio::BlinkerTimer1k pm_blinker_red;
			enum class TestState {
				//Waiting for button to be pushed
				Idle,
				//Startup blink animation in progress
				Start,
				//Test mode
				Test,
				//Finish blink animation in progress
				Finish,
			} pm_teststate = TestState::Idle;
		};
	}
}
