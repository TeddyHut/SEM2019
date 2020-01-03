//Created: 23/04/2019 12:51:33 PM

/** \file
 \brief Preprocessor configuration, default configuration and settings storage.
 \date Created 2019-04-23
 \author Teddy.Hut
 */

#pragma once

#include <inttypes.h>

/** \brief The number on the BMS PCB.
 \details One time we ran out of a particular value of resistor that was used for the current sensing circuitry. So for one of the PCBs, we changed the values to something similar, but different enough that it would affect the current reading.
 This number is marked using a sticker on each BMS PCB. Changing #BOARD_NUMBER will change the relevant resistor values in the code configuration.
 \todo When moving to C++17, make this use `constexpr if` instead (if relevant - might not be, need to investigate further)
*/
#define BOARD_NUMBER 3
/** \brief Whether or not the temperature condition should be enabled.
 \details This can be handy when programming/testing PCBs as the temperature board will not need to be connected.
 \todo Make it so that this is configurable on the BMS.
*/
#define CONDITION_TEMPERATURE_ENABLED 0
/** \brief Whether or not the battery presence condition should be enabled.
 \details This can be handy when programming/testing PCBs as the battery discharge terminals will not need to be connected.
 \todo Make it so that this is configurable on the BMS.
*/
#define CONDITION_BATTERYPRESENT_ENABLED 0

//Could and probably should split these into multiple namespaces
///Primary namespace for config.h.
namespace config {
	///\name Board Parameters
	///@{
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
	///@}

	//Consider creating a non-template struct that contains ticks_timeout and enabled that this inherits from. Then modify bms::Condition to accept that instead of storing references to enabled and ticks_timeout separately.
	template <typename value_t>
	struct TriggerSettings {
		value_t value;
		bool enabled;
		uint16_t ticks_timeout;
	};

	using TriggerSettings_f = TriggerSettings<float>;
	using TriggerSettings_b = TriggerSettings<bool>;
	//Copy aggregate initialization is fine since these are constexpr
	constexpr TriggerSettings_f default_trigger_cell_min_voltage = {3.0f, true, 20};
	constexpr TriggerSettings_f default_trigger_cell_max_voltage = {4.3f, true, 20};
	constexpr TriggerSettings_f default_trigger_max_current = {25.0f, true, 20};
	constexpr TriggerSettings_f default_trigger_max_temperature = {60.0f, true, 20};
	//false for value means that there is an error if the battery is not present (consider inverting)
	constexpr TriggerSettings_b default_trigger_battery_present = {false, true, 500};

	//System parameters
	constexpr uint16_t ticks_main_system_refresh = 1000 / 120;

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
	constexpr uint16_t default_ui_triggerdetails_ticks_display_errortext = 250;
	constexpr uint16_t default_ui_triggerdetails_ticks_display_nametext = 500;
	constexpr uint16_t default_ui_triggerdetails_ticks_display_valuetext = 750;
}

//Could and probably should split these into multiple anonymous structs
namespace config {
	/** \brief Stores user configurable settings and saves/loads them from EEPROM.
	 
	 \author Teddy.Hut
	*/
	struct Settings {
		///Writes settings to EEPROM.
		void save() const;
		///Loads settings from EEPROM.
		void load();

		/** \brief Used to determine whether a Settings object has previously been written to EEPROM.
		 \details 0x5e was chosen since it is similar to <b>SE</b>MA.
		 \note This cannot be \c const since it will delete the default move-assignment operator (used in load()), and writing a custom one would be needlessly verbose.
		 \sa load()
		 */
		uint8_t write_indicator = 0x5e;

		//Note: sizeof(float) == 4
		
		TriggerSettings_f trigger_cell_min_voltage = default_trigger_cell_min_voltage;
		TriggerSettings_f trigger_cell_max_voltage = default_trigger_cell_max_voltage;
		TriggerSettings_f trigger_max_current = default_trigger_max_current;
		TriggerSettings_f trigger_max_temperature = default_trigger_max_temperature;
		TriggerSettings_b trigger_battery_present = default_trigger_battery_present;

		/** \name ui::Armed Parameters */
		///@{
		///\copydoc default_ui_armed_ticks_displaytimeout
		uint32_t ui_armed_ticks_displaytimeout = default_ui_armed_ticks_displaytimeout;
		///\copydoc default_ui_armed_ticks_cycletimeout
		uint16_t ui_armed_ticks_cycletimeout = default_ui_armed_ticks_cycletimeout;
		///\copydoc default_ui_armed_ticks_labeltimeout
		uint16_t ui_armed_ticks_labeltimeout = default_ui_armed_ticks_labeltimeout;
		///@}
	};
	//Global settings struct
	extern Settings settings;
}
