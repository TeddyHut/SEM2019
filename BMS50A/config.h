/*
 * config.h
 *
 * Created: 23/04/2019 12:51:33 PM
 *  Author: teddy
 */ 

#pragma once

#include <inttypes.h>

#define BOARD_NUMBER 3

//Could and probably should split these into multiple namespaces
namespace config {
	//Board parameters
	constexpr float resistor_r1 = 24;
	constexpr float resistor_r2 = 10;
#if (BOARD_NUMBER == 1)
	constexpr float resistor_r37_r38 = 15.4;
	constexpr float	resistor_r55_r56 = 15.4;
	constexpr float current_cutoff_sensor_1A = 0.9;
#elif (BOARD_NUMBER == 2)
	constexpr float resistor_r37_r38 = 15.4;
	constexpr float	resistor_r55_r56 = 15.4;
	constexpr float current_cutoff_sensor_1A = 0.9;
#elif (BOARD_NUMBER == 3)
	constexpr float resistor_r37_r38 = 15;
	constexpr float	resistor_r55_r56 = 15;
	constexpr float current_cutoff_sensor_1A = 0.65;
#endif

	constexpr uint16_t ticks_main_system_refresh = 1000 / 120;
	//Condition times
	constexpr uint16_t default_ticks_timeout_cell_min_voltage = 20;
	constexpr uint16_t default_ticks_timeout_cell_max_voltage = 20;
	constexpr uint16_t default_ticks_timeout_max_current = 20;
	constexpr uint16_t default_ticks_timeout_max_temperature = 20;
	constexpr uint16_t default_ticks_timeout_battery_present = 60000;

	//Condition parameters
	constexpr float default_cell_min_voltage = 3.0;
	constexpr float default_cell_max_voltage = 4.3;
	constexpr float default_max_temperature = 60;
	constexpr float default_max_current = 25;

	//ui::Countdown parameters
	constexpr uint16_t default_ui_countdown_ticks_countdown = 5000;
	
	//ui::StartupDelay parameters
	constexpr uint16_t default_ui_startupdelay_ticks_countdown = 1000;

	//ui::Armed prameters
	constexpr uint16_t default_ui_armed_ticks_cycletimeout = 2000;
	constexpr uint16_t default_ui_armed_ticks_labeltimeout = 500;
	//Should cycle through all displays once
	constexpr uint32_t default_ui_armed_ticks_displaytimeout = default_ui_armed_ticks_cycletimeout * (6 + 5);

	//ui::TriggerDetails parameters
	constexpr uint16_t default_ui_triggerdetails_ticks_exittimeout = 1000;
}

//Could and probably should split these into multiple anonymous structs
namespace config {
	struct Settings {
		//Writes settings to EEPROM
		void save() const;
		//Loads settings from EEPROM. If settings have never been written, uses default values from config (above) instead.
		void load();

		//Condition times
		uint16_t ticks_timeout_cell_min_voltage = default_ticks_timeout_cell_min_voltage;
		uint16_t ticks_timeout_cell_max_voltage = default_ticks_timeout_cell_max_voltage;
		uint16_t ticks_timeout_max_current = default_ticks_timeout_max_current;
		uint16_t ticks_timeout_max_temperature = default_ticks_timeout_max_temperature;
		uint16_t ticks_timeout_battery_present = default_ticks_timeout_battery_present;

		//Condition parameters
		float cell_min_voltage = default_cell_min_voltage;
		float cell_max_voltage = default_cell_max_voltage;
		float max_temperature = default_max_temperature;
		float max_current = default_max_current;

		//ui::Armed parameters
		uint32_t ui_armed_ticks_displaytimeout = default_ui_armed_ticks_displaytimeout;
		uint16_t ui_armed_ticks_cycletimeout = default_ui_armed_ticks_cycletimeout;
		uint16_t ui_armed_ticks_labeltimeout = default_ui_armed_ticks_labeltimeout;
	};
	//Global settings struct
	extern Settings settings;
}
