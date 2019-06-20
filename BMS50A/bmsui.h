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
		//Will be true if Armed finishes because of a BMS condition (but not if the centre button was pressed
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

	/* Main UI menu. Itself spawns a List of menu items, but houses the callbacks for the events in the menu.
	 * If the 'Arm' option is selected, will finish.
	 */
	class MainMenu : public libmodule::ui::Screen<libmodule::ui::segdpad::Common> {
	public:
		MainMenu();
	private:
		void ui_update();
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
		Main(libmodule::ui::segdpad::Common *const ui_common);
	private:
		void ui_update() override;
		void ui_on_childComplete() override;

		bool runinit = true;
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
