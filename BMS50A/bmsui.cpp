/*
 * bmsui.cpp
 *
 * Created: 18/04/2019 12:50:48 AM
 *  Author: teddy
 */ 

#include "bmsui.h"

#include <stdio.h>
#include <string.h>

bms::BMS *ui::bms_ptr = nullptr;

//Could probably leave out the 6 here
ui::printer::CellVoltage        *ui::printer::cellvoltage[6];
ui::printer::AverageCellVoltage *ui::printer::averagecellvoltage;
ui::printer::BatteryVoltage     *ui::printer::batteryvoltage;
ui::printer::BatteryPresent     *ui::printer::batterypresent;
ui::printer::Temperature        *ui::printer::temperature;
ui::printer::Current            *ui::printer::current;

ui::statdisplay::StatDisplay *ui::statdisplay::cellvoltage[6];
ui::statdisplay::StatDisplay *ui::statdisplay::averagecellvoltage;
ui::statdisplay::StatDisplay *ui::statdisplay::batteryvoltage;
ui::statdisplay::StatDisplay *ui::statdisplay::batterypresent;
ui::statdisplay::StatDisplay *ui::statdisplay::temperature;
ui::statdisplay::StatDisplay *ui::statdisplay::current;
ui::statdisplay::StatDisplay *ui::statdisplay::all[ui::statdisplay::all_len];

void ui::printer::setup()
{
	cellvoltage[0]     = new CellVoltage(bms::snc::cellvoltage[0]);
	cellvoltage[1]     = new CellVoltage(bms::snc::cellvoltage[1]);
	cellvoltage[2]     = new CellVoltage(bms::snc::cellvoltage[2]);
	cellvoltage[3]     = new CellVoltage(bms::snc::cellvoltage[3]);
	cellvoltage[4]     = new CellVoltage(bms::snc::cellvoltage[4]);
	cellvoltage[5]     = new CellVoltage(bms::snc::cellvoltage[5]);
	averagecellvoltage = new AverageCellVoltage(bms::snc::cellvoltage);
	batteryvoltage     = new BatteryVoltage(bms::snc::cellvoltage);
	batterypresent     = new BatteryPresent(bms::snc::batterypresent);
	temperature        = new Temperature(bms::snc::temperature);
	current            = new Current(bms::snc::current_optimised);
}

void ui::statdisplay::setup()
{
	cellvoltage[0]     = new StatDisplay("c1", printer::cellvoltage[0]    );
	cellvoltage[1]     = new StatDisplay("c2", printer::cellvoltage[1]    );
	cellvoltage[2]     = new StatDisplay("c3", printer::cellvoltage[2]    );
	cellvoltage[3]     = new StatDisplay("c4", printer::cellvoltage[3]    );
	cellvoltage[4]     = new StatDisplay("c5", printer::cellvoltage[4]    );
	cellvoltage[5]     = new StatDisplay("c6", printer::cellvoltage[5]    );
	averagecellvoltage = new StatDisplay("Av", printer::averagecellvoltage);
	batteryvoltage     = new StatDisplay("bv", printer::batteryvoltage    );
	batterypresent     = new StatDisplay("bp", printer::batterypresent    );
	temperature        = new StatDisplay("tp", printer::temperature       );
	current            = new StatDisplay("Cu", printer::current           );
	for(uint8_t i = 0; i < 6; i++) all[i] = cellvoltage[i];
	all[6]  = averagecellvoltage;
	all[7]  = batteryvoltage;
	all[8]  = batterypresent;
	all[9]  = temperature;
	all[10] = current;
}

void ui::statdisplay::update()
{
	for(uint8_t i = 0; i < 6; i++) {
		cellvoltage[i]->update();
	}
	averagecellvoltage->update();
	batteryvoltage->update();
	batterypresent->update();
	temperature->update();
	current->update();
}

void ui::printer::CellVoltage::print(char str[], uint8_t const len /*= 4*/) const 
{
	snprintf(str, len, "%2.1f", static_cast<double>(s->get()));
}
ui::printer::CellVoltage::CellVoltage(bms::Sensor_t const *s) : s(s) {}


void ui::printer::AverageCellVoltage::print(char str[], uint8_t const len /*= 4*/) const
{
	float sum = 0.0f;
	for(uint8_t i = 0; i < 6; i++) {
		sum += s[i]->get();
	}
	snprintf(str, len, "%2.1f", static_cast<double>(sum / 6));
}
ui::printer::AverageCellVoltage::AverageCellVoltage(bms::Sensor_t *s[6]) : s(s) {}

void ui::printer::BatteryVoltage::print(char str[], uint8_t const len /*= 4*/) const 
{
	float sum = 0.0f;
	for(uint8_t i = 0; i < 6; i++) {
		sum += s[i]->get();
	}
	snprintf(str, len, "%02d", static_cast<int>(sum));
}
ui::printer::BatteryVoltage::BatteryVoltage(bms::Sensor_t *s[6]) : s(s) {}


void ui::printer::BatteryPresent::print(char str[], uint8_t const len /*= 4*/) const 
{
	if(s->get()) strncpy(str, "yE", len);
	else strncpy(str, "no", len);
}
ui::printer::BatteryPresent::BatteryPresent(bms::DigiSensor_t const *s) : s(s) {}


void ui::printer::Temperature::print(char str[], uint8_t const len /*= 4*/) const 
{
	float val = s->get();
	//Should read 187.5356 when disconnected
	if(val >= 180.0f) strncpy(str, "--", len);
	else snprintf(str, len, "%2.1f", static_cast<double>(val));
	//If there is a decimal point on the right, don't show it
	if(str[2] == '.') str[2] = '\0';
}
ui::printer::Temperature::Temperature(bms::Sensor_t const *s) : s(s) {}

void ui::printer::Current::print(char str[], uint8_t const len /*= 4*/) const 
{
	snprintf(str, len, "%2.1f", static_cast<double>(libmodule::utility::tmax<float>(0.0f, s->get())));
}
ui::printer::Current::Current(bms::sensor::CurrentOptimised *s) : s(s) {}

ui::statdisplay::StatDisplay::Screen_t * ui::statdisplay::StatDisplay::on_click()
{
	showing_name = !showing_name;
	//If now showing name, copy stat_name into Item::name
	if(showing_name) {
		//Remove stray right decimal point
		memset(name, 0, sizeof name);
		strncpy(name, stat_name, sizeof stat_name);
	}
	return nullptr;
}

void ui::statdisplay::StatDisplay::update()
{
	//If showing statistic, print in the value
	if(!showing_name) {
		//Zero out name before printing (prevents stray decimal point)
		memset(name, 0, sizeof name);
		stat_printer->print(name, sizeof name);
	}
}

ui::statdisplay::StatDisplay::StatDisplay(char const stat_name[3], printer::Printer const *stat_printer) : stat_printer(stat_printer)
{
	//Copy in stat_name
	strncpy(this->stat_name, stat_name, sizeof this->stat_name);
	//Zero out name memory
	memset(name, 0, sizeof name);
	//Display stat_name (copy to Item::name)
	strncpy(name, this->stat_name, sizeof this->stat_name);
}

void ui::statdisplay::StatDisplay::on_highlight()
{
	update();
}

void ui::Armed::ui_update()
{
	//Enables BMS (which enables the battery present condition and the relay flipping)
	if(runinit) {
		//--- Read settings here ---
		runinit = false;
		bms_ptr->set_enabled(true);
		//Run the armed pattern	
		ui_common->dp_right_blinker.run_pattern(pattern_dp_armed);
		//Setup and start timers
		timer_displaytimeout = ticks_displaytimeout;
		stopwatch_cycletimeout.reset();
		timer_displaytimeout.start();
		stopwatch_cycletimeout.start();
	}
	//If the BMS has an error, the BMS will flip the relay. Armed finishes there.
	if(bms_ptr->get_error_signal()) {
		result_finishedBecauseOfError = true;
		ui_common->dp_right_blinker.set_state(false);
		ui_finish();
	}
	//If the centre button is pressed, disable the BMS (which flips relay) and finish
	if(ui_common->dpad.centre.get()) {
		bms_ptr->set_enabled(false);
		ui_common->dp_right_blinker.set_state(false);
		ui_finish();
	}
	//If the left button is pressed, leave the Armed screen
	else if(ui_common->dpad.left.get()) {
		ui_common->dp_right_blinker.set_state(false);
		ui_finish();
	}

	//Reset inactive timeout if a button is pressed
	bool wakeup_button_pressed = ui_common->dpad.up.get() || ui_common->dpad.down.get() || ui_common->dpad.right.get();
	if(wakeup_button_pressed) {
		timer_displaytimeout.reset();
		timer_displaytimeout = ticks_displaytimeout;
	}

	//If the display is on, cycle through values
	if(display_on) {
		//If display timeout has been reached, turn off display
		if(timer_displaytimeout) {
			display_on = false;
			ui_common->segs.clear();
		}
		else {
			auto current_statdisplay = statdisplay::all[statdisplay_all_pos];
			//When it is time to stop showing the label, switch to the statistic (number)
			if(stopwatch_cycletimeout.ticks >= ticks_labeltimeout && current_statdisplay->showing_name) {
				//Calling on_click should swap them
				current_statdisplay->on_click();
			}
			//If it is time for a new statistic
			if(stopwatch_cycletimeout.ticks >= ticks_cycletimeout) {
				//Set the previous one back to the label
				current_statdisplay->on_click();
				//Move onto next one
				if(++statdisplay_all_pos >= statdisplay::all_len) statdisplay_all_pos = 0;
				current_statdisplay = statdisplay::all[statdisplay_all_pos];
				//Make sure next one shows label
				if(!current_statdisplay->showing_name) current_statdisplay->on_click();
				//Reset the cycle timeout
				stopwatch_cycletimeout = 0;
			}
			//Print to statdisplay
			current_statdisplay->update();
			//Write to display
			ui_common->segs.write_characters(current_statdisplay->name, sizeof current_statdisplay->name, libmodule::userio::IC_LTD_2601G_11::OVERWRITE_LEFT);
		}
	}
	//If not, check for buttons being pressed that will turn on the display
	else if(wakeup_button_pressed) {
		display_on = true;
		//Go back to start of current statistic cycle
		if(!statdisplay::all[statdisplay_all_pos]->showing_name) statdisplay::all[statdisplay_all_pos]->on_click();
		stopwatch_cycletimeout.ticks = 0;
	}
}

libmodule::userio::Blinker::Pattern ui::Armed::pattern_dp_armed = {
	250, 500, 0, 1, true, false
};

void ui::Countdown::ui_update()
{
	if(runinit) {
		runinit = false;
		//Reset the BMS
		bms_ptr->set_enabled(false);
		//Start the countdown
		timer_countdown = config::default_ui_countdown_ticks_countdown;
		timer_countdown.start();
		return;
	}
	//If an error occurs, finish
	if(bms_ptr->get_error_signal()) {
		res = Result::Error;
		ui_finish();
	}
	//If any button is pressed, finish
	if(ui_common->dpad.left.get() || ui_common->dpad.right.get() || ui_common->dpad.up.get() || ui_common->dpad.down.get() || ui_common->dpad.centre.get()) {
		res = Result::ButtonPressed;
		ui_finish();
	}
	//If the timer is finished, finish
	if(timer_countdown) {
		res = Result::CountdownFinished;
		ui_finish();
	}
	//Finish when 1 finishes, so zero is not shown (hence the + 1)
	char str[3];
	snprintf(str, sizeof str, "%2u", static_cast<unsigned int>(timer_countdown.ticks) / 1000 + 1);
	ui_common->segs.write_characters(str, sizeof str, libmodule::userio::IC_LTD_2601G_11::OVERWRITE_LEFT | libmodule::userio::IC_LTD_2601G_11::OVERWRITE_RIGHT);
}

void ui::StartupDelay::ui_update()
{
	if(runinit) {
		runinit = false;
		bms_ptr->set_enabled(false);
		timer_countdown = config::default_ui_startupdelay_ticks_countdown;
		timer_countdown.start();
	}
	if(timer_countdown) ui_finish();
	char str[3];
	snprintf(str, sizeof str, "%02u", static_cast<unsigned int>(timer_countdown.ticks / 101));
	ui_common->segs.write_characters(str, sizeof str, libmodule::userio::IC_LTD_2601G_11::OVERWRITE_LEFT | libmodule::userio::IC_LTD_2601G_11::OVERWRITE_RIGHT);
}

void ui::Main::ui_update()
{
	switch(next_spawn) {
	case Child::None: break;

	case Child::StartupDelay:
		current_spawn = Child::StartupDelay;
		ui_spawn(new StartupDelay);
		break;
	case Child::Countdown:
		current_spawn = Child::Countdown;
		ui_spawn(new Countdown);
		break;
	case Child::Armed:
		current_spawn = Child::Armed;
		ui_spawn(new Armed);
		break;
	case Child::TriggerDetails:
		current_spawn = Child::TriggerDetails;
		ui_spawn(new TriggerDetails);
		break;
	case Child::MainMenu:
		current_spawn = Child::MainMenu;
		ui_spawn(new MainMenu);
		break;
	}

}

void ui::Main::ui_on_childComplete()
{
	switch(current_spawn) {
	case Child::None: break;

	case Child::StartupDelay:
		next_spawn = Child::Countdown;
		//Calibrate sensors after startup delay
		bms::snc::calibrate();
		break;

	case Child::Countdown: {
		auto countdown_ptr = static_cast<Countdown *>(ui_child);
		switch(countdown_ptr->res) {
		case Countdown::Result::ButtonPressed:
			//Spawn MainMenu if button was pressed
			next_spawn = Child::MainMenu;
			break;
		case Countdown::Result::CountdownFinished:
			//Spawn Armed if countdown finished
			next_spawn = Child::Armed;
			break;
		case Countdown::Result::Error:
			next_spawn = Child::TriggerDetails;
			break;
		}
		break;
		}

	case Child::Armed: {
		auto armed_ptr = static_cast<Armed *>(ui_child);
		if(armed_ptr->result_finishedBecauseOfError) next_spawn = Child::TriggerDetails;
		else next_spawn = Child::MainMenu;
		break;

		}
	case Child::TriggerDetails:
		next_spawn = Child::MainMenu;
		break;

	case Child::MainMenu:
		//If main menu finishes, should spawn either countdown or armed depending on whether the BMS is active or not
		if(bms_ptr->get_enabled()) next_spawn = Child::Armed;
		else next_spawn = Child::Countdown;
		break;
	}
}

void ui::Main::update()
{
	ui_management_update();
}

ui::Main::Main(libmodule::ui::segdpad::Common *const ui_common) : Screen(ui_common) {}

void ui::TriggerDetails::ui_update()
{
	buttontimer_dpad.update();
	if(buttontimer_dpad.pressedFor(config::default_ui_triggerdetails_ticks_exittimeout)) {
		//Reset rapid-fire so that there are no accidental inputs on leaving
		ui_common->dpad.up.reset();
		ui_common->dpad.down.reset();
		ui_common->dpad.left.reset();
		ui_common->dpad.right.reset();
		ui_common->dpad.centre.reset();
		ui_finish();
	}
	ui_common->segs.write_characters("Er");
}

bool ui::TriggerDetails::get() const 
{
	//Or-ing dpads
	return ui_common->dpad.left.m_instates.held || ui_common->dpad.right.m_instates.held || ui_common->dpad.up.m_instates.held
		|| ui_common->dpad.down.m_instates.held || ui_common->dpad.centre.m_instates.held;
}

ui::TriggerDetails::TriggerDetails() : buttontimer_dpad(this) {}

void ui::MainMenu::ui_update()
{
	if(runinit) {
		auto menu_list = new libmodule::ui::segdpad::List;
		//3 menu items
		//	- Armed
		//	- Stats
		//	- Settings
		strcpy(item_armed.name, "Ar");
		strcpy(item_stats.name, "SA");
		strcpy(item_settings.name, "St");
		menu_list->m_items.resize(3);
		menu_list->m_items[0] = &item_armed;
		menu_list->m_items[1] = &item_stats;
		menu_list->m_items[2] = &item_settings;
		ui_spawn(menu_list);
	}
}

libmodule::ui::Screen<libmodule::ui::segdpad::Common> * ui::MainMenu::on_armed_clicked()
{
	//When armed is clicked, finish
	ui_finish();
	//Don't spawn any screens
	return nullptr;
}

libmodule::ui::Screen<libmodule::ui::segdpad::Common> * ui::MainMenu::on_stats_clicked()
{
	//Spawn a list of statdisplays
	auto statdisplay_list = new libmodule::ui::segdpad::List;
	statdisplay_list->m_items.resize(6 + 5);
	for(uint8_t i = 0; i < 6; i++) statdisplay_list->m_items[i] = ui::statdisplay::cellvoltage[i];
	statdisplay_list->m_items[6]  = ui::statdisplay::averagecellvoltage;
	statdisplay_list->m_items[7]  = ui::statdisplay::batteryvoltage;
	statdisplay_list->m_items[8]  = ui::statdisplay::batterypresent;
	statdisplay_list->m_items[9]  = ui::statdisplay::temperature;
	statdisplay_list->m_items[10] = ui::statdisplay::current;
	//Spawn the list
	return statdisplay_list;
}

libmodule::ui::Screen<libmodule::ui::segdpad::Common> * ui::MainMenu::on_settings_clicked()
{
	//Don't do anything when settings is clicked at the moment
	return nullptr;
}

ui::MainMenu::MainMenu() : item_armed(this, &MainMenu::on_armed_clicked), item_stats(this, &MainMenu::on_stats_clicked), item_settings(this, &MainMenu::on_settings_clicked) {}
