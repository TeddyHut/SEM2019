/*
 * ui.cpp
 *
 * Created: 14/04/2019 8:26:18 PM
 *  Author: teddy
 */ 


#ifdef LIBMODULE_INCLUDE_UI

#include "ui.h"

void libmodule::ui::Dpad::set_rapidInputLevel(uint8_t const index, userio::RapidInput3L1k::Level const value)
{
	up.set_level(index, value);
	down.set_level(index, value);
	left.set_level(index, value);
	right.set_level(index, value);
	centre.set_level(index, value);
}

libmodule::userio::Blinker::Pattern libmodule::ui::segdpad::pattern::rubberband = {
	100, 100, 0, 2, false, false
};
libmodule::userio::Blinker::Pattern libmodule::ui::segdpad::pattern::edit = {
	500, 500, 0, 1, true, false
};

void libmodule::ui::segdpad::List::Item::on_finish(Screen *const screen) {}

void libmodule::ui::segdpad::List::Item::on_highlight(bool const firstcycle) {}

libmodule::ui::segdpad::List::List(bool const wrap /*= true*/) : pm_wrap(wrap) {}

void libmodule::ui::segdpad::List::ui_update()
{
	//If there are no items, write "--"
	if(m_items.size() == 0) {
		ui_common->segs.write_characters("--", 2, 0);
		//Check for finish
		if(ui_common->dpad.left.get()) ui_finish();
		return;
	}

	//Used to determine whether to set firstcycle to true in on_highlight()
	uint8_t const previous_item = pm_currentitem;

	//Move up an item
	if(ui_common->dpad.up.get()) {
		if(pm_currentitem > 0) pm_currentitem--;
		else if(pm_wrap) pm_currentitem = m_items.size() - 1;
		else ui_common->dp_right_blinker.run_pattern(pattern::rubberband);
	}
	//Move down an item
	else if(ui_common->dpad.down.get()) {
		if(++pm_currentitem >= m_items.size()) {
			if(pm_wrap) pm_currentitem = 0;
			else {
				pm_currentitem--;
				ui_common->dp_right_blinker.run_pattern(pattern::rubberband);
			}
		}
	}
	//Go back a screen
	else if(ui_common->dpad.left.get()) {
		ui_finish();
	}
	//Select the item
	else if(ui_common->dpad.right.get() || ui_common->dpad.centre.get()) {
		if(m_items[pm_currentitem] == nullptr) hw::panic();
		auto res = m_items[pm_currentitem]->on_click();
		if(res != nullptr) ui_spawn(res);
	}
	//Update the display
	if(m_items[pm_currentitem] == nullptr) hw::panic();
	//Call the on_highlight function (idea is item use this to update name if needbe)
	m_items[pm_currentitem]->on_highlight((pm_currentitem != previous_item) | run_init);
	run_init = false;
	//If a pattern is not running, turn off right dp (item may turn it on again in name, however)
	//if(ui_common->dp_right_blinker.currentMode() == userio::Blinker::Mode::Solid) ui_common->dp_right_blinker.set_state(false);
	ui_common->segs.write_characters(m_items[pm_currentitem]->name, sizeof(Item::name), userio::IC_LTD_2601G_11::OVERWRITE_LEFT |
		((ui_common->dp_right_blinker.currentMode() == userio::Blinker::Mode::Solid) ? userio::IC_LTD_2601G_11::OVERWRITE_RIGHT : 0));
}

void libmodule::ui::segdpad::List::ui_on_childComplete()
{
	m_items[pm_currentitem]->on_finish(ui_child);
}

libmodule::ui::segdpad::NumberInputDecimal::NumberInputDecimal(Config const &config, uint16_t const default_value)
 : m_value(default_value), m_default_value(default_value), m_confirmed(false), pm_runinit(true), pm_config(config) {}

void libmodule::ui::segdpad::NumberInputDecimal::ui_update()
{
	//If starting, or there is no pattern running (e.g. after a rubberband), run the edit pattern.
	if(pm_runinit || ui_common->dp_right_blinker.currentMode() == userio::Blinker::Mode::Solid) {
		pm_runinit = false;
		ui_common->dp_right_blinker.run_pattern(pattern::edit);
	}

	//Most significant digit exp
	uint8_t msd_sig10 = log10i(m_value);
	//Distance between the MSD and decimal point, with the MSD on the left resulting in a positive distance.
	int8_t dist_msd_dp = static_cast<int8_t>(log10i(m_value)) - pm_config.sig10;
	//Distance between the step size and the decimal point
	int8_t dist_step_dp = static_cast<int8_t>(log10i(pm_config.step)) - pm_config.sig10;
	//Significance of segments (0 == leftmost digit). Default to leftmost as the MSD, unless the MSD has sig0 (since there would be no right digit)
	uint8_t seg_sig10[2] = {utility::tmax<uint8_t>(msd_sig10, 1), 0};
	//If MSD is 0.x and the step size is [0.1, 1) , then the MSB is on the right
	if(dist_msd_dp == -1 && dist_step_dp == -1) {
		seg_sig10[0] = msd_sig10 + 1;
	}
	//Second MSD is 1 base less significant
	seg_sig10[1] = seg_sig10[0] - 1;

	//Extract digits for display.
	//Note that value from last frame is used, to avoid needing to re-calculate everything in the case of a wrap or change of significance.
	char str[2] = { utility::digit_to_ascii(extract_digit10i(m_value, seg_sig10[0])),
					utility::digit_to_ascii(extract_digit10i(m_value, seg_sig10[1]))};
	//Show decimal point if the left digit has the same significance as the decimal point
	ui_common->segs.write_characters(str, 2,
		userio::IC_LTD_2601G_11::OVERWRITE_LEFT | (seg_sig10[0] == pm_config.sig10 ? userio::IC_LTD_2601G_11::DISPLAY_LEFT : 0));
	
	//Functional step is the step that will be made if needed
	uint16_t functional_step = pm_config.step;
	//Dynamic step makes it so that a step is always the significance of the right digit.
	//The quantity of the step (e.g. 0.4, the quantity is 4) is maintained.
	if(pm_config.dynamic_step) {
		uint16_t step_expanded_significance = powi<uint16_t>(10, log10i(pm_config.step));
		uint8_t step_quantity = pm_config.step / step_expanded_significance;
		functional_step = step_quantity * powi<uint16_t>(10, seg_sig10[1]);
	}

	//Up - add step, wrap if configured to do so
	if(ui_common->dpad.up.get()) {
		m_value += functional_step;
		if(m_value > pm_config.max) {
			if(pm_config.wrap) m_value = pm_config.min;
			else {
				m_value = pm_config.max;
				ui_common->dp_right_blinker.run_pattern(pattern::rubberband);
			}
		}
	}
	//Down - subtract step, wrap if configured to do so
	else if(ui_common->dpad.down.get()) {
		//Check for underflow
		if(m_value < functional_step || (m_value -= functional_step) < pm_config.min) {
			if(pm_config.wrap) m_value = pm_config.max;
			else {
				m_value = pm_config.min;
				ui_common->dp_right_blinker.run_pattern(pattern::rubberband);
			}
		}
	}
	//Go back with no confirmation, stop edit blink
	else if(ui_common->dpad.left.get()) {
		m_confirmed = false;
		ui_common->dp_right_blinker.set_state(false);
		ui_finish();
	}
	//Reset value to default
	else if(ui_common->dpad.right.get()) {
		m_value = m_default_value;
	}
	//Go back with confirmation, stop edit blink
	else if(ui_common->dpad.centre.get()) {
		m_confirmed = true;
		ui_common->dp_right_blinker.set_state(false);
		ui_finish();
	}
}

libmodule::ui::segdpad::ToggleList::ToggleList(bool const wrap /*= true*/) : pm_wrap(wrap), pm_runinit(true) {}

void libmodule::ui::segdpad::ToggleList::ui_update()
{
	//If there are no items, write "--"
	if(m_items.size() == 0) {
		ui_common->segs.write_characters("--", 2, 0);
		//Check for finish
		if(ui_common->dpad.left.get()) ui_finish();
		return;
	}

	//Move up an item
	if(ui_common->dpad.up.get()) {
		if(pm_currentitem > 0) pm_currentitem--;
		else if(pm_wrap) pm_currentitem = m_items.size() - 1;
		else ui_common->dp_right_blinker.run_pattern(pattern::rubberband);
	}
	//Move down an item
	else if(ui_common->dpad.down.get()) {
		if(++pm_currentitem >= m_items.size()) {
			if(pm_wrap) pm_currentitem = 0;
			else {
				pm_currentitem--;
				ui_common->dp_right_blinker.run_pattern(pattern::rubberband);
			}
		}
	}
	//Go back (and save changes if implemented in parent), stop the edit blink
	else if(ui_common->dpad.left.get()) {
		ui_common->dp_right_blinker.set_state(false);
		ui_finish();
	}
	//Reset items to default, and stop the edit blink
	else if(ui_common->dpad.right.get()) {
		pm_runinit = true;
		ui_common->dp_right_blinker.set_state(false);
	}
	//Toggle item
	else if(ui_common->dpad.centre.get()) {
		m_items[pm_currentitem].value = !m_items[pm_currentitem].value;
	}

	if(pm_runinit) {
		pm_runinit = false;
		//Set the value of all the items to the default value
		for(uint8_t i = 0; i < m_items.size(); i++) m_items[i].value = m_items[i].default_value;
	}
	
	//If any items are found with a value different to their default, the edit blink should be running.
	bool runeditblink = false;
	for(uint8_t i = 0; i < m_items.size(); i++) {
		if(m_items[i].value != m_items[i].default_value) {
			runeditblink = true;
			break;
		}
	}
	if(runeditblink && ui_common->dp_right_blinker.currentMode() == userio::Blinker::Mode::Solid) ui_common->dp_right_blinker.run_pattern(pattern::edit);
	else ui_common->dp_right_blinker.set_state(false);

	//Display the item, and if the item is on then light up the left decimal point
	ui_common->segs.write_characters(m_items[pm_currentitem].name, sizeof(Item::name),
		userio::IC_LTD_2601G_11::OVERWRITE_LEFT | (m_items[pm_currentitem].value ? userio::IC_LTD_2601G_11::DISPLAY_LEFT : 0));
}

#endif
