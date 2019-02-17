/*
 * _74hc595.h
 *
 * Created: 17/02/2019 5:59:19 PM
 *  Author: teddy
 */ 

#pragma once

#include "utility.h"

namespace libmodule {
	namespace userio {
		//Currently only supports output
		class IC_74HC595 {
		public:
			//Pushes a buffer
			void push_buffer(void const *const ptr, size_t const len) const;

			//Makes sequential calls to push_data<uint8_t>
			template <typename T>
			void push_data(T const data) const;
			template <typename T>
			void push_data(T const data, bool const msbfirst);

			//Latches the registers
			void latch_regs() const;

			//Enables or disables the output drivers
			void set_outputdrivers(bool const enable) const;

			void set_digiout_data(utility::Output<bool> *const output);
			void set_digiout_clk(utility::Output<bool> *const output);
			void set_digiout_latch(utility::Output<bool> *const output);
			void set_digiout_drivers(utility::Output<bool> *const output);
		private:
			utility::Output<bool> *digiout_data = nullptr;
			utility::Output<bool> *digiout_clk = nullptr;
			utility::Output<bool> *digiout_latch = nullptr;
			utility::Output<bool> *digiout_drivers = nullptr;
			bool msbfirst = true;
		};
		//TODO: Add ability for selecting endian form the push_data

		//Writes one bit to the 74HC595
		template <>
		void IC_74HC595::push_data<bool>(bool const data) const;
		//Bit-bangs data onto the 74HC595, MSB first
		template <>
		void IC_74HC595::push_data<uint8_t>(uint8_t const data) const;
	}
}


template <typename T>
void libmodule::userio::IC_74HC595::push_data(T const data) const
{
	for(size_t i = 0; i < sizeof(T); i++) {
		push_data<uint8_t>(reinterpret_cast<uint8_t const *>(&data)[i]);
	}
}


template <typename T>
void libmodule::userio::IC_74HC595::push_data(T const data, bool const msbfirst)
{
	//Doing it this way since template specializations have to be exactly the same aside from the type (even reference differences stuffs it up)
	//May be able to partially specialize one with a reference and then rely on SFINAE to block out the non-reference one, but eh.
	msbfirst = msbfirst;
	push_data(data);
	msbfirst = true;
}
