/*
 * bms.cpp
 *
 * Created: 20/04/2019 2:57:25 PM
 *  Author: teddy
 */ 

#include "bms.h"
#include "config.h"

bool bms::ConditionDaemon::get() const
{
	return errorsignal;
}

void bms::ConditionDaemon::update()
{
	errorsignal = false;
	if(!enabled) return;
	//If there is currently a condition registered as the cause of the signal and this condition is enabled, make sure it is still a problem
	if(signal_cause != nullptr) {
		if(signal_cause->get_ok() && signal_cause->cd_enabled) signal_cause = nullptr;
	}

	for(uint8_t i = 0; i < conditions.size(); i++) {
		//If everything is all good and this condition is enabled, make sure timer isn't running
		if(conditions[i]->get_ok() && signal_cause->cd_enabled) {
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
bms::condition::CellUndervoltage::CellUndervoltage(config::TriggerSettings_f const &triggersettings, uint8_t const index)
 : Condition(static_cast<ConditionID>(static_cast<uint8_t>(ConditionID::CellUndervoltage_0) + index), triggersettings),
   min(triggersettings.value), index(index) {}

bool bms::condition::CellOvervoltage::get_ok() const 
{
	return snc::cellvoltage[index]->get() <= max;
}
bms::condition::CellOvervoltage::CellOvervoltage(config::TriggerSettings_f const &triggersettings, uint8_t const index)
 : Condition(static_cast<ConditionID>(static_cast<uint8_t>(ConditionID::CellOvervoltage_0) + index), triggersettings),
   max(triggersettings.value), index(index) {}

bool bms::condition::OverTemperature::get_ok() const 
{
	return snc::temperature->get() <= max;
}
bms::condition::OverTemperature::OverTemperature(config::TriggerSettings_f const &triggersettings) : Condition(ConditionID::OverTemperature, triggersettings),
  max(triggersettings.value) {}

bool bms::condition::OverCurrent::get_ok() const 
{
	return snc::current_optimised->get() <= max;
}
bms::condition::OverCurrent::OverCurrent(config::TriggerSettings_f const &triggersettings) : Condition(ConditionID::OverCurrent, triggersettings), max(triggersettings.value) {}

bool bms::condition::Battery::get_ok() const 
{
	//If not enabled, always return ok
	return !enabled || snc::batterypresent->get() != error_state;
}

bms::condition::Battery::Battery(config::TriggerSettings_b const &triggersettings) : Condition(ConditionID::Battery, triggersettings), error_state(triggersettings.value) {}

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
//Would make a whole lot more sense just to put these as default values in the constructor
cd_cell_uv{
	{config::settings.trigger_cell_min_voltage, 0},
	{config::settings.trigger_cell_min_voltage, 1},
	{config::settings.trigger_cell_min_voltage, 2},
	{config::settings.trigger_cell_min_voltage, 3},
	{config::settings.trigger_cell_min_voltage, 4},
	{config::settings.trigger_cell_min_voltage, 5}},
cd_cell_ov{
	{config::settings.trigger_cell_max_voltage, 0},
	{config::settings.trigger_cell_max_voltage, 1},
	{config::settings.trigger_cell_max_voltage, 2},
	{config::settings.trigger_cell_max_voltage, 3},
	{config::settings.trigger_cell_max_voltage, 4},
	{config::settings.trigger_cell_max_voltage, 5}},
cd_overtemperature(config::settings.trigger_max_temperature), cd_overcurrent(config::settings.trigger_max_current),
cd_battery(config::settings.trigger_battery_present)
{
	//Fill conditiondaemon with conditions
	constexpr uint8_t conditions_count = 6 + 6 + 1 + CONDITION_TEMPERATURE_ENABLED + CONDITION_BATTERYPRESENT_ENABLED;

	conditiondaemon.conditions.resize(conditions_count);
	uint8_t itr = 0;
	for(uint8_t i = 0; i < 6; i++) {
		conditiondaemon.conditions[itr++] = cd_cell_uv + i;
	}
	for(uint8_t i = 0; i < 6; i++) {
		conditiondaemon.conditions[itr++] = cd_cell_ov + i;
	}
#if (CONDITION_TEMPERATURE_ENABLED == 1)
	conditiondaemon.conditions[itr++] = &cd_overtemperature;
#endif
	conditiondaemon.conditions[itr++] = &cd_overcurrent;
#if (CONDITION_BATTERYPRESENT_ENABLED == 1)
	conditiondaemon.conditions[itr++] = &cd_battery;
#endif
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
