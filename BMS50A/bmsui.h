/*
 * bmsui.h
 *
 * Created: 18/04/2019 12:50:28 AM
 *  Author: teddy
 */ 

#pragma once

#include <libmodule/timer.h>
#include <libmodule/userio.h>
#include <libmodule/ui.h>
#include "sensors.h"
#include "bms.h"
#include "config.h"

//TODO: Transfer all the global variables used here into a struct inherited from ui::segdpad::Common

namespace ui {
	extern bms::BMS *bms_ptr;

	namespace printer {
		struct Printer {
			virtual void print(char str[], uint8_t const len = 4) const = 0;
		};
		struct CellVoltage : public Printer {
			void print(char str[], uint8_t const len = 4) const override;
			CellVoltage(bms::Sensor_t const *s);
			bms::Sensor_t const *s;
		};
		struct AverageCellVoltage : public Printer {
			void print(char str[], uint8_t const len = 4) const override;
			//For some reason GCC doesn't like const here
			AverageCellVoltage(bms::Sensor_t *s[6]);
			//Or here
			bms::Sensor_t **s;
		};
		struct BatteryVoltage : public Printer {
			void print(char str[], uint8_t const len = 4) const override;
			//Takes an array of 6 sensors to perform a sum
			BatteryVoltage(bms::Sensor_t *s[6]);
			bms::Sensor_t **s;
		};
		struct BatteryPresent : public Printer {
			void print(char str[], uint8_t const len = 4) const override;
			BatteryPresent(bms::DigiSensor_t const *s);
			bms::DigiSensor_t const *s;
		};
		struct Temperature : public Printer {
			void print(char str[], uint8_t const len = 4) const override;
			Temperature(bms::Sensor_t const *s);
			bms::Sensor_t const *s;
		};
		struct Current : public Printer {
			void print(char str[], uint8_t const len = 4) const override;
			Current(bms::sensor::CurrentOptimised *s);
			bms::sensor::CurrentOptimised *s;
		};

		extern CellVoltage        *cellvoltage[6];
		extern AverageCellVoltage *averagecellvoltage;
		extern BatteryVoltage     *batteryvoltage;
		extern BatteryPresent     *batterypresent;
		extern Temperature        *temperature;
		extern Current            *current;

		void setup();
	}

	namespace statdisplay {
		//A list item used to display a statistic. When clicked toggles between the statistic name and the statistic value.
		struct StatDisplay : public libmodule::ui::segdpad::List::Item {
			//Screen_t is introduced in Item as Screen<Common>;
			Screen_t *on_click() override;
			//Will call update
			void on_highlight(bool const firstcycle) override;
			void update();
			StatDisplay(char const stat_name[3], printer::Printer const *stat_printer);
			char stat_name[2];
			printer::Printer const *stat_printer;
			bool showing_name = true;
		};
		//TODO: Consider a template class that can be explicitly instantiated that would create multiple items of the same type in the following format
		extern StatDisplay *cellvoltage[6];
		extern StatDisplay *averagecellvoltage;
		extern StatDisplay *batteryvoltage;
		extern StatDisplay *batterypresent;
		extern StatDisplay *temperature;
		extern StatDisplay *current;

		constexpr size_t all_len = 6 + 5;
		extern StatDisplay *all[all_len];
		
		//main calls setup
		void setup();
		//Responsibility of whatever Screen is using them to update the StatDisplays.
		void update();
		//Gets the statdisplay that shows the value for the associated TriggerID
		StatDisplay *get_statdisplay_conditionID(bms::ConditionID const id);
	}

	/* For when the BMS is 'armed'/in its running state (technically it should always be armed unless interrupted at startup or triggered).
	 * Upon startup will arm the relay if it wasn't already -> enable the BMS
	 * Will cycle through statistics to be displayed, showing the name of the statistic and then the value. Display will turn off after timeout.
	 * Navigation:
	 *	up: skip/cycle current statistic up. If display is off, turn on display.
	 *	down: skip/cycle current statistic down. If display is off, turn on display.
	 *	left: ui_finish
	 *	right: wake up display
	 *	centre:	trigger relay, ui_finish
	 * Finish conditions:
	 *	Relay triggered
	 */
	class Armed : public libmodule::ui::Screen<libmodule::ui::segdpad::Common> {
	public:
		bool result_finishedBecauseOfError = false;
		static libmodule::userio::Blinker::Pattern pattern_dp_armed;
	private:
		void ui_update() override;
		//Will be true if Armed finishes because of a BMS condition (but not if the centre button was pressed)
		bool runinit = true;
		bool display_on = true;
		//The amount time of inactivity before the display turns off
		uint32_t ticks_displaytimeout = config::default_ui_armed_ticks_displaytimeout;
		//The ticks_labeltimeout + ticks for value
		uint16_t ticks_cycletimeout = config::default_ui_armed_ticks_cycletimeout;
		//The amount of time to show the statistic name for
		uint16_t ticks_labeltimeout = config::default_ui_armed_ticks_labeltimeout;
		//Use uint32_ts for the display timeout (since 65535 is only 65 seconds)
		libmodule::time::Timer<1000, uint32_t> timer_displaytimeout;
		libmodule::Stopwatch1k stopwatch_cycletimeout;
		uint8_t statdisplay_all_pos = 0;
	};

	/* Counts down from a number (probably 5) in seconds.
	 * Will turn BMS off (flip relay) on startup
	 * Countdown will finish when the timer reaches 0
	 * If any trigger occurs, will finish.
	 * If any button is pressed, will finish.
	 */
	class Countdown : public libmodule::ui::Screen<libmodule::ui::segdpad::Common> {
	public:
		enum class Result {
			CountdownFinished,
			ButtonPressed,
			Error,
		} res;
	private:
		void ui_update() override;
		bool runinit = true;
		libmodule::Timer1k timer_countdown;
	};

	/* Counts down rapidly from 100 (or 99) to 0, in milliseconds. Finishes when reaching 0.
	 * Will turn BMS off (flip relay) on startup
	 */
	class StartupDelay : public libmodule::ui::Screen<libmodule::ui::segdpad::Common> {
		void ui_update() override;
		bool runinit = true;
		libmodule::Timer1k timer_countdown;
	};

	/* Displays details about the cause of a trigger. If a button is held for 1 second, will finish.
	 * This object uses itself as the input for the buttontimer, since the dpad needs to be or-ed (it really should be a separate object though)
	 */
	class TriggerDetails : public libmodule::ui::Screen<libmodule::ui::segdpad::Common>, public libmodule::utility::Input<bool> {
	public:
		void ui_update() override;
		//Used for OR-ing dpad
		bool get() const override;
		TriggerDetails();
	private:
		enum class DisplayState {
			ErrorText,
			NameText,
			ValueText,
		} displaystate = DisplayState::ErrorText;
		bool runinit = true;
		libmodule::userio::ButtonTimer1k buttontimer_dpad;
		libmodule::Timer1k timer_animation;
		char str_errorname[2] = {'-', '-'};
		char str_errorvalue[4] = "--";
	};

	/* UI interface to allow the user to edit a config::TriggerConfig<value_t> object.
	 * There is a specialization for bool, which is why there is a Common object and two inherited objects.
	 */
	template <typename value_t>
	class TriggerSettingsEdit_Common : public libmodule::ui::Screen<libmodule::ui::segdpad::Common> {
	public:
		TriggerSettingsEdit_Common(config::TriggerSettings<value_t> &trigger, config::TriggerSettings<value_t> const &trigger_default);
	protected:
		config::TriggerSettings<value_t> &triggersettings;
		//virtual so that a different method can be used for subclass value_t specialization
		virtual Screen *on_valueedit_clicked() = 0;
		virtual void on_valueedit_finished(Screen *const) = 0;
	private:
		void ui_update() override;
		//Saves settings and finishes
		void ui_on_childComplete() override;
		
		//Spawns NumberInputDecimal to edit timeout
		Screen *on_timeedit_clicked();
		//Toggles the decimal point on the "En" screen
		Screen *on_enablededit_clicked();
		//Spawns Selector with yes/no options
		Screen *on_resettodefault_clicked();

		//Makes sure the user confirmed a new time, and sets it
		void on_timeedit_finished(Screen *const decinput);
		//If the user selected "yes", resets triggersettings to triggersettings_default
		void on_resettodefault_finished(Screen *const yn_input);

		//Sets the decimal point to match the enabled state
		void on_enableedit_highlight(bool const firstcycle);
		
		bool runinit = true;
		config::TriggerSettings<value_t> const &triggersettings_default;
		libmodule::ui::segdpad::List::Item_MemFnCallback<TriggerSettingsEdit_Common> item_valueedit;
		libmodule::ui::segdpad::List::Item_MemFnCallback<TriggerSettingsEdit_Common> item_timeedit;
		libmodule::ui::segdpad::List::Item_MemFnCallback<TriggerSettingsEdit_Common> item_enablededit;
		libmodule::ui::segdpad::List::Item_MemFnCallback<TriggerSettingsEdit_Common> item_resettodefault;
	};

	//Consider changing uint16_t to a type template parameter.
	/* Unspecialized TriggerSettingsEdit.
	 * Requires:
	 *  Conversion functions to convert from the display interpretation of the value to the settings interpretation, and the opposite.
	 *  Configuration for the NumberInputDecimal.
	 *  Reference to the TriggerSettings in the settings object.
	 *  Reference to the default TriggerSettings for these settings.
	 */
	template <typename value_t>
	class TriggerSettingsEdit : public TriggerSettingsEdit_Common<value_t> {
		using typename TriggerSettingsEdit_Common<value_t>::Screen;
		template <typename input_t, typename output_t>
		using conv_fn = output_t (input_t const value);
	private:
		Screen *on_valueedit_clicked() override;
		void on_valueedit_finished(Screen *const decinput) override;

		//Configuration to pass to value_inputconfig for entering a value
		libmodule::ui::segdpad::NumberInputDecimal::Config const value_decinputconfig;

		//Ideally these could be lambdas. Although this is somewhat difficult to do without C++17 or the standard library.
		//Could use an intermediate function specialization to determine the lambda type (and then perhaps typedef it in a helper class or something), but that would be annoying.
		//Something like std::function, but that requires dynamic memory to implement, something we don't use unless we have to around here.
		//Convert NumberInputDecimal output to value_t. Allows TriggerSettingsEdit to set the value in triggersettings on completion)
		conv_fn<uint16_t, value_t> const &conv_edit_to_value;
		//Convert value_t to NumberInputDecimal output. Allows TriggerSettingsEdit to set the default value in the NumberInputDecimal.
		conv_fn<value_t, uint16_t> const &conv_value_to_edit;
	public:
		//I think that the const & is included in the type of conv_edit_to_value, but I'm not sure at the moment.
		TriggerSettingsEdit(libmodule::ui::segdpad::NumberInputDecimal::Config const &value_decinputconfig,
			config::TriggerSettings<value_t> &trigger, config::TriggerSettings<value_t> const &trigger_default,
			decltype(conv_edit_to_value) conv_edit_to_value, decltype(conv_value_to_edit) conv_value_to_edit);
	};

	/* Specialization for bool.
	 * Spawns an "on" or "oF" selector screen to set the value to true or false, respectively.
	 * This means that value conversion functions and the NumberInputDecimal configuration are not required.
	 */
	template <>
	class TriggerSettingsEdit<bool> : public TriggerSettingsEdit_Common<bool> {
	public:
		//Inherit constructor
		using TriggerSettingsEdit_Common::TriggerSettingsEdit_Common;
	private:
		//Spawns an "on" or "oF" selector
		Screen *on_valueedit_clicked() override;
		void on_valueedit_finished(Screen *const onoff_input);
	};


	class TriggerSettingsList : public libmodule::ui::Screen<libmodule::ui::segdpad::Common> {
	public:
		TriggerSettingsList();
	private:
		void ui_update() override;
		//Finishes
		void ui_on_childComplete() override;

		//Each spawns a TriggerSettingsEdit
		Screen *on_undervoltage_clicked();
		Screen *on_overvoltage_clicked();
		Screen *on_overcurrent_clicked();
		Screen *on_overtemperature_clicked();
		Screen *on_batterypresent_clicked();

		//This probably isn't actually needed, since ui_update should only be calld once. TODO: Check.
		bool runinit = true;
		using Item_MemFnCallback = libmodule::ui::segdpad::List::Item_MemFnCallback<TriggerSettingsList>;
		Item_MemFnCallback item_undervoltage;
		Item_MemFnCallback item_overvoltage;
		Item_MemFnCallback item_overcurrent;
		Item_MemFnCallback item_overtemperature;
		Item_MemFnCallback item_batterypresent;
	};

	class SettingsMenu : public libmodule::ui::Screen<libmodule::ui::segdpad::Common> {
	public:
		SettingsMenu();
	private:
		void ui_update() override;
		void ui_on_childComplete() override;

		//Spawns TriggerSettingsList
		Screen *on_triggersettings_clicked();
		//Spawns
		//Screen *on_displaysettings_clicked();
		//Spawns yes/no selector
		Screen *on_resetall_clicked();

		//Resets settings if user confirmed
		void on_resetall_finished(Screen *const selector);

		using Item_MemFnCallback = libmodule::ui::segdpad::List::Item_MemFnCallback<SettingsMenu>;
		Item_MemFnCallback item_triggersettings;
		//Item_MemFnCallback item_displaysettings;
		Item_MemFnCallback item_resetall;
	};

	/* Main UI menu. Itself spawns a List of menu items, but houses the callbacks for the events in the menu.
	 * If the 'Arm' option is selected, will finish.
	 */
	class MainMenu : public libmodule::ui::Screen<libmodule::ui::segdpad::Common> {
	public:
		MainMenu();
	private:
		void ui_update() override;
		//Will check the BMS status and run the Armed blinker if it is enabled
		void update_armed_blinker();

		Screen *on_armed_clicked();
		Screen *on_stats_clicked();
		Screen *on_settings_clicked();


		bool runinit = true;
		libmodule::ui::segdpad::List::Item_MemFnCallback<MainMenu> item_armed;
		libmodule::ui::segdpad::List::Item_MemFnCallback<MainMenu> item_stats;
		libmodule::ui::segdpad::List::Item_MemFnCallback<MainMenu> item_settings;
	};

	/* Top-level Screen. On startup, spawns StartupDelay.
	 * When startup delay finishes, spawns Countdown.
	 * When Countdown finishes, if there is a trigger, it will spawn TriggerDetails
	 * When TriggerDetails finishes, will spawn MainMenu
	 * If there is no trigger, it will spawn MainManu.
	 * When MainMenu finishes, will spawn Countdown.
	 */
	class Main : public libmodule::ui::Screen<libmodule::ui::segdpad::Common> {
	public:
		void update();
		Main(libmodule::ui::segdpad::Common *const ui_common, bool const start_at_mainmenu);
	private:
		void ui_update() override;
		void ui_on_childComplete() override;

		bool runinit = true;
		bool start_at_mainmenu;
		enum class Child {
			None,
			StartupDelay,
			Countdown,
			Armed,
			TriggerDetails,
			MainMenu,
		} current_spawn = Child::None, next_spawn = Child::StartupDelay;
	};
}


template <typename value_t>
ui::TriggerSettingsEdit_Common<value_t>::TriggerSettingsEdit_Common(config::TriggerSettings<value_t> &trigger, config::TriggerSettings<value_t> const &trigger_default)
 : triggersettings(trigger), triggersettings_default(trigger_default),
 item_valueedit(this, &TriggerSettingsEdit_Common::on_valueedit_clicked, &TriggerSettingsEdit_Common::on_valueedit_finished),
 item_timeedit(this, &TriggerSettingsEdit_Common::on_timeedit_clicked, &TriggerSettingsEdit_Common::on_timeedit_finished),
 item_enablededit(this, &TriggerSettingsEdit_Common::on_enablededit_clicked, nullptr, &TriggerSettingsEdit_Common::on_enableedit_highlight),
 item_resettodefault(this, &TriggerSettingsEdit_Common::on_resettodefault_clicked, &TriggerSettingsEdit_Common::on_resettodefault_finished) {}

template <typename value_t>
void ui::TriggerSettingsEdit_Common<value_t>::ui_update()
{
	if(runinit) {
		runinit = false;
		//Clear the name for enablededit to allow null terminators for both 2 character and 3 character strings
		memset(item_enablededit.name, '\0', sizeof item_enablededit.name);
		//Copy in the rest of the strings
		strcpy(item_valueedit.name, "VA");
		strcpy(item_timeedit.name, "ti");
		strcpy(item_enablededit.name, "En");
		strcpy(item_resettodefault.name, "dE");
		//Add all the callback items to the list and spawn the list
		auto settings_list = new libmodule::ui::segdpad::List;
		settings_list->m_items.resize(4);
		settings_list->m_items[0] = &item_valueedit;
		settings_list->m_items[1] = &item_timeedit;
		settings_list->m_items[2] = &item_enablededit;
		settings_list->m_items[3] = &item_resettodefault;
		//Note to self for future: ui_spawn needs to be in ui_update since ui_common has not transferred in the	constructor, and therefore the child will never receive it.
		ui_spawn(settings_list);
	}
}

template <typename value_t>
void ui::TriggerSettingsEdit_Common<value_t>::ui_on_childComplete()
{
	//When the user exits the list, save the settings to EEPROM and finish
	config::settings.save();
	ui_finish();
}

template <typename value_t>
auto ui::TriggerSettingsEdit_Common<value_t>::on_timeedit_clicked()->Screen *
{
	//Consider putting this in class declaration
	using libmodule::ui::segdpad::NumberInputDecimal;
	//Could use aggregate initialization, but without designated initializers (C++20) that would be much less readable.
	//Units column represents 100ms
	NumberInputDecimal::Config input_config;
	input_config.min = 0; //0 ms
	input_config.max = 990; //9.9s
	input_config.step = 1; //10ms
	input_config.sig10 = 1;
	input_config.wrap = false;
	input_config.dynamic_step = true;
	//Spawn a NumberInputDecimal so user can enter a time
	return new NumberInputDecimal(input_config, triggersettings.ticks_timeout / 10);
}

template <typename value_t>
auto ui::TriggerSettingsEdit_Common<value_t>::on_enablededit_clicked()->Screen *
{
	//Toggle the enabled setting
	triggersettings.enabled ^= true;
	return nullptr;
}

template <typename value_t>
auto ui::TriggerSettingsEdit_Common<value_t>::on_resettodefault_clicked()->Screen *
{
	//Spawn yes/no screen
	char const items[][4] = {"no", "yE"};
	return new libmodule::ui::segdpad::Selector<2>(items, 0);
}

template <typename value_t>
void ui::TriggerSettingsEdit_Common<value_t>::on_timeedit_finished(Screen *const decinput)
{
	using libmodule::ui::segdpad::NumberInputDecimal;
	//Ensure the user confirmed they wanted to set a new value, and if they did, set the new timeout value (this should set it in the settings object it originated from)
	if(static_cast<NumberInputDecimal *>(decinput)->m_confirmed) {
		triggersettings.ticks_timeout = static_cast<NumberInputDecimal *>(decinput)->m_value * 10;
		//Run a confirm animation if no other animations are running
		ui_common->dp_right_blinker.run_pattern_ifSolid(libmodule::ui::segdpad::pattern::rubberband);
	}
}

template <typename value_t>
void ui::TriggerSettingsEdit_Common<value_t>::on_resettodefault_finished(Screen *const yn_input)
{
	using Selector = libmodule::ui::segdpad::Selector<2>;
	//If the user selected yes, reset the triggersettings to their default. Note that yes == 1 (could use an enum, but really not worth it).
	if(static_cast<Selector *>(yn_input)->m_confirmed && static_cast<Selector *>(yn_input)->m_result == 1) {
		triggersettings = triggersettings_default;
		//Run a confirm animation if the user reset and no other animations are running
		ui_common->dp_right_blinker.run_pattern_ifSolid(libmodule::ui::segdpad::pattern::rubberband);
	}
}

template <typename value_t>
void ui::TriggerSettingsEdit_Common<value_t>::on_enableedit_highlight(bool const firstcycle)
{
	//Turn the decimal point on if enabled
	item_enablededit.name[2] = triggersettings.enabled ? '.' : '\0';
	//ui_common->dp_right_blinker.set_state(triggersettings.enabled);
}


template <typename value_t>
ui::TriggerSettingsEdit<value_t>::TriggerSettingsEdit(
	libmodule::ui::segdpad::NumberInputDecimal::Config const &value_decinputconfig,
	config::TriggerSettings<value_t> &trigger, config::TriggerSettings<value_t> const &trigger_default,
	decltype(conv_edit_to_value) conv_edit_to_value, decltype(conv_value_to_edit) conv_value_to_edit)
 : TriggerSettingsEdit_Common<value_t>(trigger, trigger_default),
 value_decinputconfig(value_decinputconfig),
 conv_edit_to_value(conv_edit_to_value),
 conv_value_to_edit(conv_value_to_edit) {}

template <typename value_t>
auto ui::TriggerSettingsEdit<value_t>::on_valueedit_clicked()->Screen *
{
	//Spawn an editor using supplied editor configuration
	return new libmodule::ui::segdpad::NumberInputDecimal(value_decinputconfig, conv_value_to_edit(this->triggersettings.value));
}

template <typename value_t>
void ui::TriggerSettingsEdit<value_t>::on_valueedit_finished(Screen *const decinput)
{
	using libmodule::ui::segdpad::NumberInputDecimal;
	//If user confirmed, set value and run animation
	if(static_cast<NumberInputDecimal *>(decinput)->m_confirmed) {
		this->triggersettings.value = conv_edit_to_value(static_cast<NumberInputDecimal *>(decinput)->m_value);
		this->ui_common->dp_right_blinker.run_pattern_ifSolid(libmodule::ui::segdpad::pattern::rubberband);
	}
}
