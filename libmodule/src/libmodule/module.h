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

		template <size_t len_c, typename sample_t = uint32_t>
		class SpeedMonitor {
			static_assert(len_c > 0, "SpeedMonitor len must be greater than 0");
			static_assert(sizeof(sample_t) <= 0xf, "SpeedMonitor sample_t must have size less than 0xf");

			template <typename, size_t>
			friend class SpeedMonitorManager;
		public:
			void set_rps_constant(metadata::speedmonitor::rps_t const rps);
			void set_tps_constant(metadata::speedmonitor::cps_t const tps);

			void push_sample(sample_t const sample);
			void clear_samples();
		private:
			utility::Buffer buffer;
			uint8_t pm_samplepos = 0;
		};

		template <typename>
		class speedmonitor_len;

		//Used to determine len_c and sample_t of a SpeedMonitor
		template <size_t len, typename tsample>
		class speedmonitor_len<SpeedMonitor<len, tsample>> {
			static constexpr size_t len_c = len;
			using sample_t = tsample;
		};

		//TODO: Consider making it possible for all modules to have multiple instances within a single manager
		template <typename SpeedMonitor_t, size_t count_c>
		class SpeedMonitorManager : public Slave {
			static_assert(count_c > 0, "SpeedMonitorManager count must be greater than 0");

			using speedmonitor_len_t = speedmonitor_len<SpeedMonitor_t>;
			using sample_t = typename speedmonitor_len_t::sample_t;
			static constexpr size_t len_c = speedmonitor_len_t::len;
			static constexpr size_t instance_buffer_size_c = metadata::speedmonitor::offset::instance::SampleBuffer + len_c * sizeof(sample_t);
			static constexpr size_t manager_buffer_size_c = metadata::speedmonitor::offset::manager::_size;
			static constexpr size_t overall_buffer_size_c = manager_buffer_size_c + count_c * instance_buffer_size_c;
		public:
			void register_speedMonitor(uint8_t const pos, SpeedMonitor_t *const instance);

			SpeedMonitorManager(twi::TWISlave &twislave);
		private:
			utility::StaticBuffer<overall_buffer_size_c> buffer;
			
		};

		class MotorController : public Slave {
		public:
			enum class MotorMode {
				Off     = 0b00,
				Voltage = 0b01,
				PWM     = 0b10,
			};
			enum class OvercurrentState {
				None = 0b00,
				PWM  = 0b01, //Voltage mode -> PWM mode
				Off  = 0b10, //PWM mode -> off
			};

			OvercurrentState get_state_overcurrent() const;
			void update();

			void set_measured_current(uint16_t const mA);
			void set_measured_voltage(uint16_t const mV);
			void set_overcurrent_timeout(uint16_t const ms);
			MotorMode get_mode_motor() const;
			uint16_t get_pwm_frequency() const;
			uint8_t get_pwm_duty() const;
			uint16_t get_control_mV() const;

			MotorController(twi::TWISlave &twislave);
		private:
			utility::StaticBuffer<metadata::motorcontroller::offset::_size> buffer;
			MotorMode pm_motormode;
			OvercurrentState pm_overcurrentstate;
			Timer1k pm_timer;
			uint16_t pm_timeout;
			uint16_t pm_measured_mA;
			uint16_t pm_measured_mV;
		};

		class MotorMover : public Slave {
		public:
			enum class Mode {
				Binary     = 0b0,
				Continuous = 0b1,
			};

			void set_position_engaged(uint16_t const pos);
			void set_position_disengaged(uint16_t const pos);

			void set_engaged(bool const engaged);
			Mode get_mode() const;
			bool get_binary_engaged() const;
			uint16_t get_continuous_position() const;
			bool get_mechanism_powered() const;
			MotorMover(twi::TWISlave &twislave);
		private:
			utility::StaticBuffer<metadata::motormover::offset::_size> buffer;
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


template <size_t len_c, typename sample_t /*= uint32_t*/>
void libmodule::module::SpeedMonitor<len_c, sample_t>::set_rps_constant(metadata::speedmonitor::rps_t const rps)
{
	buffer.serialiseWrite(rps, metadata::speedmonitor::offset::instance::Constant_RPS);
}


template <size_t len_c, typename sample_t /*= uint32_t*/>
void libmodule::module::SpeedMonitor<len_c, sample_t>::set_tps_constant(metadata::speedmonitor::rps_t const tps)
{
	buffer.serialiseWrite(tps, metadata::speedmonitor::offset::instance::Constant_TPS);
}


template <size_t len_c, typename sample_t /*= uint32_t*/>
void libmodule::module::SpeedMonitor<len_c, sample_t>::push_sample(sample_t const sample)
{
	buffer.serialiseWrite(sample, metadata::speedmonitor::offset::instance::SampleBuffer + pm_samplepos * sizeof(sample_t));
	buffer.serialiseWrite(pm_samplepos, metadata::speedmonitor::offset::instance::SamplePos);
	if(++pm_samplepos >= len_c) {
		pm_samplepos = 0;
	}
}

template <size_t len_c, typename sample_t /*= uint32_t*/>
void libmodule::module::SpeedMonitor<len_c, sample_t>::clear_samples()
{
	memset(buffer.pm_ptr + metadata::speedmonitor::offset::instance::SampleBuffer, 0, len_c * sizeof(sample_t));
	pm_samplepos = 0;
}

template <typename SpeedMonitor_t, size_t count_c>
void libmodule::module::SpeedMonitorManager<SpeedMonitor_t, count_c>::register_speedMonitor(uint8_t const pos, SpeedMonitor_t *const instance)
{
	if(instance == nullptr || pos > count_c) {
		hw::panic();
	}

	instance->buffer.pm_ptr = buffer.pm_ptr + manager_buffer_size_c + pos * instance_buffer_size_c;
	instance->buffer.pm_len = instance_buffer_size_c;
}


template <typename SpeedMonitor_t, size_t count_c>
libmodule::module::SpeedMonitorManager<SpeedMonitor_t, count_c>::SpeedMonitorManager(twi::TWISlave &twislave) : Slave(twislave, buffer)
{
	memset(buffer.pm_ptr, 0, overall_buffer_size_c);
	buffer.bit_set(metadata::com::offset::Status, metadata::com::sig::status::Active, true);
	set_operational(true);

	//Set sample size, instance count, and sample count
	buffer.bit_set_mask(metadata::com::offset::Status, sizeof(sample_t) << metadata::speedmonitor::sig::status::SampleSize);
	buffer.serialiseWrite(static_cast<uint8_t>(count_c), metadata::speedmonitor::offset::manager::InstanceCount);
	buffer.serialiseWrite(static_cast<uint8_t>(len_c), metadata::speedmonitor::offset::manager::SampleCount);
}
