/*
 * ltd_2601g_11.cpp
 *
 * Created: 17/02/2019 8:36:29 PM
 *  Author: teddy
 */ 

#include "ltd_2601g_11.h"
#include <util/delay.h>

void libmodule::userio::IC_LTD_2601G_11::update()
{
	if(ic_shiftreg == nullptr || font.len == 0) return;
	if(timer) {
		timer = refreshinterval;
		timer.start();

		uint8_t previousdigit = nextdigit == 0 ? 1 : 0;
		//Single line/select mode
		if(digiout_anode[1] == nullptr) {
			//Turn off the current digit
			ic_shiftreg->push_data<uint8_t>(0xff);
			ic_shiftreg->latch_regs();
			//Swap the digit
			digiout_anode[0]->toggle();
			//Push the new data
			ic_shiftreg->push_data(digitdata[nextdigit]);
			ic_shiftreg->latch_regs();
		}
		//Dual line/select mode
		else {
			//Push the digit to the register
			ic_shiftreg->push_data(digitdata[nextdigit]);
			//Turn off the previous digit
			digiout_anode[previousdigit]->set(true);
			//Latch the register
			ic_shiftreg->latch_regs();
			//Turn on the next digit
			digiout_anode[nextdigit]->set(false);
		}
		//Make the next digit the previous digit for next time
		nextdigit = previousdigit;
	}
}

void libmodule::userio::IC_LTD_2601G_11::set_digiout_anode(uint8_t const pos, utility::Output<bool> *const output)
{
	if(pos >= 2)
		hw::panic();
	digiout_anode[pos] = output;
}

void libmodule::userio::IC_LTD_2601G_11::set_74hc595(IC_74HC595 *const ic)
{
	ic_shiftreg = ic;
	//Outputs should always be enabled
	ic_shiftreg->set_outputdrivers(true);
}

void libmodule::userio::IC_LTD_2601G_11::set_pwminterval(uint16_t const interval)
{
	refreshinterval = interval;
}

void libmodule::userio::IC_LTD_2601G_11::set_font(ic_ldt_2601g_11_fontdata::Font const font)
{
	this->font = font;
}

void libmodule::userio::IC_LTD_2601G_11::write_characters(char const str[], uint8_t const len /*= 2*/, uint8_t const dp_flags /*= 0*/)
{
	//If overwrite_dps is true, fully clear segments
	digitdata[0] = ~(digitdata[0]) & (!(dp_flags & OVERWRITE_LEFT)  << 7);
	digitdata[1] = ~(digitdata[1]) & (!(dp_flags & OVERWRITE_RIGHT) << 7);
	
	//Digit index
	uint8_t j = 0;
	for(uint8_t i = 0; i < len; i++) {
		//If past last digit is reached, break
		if(j >= 2) break;

		//Decimal point
		if(str[i] == '.') {
			//Decimal point is always at the end of a digit, hence j++
			digitdata[j++] |= 1 << 7;
			//Next char
			continue;
		}

		//Other character
		digitdata[j] |= find_digit(str[i]);
		if(i + 1 >= len) break;
		//If the next character is not a decimal point, move to next digit
		if(str[i + 1] != '.') j++;
	}
	//Add override decimal points if needed
	if(dp_flags & (OVERWRITE_LEFT)  && (dp_flags & DISPLAY_LEFT))  digitdata[0] |= 1 << 7;
	if(dp_flags & (OVERWRITE_RIGHT) && (dp_flags & DISPLAY_RIGHT)) digitdata[1] |= 1 << 7;

	//Invert digits, since the display uses a common anode (therefore 1 is off)
	digitdata[0] = ~digitdata[0];
	digitdata[1] = ~digitdata[1];
}

void libmodule::userio::IC_LTD_2601G_11::clear()
{
	digitdata[0] = 0xff;
	digitdata[1] = 0xff;
}

libmodule::utility::Output<bool> * libmodule::userio::IC_LTD_2601G_11::get_output_dp_left()
{
	return &output_dp_left;
}

libmodule::utility::Output<bool> * libmodule::userio::IC_LTD_2601G_11::get_output_dp_right()
{
	return &output_dp_right;
}

libmodule::userio::IC_LTD_2601G_11::IC_LTD_2601G_11() : output_dp_left(this, 0), output_dp_right(this, 1)
{
	timer.finished = true;
}

uint8_t libmodule::userio::IC_LTD_2601G_11::find_digit(char const c) const
{
	for(uint8_t i = 0; i < font.len; i++) {
		ic_ldt_2601g_11_fontdata::SerialDigit digit;
		memcpy_P(&digit, font.pgm_character + i, sizeof(ic_ldt_2601g_11_fontdata::SerialDigit));
		if(digit.key == c) return digit.data;
	}
	//If nothing is found, leave digit off
	return 0;
}

libmodule::userio::ic_ldt_2601g_11_fontdata::Font libmodule::userio::ic_ldt_2601g_11_fontdata::decimal_font = {&(decimal_serial::pgm_arr[0]), decimal_len};

void libmodule::userio::IC_LTD_2601G_11::DPOut::set(bool const p)
{
	if(p) parent->digitdata[digit] &= ~(1 << 7);
	else parent->digitdata[digit] |= 1 << 7;
}

libmodule::userio::IC_LTD_2601G_11::DPOut::DPOut(IC_LTD_2601G_11 *const parent, uint8_t const digit) : parent(parent), digit(digit) {}
