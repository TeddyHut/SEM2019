/*
 * module.cpp
 *
 * Created: 12/01/2019 6:36:44 PM
 *  Author: teddy
 */ 

#include <string.h>

#include "module.h"

void libmodule::module::Slave::set_timeout(size_t const timeout)
{
	buffermanager.set_timeout(timeout);
}

void libmodule::module::Slave::set_twiaddr(uint8_t const addr)
{
	buffermanager.set_twiaddr(addr);
}

bool libmodule::module::Slave::connected() const
{
	return buffermanager.connected();
}

void libmodule::module::Slave::set_signature(uint8_t const signature)
{
	buffer.serialiseWrite(signature, metadata::com::offset::Signature);
}

void libmodule::module::Slave::set_id(uint8_t const id)
{
	buffer.serialiseWrite(id, metadata::com::offset::ID);
}

void libmodule::module::Slave::set_name(char const name[])
{
	buffer.write(static_cast<void const *>(name), utility::tmin<uint8_t>(metadata::com::NameLength, strlen(name)), metadata::com::offset::Name);
}

void libmodule::module::Slave::set_operational(bool const state)
{
	buffer.bit_set(metadata::com::offset::Status, metadata::com::sig::status::Operational);
}

bool libmodule::module::Slave::get_led()
{
	return buffer.bit_get(metadata::com::offset::Settings, metadata::com::sig::settings::LED);
}

bool libmodule::module::Slave::get_power()
{
	return buffer.bit_get(metadata::com::offset::Settings, metadata::com::sig::settings::Power);
}

libmodule::module::Slave::Slave(twi::TWISlave &twislave, utility::Buffer &buffer) : buffer(buffer), buffermanager(twislave, buffer, metadata::com::Header, 1) {}

void libmodule::module::Slave::write_header()
{
}

void libmodule::module::Slave::write_constants() {}

void libmodule::module::Slave::update()
{
	//If there is a change of connection, re-write all the constants
	//Could do this every cycle, but having it being overwritten is handy for finding communication bugs
	//Note: Status is special
	if(previousconnected != buffermanager.connected()) {
		previousconnected = buffermanager.connected();
		//Header
		buffer.write(static_cast<void const *>(metadata::com::Header), sizeof(metadata::com::Header), 0);
		//Active (should always read as 1)
		buffer.bit_set(metadata::com::offset::Status, metadata::com::sig::status::Active);
		//Derived classes
		write_constants();
	}

	buffermanager.update();
}

bool libmodule::module::Horn::get_state_horn() const
{
	return buffer.bit_get(metadata::com::offset::Settings, metadata::horn::sig::settings::HornState);
}

libmodule::module::Horn::Horn(twi::TWISlave &twislave) : Slave(twislave, buffer) {
	//This has to be done here and not in the slave constructor because the Slave constructor runs before the StaticBuffer constructor
	//Clear the buffer
	memset(buffer.pm_ptr, 0, buffer.pm_len);
	set_operational(true);
}

void libmodule::module::Client::update()
{
	if(!populated())
		return;
	auto previousmode = pm_mode;

	//---Determine Mode---
	if(pm_mode == Mode::None)
		pm_mode = Mode::Manual;

	pm_slave->update();

	//Update mode from stimulus
	pm_mode = get_nontest_mode();

	//Check test button
	pm_btntimer_test.update();
	if(pm_btntimer_test.pressedFor(1000) || pm_teststate != TestState::Idle)
		pm_mode = Mode::Test;

	//---Update LED for Mode---
	if(pm_mode != previousmode) {
		userio::BlinkerTimer1k::Pattern pattern;
		switch(pm_mode) {
		case Mode::None:
			hw::panic();
			break;
		case Mode::Connected:
			pm_blinker_red.set_state(false);
			break;
		case Mode::Manual:
			//Manual blink pattern
			pattern.inverted = false;
			pattern.repeat = true;
			pattern.count = 1;
			pattern.ontime = 500;
			pattern.offtime = 4000;
			pattern.resttime = 0;
			pm_blinker_red.run_pattern(pattern);
			break;
		case Mode::Test:
			//Startup test blink pattern
			pattern.inverted = false;
			pattern.repeat = false;
			pattern.count = 2;
			pattern.ontime = 125;
			pattern.offtime = 250;
			pattern.resttime = 0;
			pm_blinker_red.run_pattern(pattern);
			break;
		}
	}

	//---Mode Actions---
	switch(pm_mode) {
	case Mode::None:
		hw::panic();
		break;
	case Mode::Connected:
		//Update LED from master
		pm_blinker_red.set_state(pm_slave->get_led());
		break;
	case Mode::Test:
		switch(pm_teststate) {
		case TestState::Idle:
			pm_teststate = TestState::Start;
		case TestState::Start:
			//Wait for animation to stop
			if(pm_blinker_red.currentMode() == userio::BlinkerTimer1k::Mode::Solid)
				pm_teststate = TestState::Test;
			break;
		case TestState::Test:
			break;
		case TestState::Finish:
			//Wait for animation to stop
			if(pm_blinker_red.currentMode() == userio::BlinkerTimer1k::Mode::Solid) {
				//Mode will be changed on next cycle above
				pm_teststate = TestState::Idle;
			}
			break;
		}
		break;
	case Mode::Manual:
		break;
	}

	pm_blinker_red.update();
}

void libmodule::module::Client::test_blink()
{
	if(pm_mode == Mode::Test && populated()) {
		userio::BlinkerTimer1k::Pattern pattern;
		pattern.inverted = false;
		pattern.repeat = false;
		pattern.count = 1;
		pattern.ontime = 150;
		pattern.offtime = 150;
		pattern.resttime = 0;
		pm_blinker_red.run_pattern(pattern);
	}
}

void libmodule::module::Client::test_finish()
{
	if(pm_teststate == TestState::Test) {
		userio::BlinkerTimer1k::Pattern pattern;
		pattern.inverted = false;
		pattern.repeat = false;
		pattern.count = 1;
		pattern.ontime = 1000;
		pattern.offtime = 1000;
		pattern.resttime = 0;
		pm_blinker_red.run_pattern(pattern);
		pm_teststate = TestState::Finish;
	}
}

void libmodule::module::Client::register_module(Slave *const slave)
{
	pm_slave = slave;
}

void libmodule::module::Client::register_input_button_test(utility::Input<bool> const *const input)
{
	pm_btntimer_test.set_input(input);
}

void libmodule::module::Client::register_input_switch_mode(utility::Input<bool> const *const input)
{
	pm_sw_mode = input;
}

void libmodule::module::Client::register_output_led_red(utility::Output<bool> *const output)
{
	pm_blinker_red.pm_out = output;
}

libmodule::module::Client::Mode libmodule::module::Client::get_mode() const
{
	//If the start animation is not taking place, return the mode
	if(pm_teststate == TestState::Idle || pm_teststate == TestState::Test || pm_teststate == TestState::Finish)
		return pm_mode;
	//Otherwise, return mode ignoring animation
	return get_nontest_mode();
}

libmodule::module::Client::Mode libmodule::module::Client::get_nontest_mode() const
{
	Mode rtrn = Mode::None;
	if(populated()) {
		rtrn = pm_slave->connected() ? Mode::Connected : Mode::Manual;
		if(pm_sw_mode->get())
			rtrn = Mode::Manual;
	}
	return rtrn;
}

bool libmodule::module::Client::populated() const
{
	return pm_slave != nullptr && pm_sw_mode != nullptr && pm_btntimer_test.get_input() != nullptr && pm_blinker_red.pm_out != nullptr;
}

libmodule::module::MotorController::OvercurrentState libmodule::module::MotorController::get_state_overcurrent() const
{
	return pm_overcurrentstate;
}

void libmodule::module::MotorController::update()
{
	auto previousovercurrent = pm_overcurrentstate;
	bool reset_timeout = false;

	//Read max current from buffer
	uint16_t const max_current = buffer.serialiseRead<uint16_t>(
		pm_motormode == MotorMode::Voltage ? metadata::motorcontroller::offset::Voltage_MaxCurrent : metadata::motorcontroller::offset::PWM_MaxCurrent);

	//If over-current condition occurred
	if(connected() && pm_measured_mA > max_current && pm_timer && pm_overcurrentstate != OvercurrentState::Off) {
		switch(pm_motormode) {
		case MotorMode::Off:
		case MotorMode::PWM:
			pm_overcurrentstate = OvercurrentState::Off;
			pm_motormode = MotorMode::Off;
			set_operational(false);
			break;
		case MotorMode::Voltage:
			pm_overcurrentstate = OvercurrentState::PWM;
			pm_motormode = MotorMode::PWM;
			reset_timeout = true;
			break;
		}
	}
	else {
		reset_timeout = true;
	}

	//Poll whether master has requested an over-current mode reset
	if(buffer.bit_get(metadata::com::offset::Settings, metadata::motorcontroller::sig::settings::OvercurrentReset)) {
		//If so, set the flag back to zero
		buffer.bit_set(metadata::com::offset::Settings, metadata::motorcontroller::sig::settings::OvercurrentReset, false);
		//Set the motor mode back to normal and reset timeout
		pm_overcurrentstate = OvercurrentState::None;
		reset_timeout = true;
	}
	
	//---Update changes---
	if(reset_timeout) {
		pm_timer = pm_timeout;
		pm_timer.start();
	}
	//If not in an over-current state, the mode is set by master (also implies operational)
	if(pm_overcurrentstate == OvercurrentState::None) {
		set_operational(true);
		pm_motormode = static_cast<MotorMode>(
			(buffer.serialiseRead<uint8_t>(metadata::com::offset::Settings) & metadata::motorcontroller::mask::settings::MotorMode)
			>> metadata::motorcontroller::sig::settings::MotorMode);
	}
	if(previousovercurrent != pm_overcurrentstate) {
		buffer.bit_clear_mask(metadata::com::offset::Status, metadata::motorcontroller::mask::status::OvercurrentState);
		buffer.bit_set_mask(metadata::com::offset::Status, static_cast<uint8_t>(pm_overcurrentstate) << metadata::motorcontroller::sig::status::OvercurrentState);
	}
}

void libmodule::module::MotorController::set_measured_current(uint16_t const mA)
{
	pm_measured_mA = mA;
	buffer.serialiseWrite(mA, metadata::motorcontroller::offset::MeasuredCurrent);
}

void libmodule::module::MotorController::set_measured_voltage(uint16_t const mV)
{
	buffer.serialiseWrite(mV, metadata::motorcontroller::offset::MeasuredVoltage);
}

void libmodule::module::MotorController::set_overcurrent_timeout(uint16_t const ms)
{
	pm_timeout = ms;
}

libmodule::module::MotorController::MotorMode libmodule::module::MotorController::get_mode_motor() const
{
	return pm_motormode;
}

uint16_t libmodule::module::MotorController::get_pwm_frequency() const
{
	return buffer.serialiseRead<uint16_t>(metadata::motorcontroller::offset::PWMFrequency);
}

uint8_t libmodule::module::MotorController::get_pwm_duty() const
{
	return buffer.serialiseRead<uint8_t>(metadata::motorcontroller::offset::PWMDutyCycle);
}

uint16_t libmodule::module::MotorController::get_control_mV() const
{
	return buffer.serialiseRead<uint16_t>(metadata::motorcontroller::offset::ControlVoltage);
}

libmodule::module::MotorController::MotorController(twi::TWISlave &twislave) : Slave(twislave, buffer)
{
	memset(buffer.pm_ptr, 0, buffer.pm_len);
	buffer.bit_set(metadata::com::offset::Status, metadata::com::sig::status::Active, true);
	set_operational(true);
}

void libmodule::module::MotorMover::set_position_engaged(uint16_t const pos)
{
	buffer.serialiseWrite(pos, metadata::motormover::offset::Position_Engaged);
}

void libmodule::module::MotorMover::set_position_disengaged(uint16_t const pos)
{
	buffer.serialiseWrite(pos, metadata::motormover::offset::Position_Disengaged);
}

void libmodule::module::MotorMover::set_engaged(bool const engaged)
{
	buffer.bit_set(metadata::com::offset::Status, metadata::motormover::sig::status::Engaged);
}

libmodule::module::MotorMover::Mode libmodule::module::MotorMover::get_mode() const
{
	return static_cast<Mode>(buffer.bit_get(metadata::com::offset::Settings, metadata::motormover::sig::settings::Mode));
}

bool libmodule::module::MotorMover::get_binary_engaged() const
{
	return buffer.bit_get(metadata::com::offset::Settings, metadata::motormover::sig::settings::Engaged);
}

uint16_t libmodule::module::MotorMover::get_continuous_position() const
{
	return buffer.serialiseRead<uint16_t>(metadata::motormover::offset::ContinuousPosition);
}

bool libmodule::module::MotorMover::get_mechanism_powered() const
{
	return buffer.bit_get(metadata::com::offset::Settings, metadata::motormover::sig::settings::Powered);
}

libmodule::module::MotorMover::MotorMover(twi::TWISlave &twislave) : Slave(twislave, buffer)
{
	memset(buffer.pm_ptr, 0, buffer.pm_len);
	buffer.bit_set(metadata::com::offset::Status, metadata::com::sig::status::Active, true);
	set_operational(true);
}
