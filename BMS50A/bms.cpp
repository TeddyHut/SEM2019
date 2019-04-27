/*
 * bms.cpp
 *
 * Created: 20/04/2019 2:57:25 PM
 *  Author: teddy
 */ 

#include "bms.h"
#include "config.h"

bms::Condition::Condition(ConditionID const id, uint16_t const timeout) : cd_timeout(timeout),  cd_id(id) {}

bool bms::ConditionDaemon::get() const
{
	return errorsignal;
}

void bms::ConditionDaemon::update()
{
	errorsignal = false;
	if(!enabled) return;
	//If there is currently a condition registered as the cause of the signal, make sure it is still a problem
	if(signal_cause != nullptr) {
		if(signal_cause->get_ok()) signal_cause = nullptr;
	}

	for(uint8_t i = 0; i < conditions.size(); i++) {
		//If everything is all good, make sure timer isn't running
		if(conditions[i]->get_ok()) {
			conditions[i]->cd_timer.reset();
			conditions[i]->cd_timer = conditions[i]->cd_timeout;
		}
		//If there is an error, make sure timer is running
		else {
			conditions[i]->cd_timer.start();
			//If timer has finished, signal an error
			if(conditions[i]->cd_timer.finished) {
				errorsignal = true;
				//If there is no current condition causing error, make this the signal cause
				if(signal_cause == nullptr)	signal_cause = conditions[i];
			}
		}
	}
}

void bms::ConditionDaemon::set_enabled(bool const enable)
{
	//If enabled, reset conditions to default timeout
	if(enable) {
		for(uint8_t i = 0; i < conditions.size(); i++) {
			conditions[i]->cd_timer.reset();
			conditions[i]->cd_timer = conditions[i]->cd_timeout;
		}
	}
	enabled = enable;
}

bms::Condition * bms::ConditionDaemon::get_signal_cause() const
{
	return signal_cause;
}

bms::ConditionDaemon::ConditionDaemon() : enabled(false), errorsignal(false) {}

bool bms::condition::CellUndervoltage::get_ok() const 
{
	return snc::cellvoltage[index]->get() >= min;
}
bms::condition::CellUndervoltage::CellUndervoltage(float const min, uint8_t const index)
 : Condition(static_cast<ConditionID>(static_cast<uint8_t>(ConditionID::CellUndervoltage_0) + index), config::default_ticks_timeout_cell_min_voltage),
   min(min), index(index) {}

bool bms::condition::CellOvervoltage::get_ok() const 
{
	return snc::cellvoltage[index]->get() <= max;
}
bms::condition::CellOvervoltage::CellOvervoltage(float const max, uint8_t const index)
 : Condition(static_cast<ConditionID>(static_cast<uint8_t>(ConditionID::CellOvervoltage_0) + index), config::default_ticks_timeout_cell_max_voltage),
   max(max), index(index) {}

bool bms::condition::OverTemperature::get_ok() const 
{
	return snc::temperature->get() <= max;
}
bms::condition::OverTemperature::OverTemperature(float const max) : Condition(ConditionID::OverTemperature, config::default_ticks_timeout_max_temperature),
  max(max) {}

bool bms::condition::OverCurrent::get_ok() const 
{
	return snc::current_optimised->get() <= max;
}
bms::condition::OverCurrent::OverCurrent(float const max) : Condition(ConditionID::OverCurrent, config::default_ticks_timeout_max_current), max(max) {}

bool bms::condition::Battery::get_ok() const 
{
	//If not enabled, always return ok
	return !enabled || snc::batterypresent->get();
}

bms::condition::Battery::Battery() : Condition(ConditionID::Battery, config::default_ticks_timeout_battery_present) {}

void bms::BMS::update()
{
	conditiondaemon.update();
	//If enabled and there is an error, disable (will flip relay)
	if(enabled && conditiondaemon.get()) {
		disabled_error_id = conditiondaemon.get_signal_cause()->cd_id;
		set_enabled(false);
	}
	btimer_relayLeft.update();
	btimer_relayRight.update();
}

void bms::BMS::set_enabled(bool const enable)
{
	enabled = enable;
	//Enabling condition daemon will reset it, which should happen either way
	conditiondaemon.set_enabled(true);
	cd_battery.enabled = enable;
	flip_relay(enable);
}

void bms::BMS::set_digiout_relayLeft(DigiOut_t *const p)
{
	btimer_relayLeft.pm_out = p;
	//Turn on (default to both on)
	btimer_relayLeft.set_state(true);
}

void bms::BMS::set_digiout_relayRight(DigiOut_t *const p)
{	
	btimer_relayRight.pm_out = p;
	btimer_relayRight.set_state(true);
}

bool bms::BMS::get_error_signal() const
{
	return conditiondaemon.get();
}

bool bms::BMS::get_enabled() const
{
	return enabled;
}

bms::ConditionID bms::BMS::get_current_error_id() const
{
	return conditiondaemon.get_signal_cause()->cd_id;
}

bms::ConditionID bms::BMS::get_disabled_error_id() const
{
	return disabled_error_id;
}

bms::BMS::BMS() :
//Would make a whole lot more sense just to put these as default values in the contructor
cd_cell_uv{
	{config::default_cell_min_voltage, 0},
	{config::default_cell_min_voltage, 1},
	{config::default_cell_min_voltage, 2},
	{config::default_cell_min_voltage, 3},
	{config::default_cell_min_voltage, 4},
	{config::default_cell_min_voltage, 5}},
cd_cell_ov{
	{config::default_cell_max_voltage, 0},
	{config::default_cell_max_voltage, 1},
	{config::default_cell_max_voltage, 2},
	{config::default_cell_max_voltage, 3},
	{config::default_cell_max_voltage, 4},
	{config::default_cell_max_voltage, 5}},
cd_overtemperature(config::default_max_temperature), cd_overcurrent(config::default_max_current)
{
	//Fill conditiondaemon with conditions
	conditiondaemon.conditions.resize(6 + 6 + 3);
	uint8_t itr = 0;
	for(uint8_t i = 0; i < 6; i++) {
		conditiondaemon.conditions[itr++] = cd_cell_uv + i;
	}
	for(uint8_t i = 0; i < 6; i++) {
		conditiondaemon.conditions[itr++] = cd_cell_ov + i;
	}
	conditiondaemon.conditions[itr++] = &cd_overtemperature;
	conditiondaemon.conditions[itr++] = &cd_overcurrent;
	conditiondaemon.conditions[itr++] = &cd_battery;
}

void bms::BMS::flip_relay(bool const on)
{
	//Left should be off for 250ms
	if(on) btimer_relayLeft.run_pattern(pattern_flip);
	else btimer_relayRight.run_pattern(pattern_flip);
}

//Run relay for 250ms
libmodule::userio::Blinker::Pattern bms::BMS::pattern_flip = {
	250, 0, 0, 1, false, true
};
