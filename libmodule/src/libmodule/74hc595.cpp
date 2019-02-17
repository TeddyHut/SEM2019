/*
 * _74hc595.cpp
 *
 * Created: 17/02/2019 5:59:51 PM
 *  Author: teddy
 */ 

#include "74hc595.h"

#include <util/delay.h>

void libmodule::userio::IC_74HC595::push_buffer(void const *const ptr, size_t const len) const
{
	for(size_t i = 0; i < len; i++) {
		push_data<uint8_t>(static_cast<uint8_t const *>(ptr)[i]);
	}
}

template <>
void libmodule::userio::IC_74HC595::push_data<bool>(bool const data) const
{
	if(digiout_data != nullptr && digiout_clk != nullptr) {
		digiout_data->set(data);
		//_delay_us(100);
		digiout_clk->set(true);
		//_delay_us(100);
		digiout_clk->set(false);
	}
}

template <>
void libmodule::userio::IC_74HC595::push_data<uint8_t>(uint8_t const data) const
{
	if(msbfirst) {
		for(uint8_t i = 0x80; i != 0; i >>= 1) {
			push_data<bool>(data & i);
		}
	}
	else {
		for(uint8_t i = 0x01; i != 0; i <<= 1) {
			push_data<bool>(data & i);
		}
	}
}


void libmodule::userio::IC_74HC595::latch_regs() const
{
	if(digiout_latch != nullptr) {
		digiout_latch->set(true);
		//_delay_us(100);
		digiout_latch->set(false);
	}
}

void libmodule::userio::IC_74HC595::set_outputdrivers(bool const enable) const
{
	if(digiout_drivers != nullptr) {
		digiout_drivers->set(!enable);
	}
}

void libmodule::userio::IC_74HC595::set_digiout_data(utility::Output<bool> *const output)
{
	digiout_data = output;
}

void libmodule::userio::IC_74HC595::set_digiout_clk(utility::Output<bool> *const output)
{
	digiout_clk = output;
}

void libmodule::userio::IC_74HC595::set_digiout_latch(utility::Output<bool> *const output)
{
	digiout_latch = output;
}

void libmodule::userio::IC_74HC595::set_digiout_drivers(utility::Output<bool> *const output)
{
	digiout_drivers = output;
}
