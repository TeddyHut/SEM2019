/*
 * mux.h
 *
 * Created: 13/04/2019 8:46:10 PM
 *  Author: teddy
 */ 

#pragma once

#include <string.h>
#include "utility.h"
#include "timer.h"

namespace libmodule {
	namespace userio {
		template <typename select_t, uint8_t bitcount_c>
		class BinaryOutput {
		public:
			void set_digiout_bit(uint8_t const bit, utility::Output<bool> *const output);
			void set_value(select_t const value);
			BinaryOutput();
		private:
			utility::Output<bool> *digiout_bit[bitcount_c];
		};

		template <typename>
		class MultiplexDigitalInput;

		template <typename select_t, uint8_t bitcount_c>
		class MultiplexDigitalInput<BinaryOutput<select_t, bitcount_c>> : public utility::Input<bool> {
		public:
			using BinaryOutput_t = BinaryOutput<select_t, bitcount_c>;
			bool get() const override;
			MultiplexDigitalInput(utility::Input<bool> const *const commoninput, BinaryOutput_t *const binaryout, uint8_t const index);
		private:
			utility::Input<bool> const *pm_commoninput;
			BinaryOutput_t *pm_binaryout;
			uint8_t pm_index;
		};
	}
}

template <typename select_t, uint8_t bitcount_c>
void libmodule::userio::BinaryOutput<select_t, bitcount_c>::set_digiout_bit(uint8_t const bit, utility::Output<bool> *const output)
{
	if(bit >= bitcount_c) hw::panic();
	digiout_bit[bit] = output;
}

template <typename select_t, uint8_t bitcount_c>
void libmodule::userio::BinaryOutput<select_t, bitcount_c>::set_value(select_t const value)
{
	for(uint8_t i = 0; i < bitcount_c; i++) {
		if(digiout_bit[i] == nullptr)
			hw::panic();
		digiout_bit[i]->set(value & 1 << i);
	}
}

template <typename select_t, uint8_t bitcount_c>
libmodule::userio::BinaryOutput<select_t, bitcount_c>::BinaryOutput()
{
	memset(digiout_bit, 0, sizeof digiout_bit);
}

template <typename select_t, uint8_t bitcount_c>
bool libmodule::userio::MultiplexDigitalInput<libmodule::userio::BinaryOutput<select_t, bitcount_c>>::get() const
{
	//Could put a check in for pm_index >= pow(bitcount_c, 2), but eh.
	if(pm_binaryout == nullptr || pm_commoninput == nullptr) hw::panic();
	pm_binaryout->set_value(pm_index);
	return pm_commoninput->get();
}

template <typename select_t, uint8_t bitcount_c>
libmodule::userio::MultiplexDigitalInput<libmodule::userio::BinaryOutput<select_t, bitcount_c>>::MultiplexDigitalInput
 (utility::Input<bool> const *const commoninput, BinaryOutput_t *const binaryout, uint8_t const index)
 : pm_commoninput(commoninput), pm_binaryout(binaryout), pm_index(index) {}
