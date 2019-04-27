/*
 * ltd_2601g_11.h
 *
 * Created: 17/02/2019 8:35:53 PM
 *  Author: teddy
 */ 

#pragma once

#include <avr/pgmspace.h>

#include "utility.h"
#include "timer.h"
#include "74hc595.h"

namespace libmodule {
	namespace userio {
		namespace ic_ldt_2601g_11_fontdata {
			//Digit is used for constexpr array initialization (order makes it so that numbers can look correct in array)
			struct ConstDigit {
				char key;
				bool a : 1;
				bool f : 1, b : 1;
				bool g : 1;
				bool e : 1, c : 1;
				bool d : 1, dp : 1;
			};
			struct SerialDigit {
				char key;
				uint8_t data;
			};
			struct Font {
				SerialDigit const *pgm_character = nullptr;
				uint8_t len = 0;
			};
		}

		//Configured for a single 74HC595 with the common anodes connected to P-FETs
		class IC_LTD_2601G_11 {
		public:
			void update();
			
			//pos is indexed from left to right
			void set_digiout_anode(uint8_t const pos, utility::Output<bool> *const output);
			void set_74hc595(IC_74HC595 *const ic);
			void set_pwminterval(uint16_t const interval);
			void set_font(ic_ldt_2601g_11_fontdata::Font const font);
			
			enum Flags_DecimalPoint {
				OVERWRITE_LEFT  = 0b0001,
				OVERWRITE_RIGHT = 0b0010, 
				DISPLAY_LEFT    = 0b0100,
				DISPLAY_RIGHT   = 0b1000,
			};
			//Write characters in str to display
			//str can have decimal points (hence size of 4)
			//If 'overwrite_flags' is true for a decimal point, the decimal point can be manually entered using dpl and dpr, respectively.
			//str[0] corresponds to the character on the left
			void write_characters(char const str[], uint8_t const len = 2, uint8_t const dp_flags = 0);
			//Clears the display
			void clear();

			utility::Output<bool> *get_output_dp_left();
			utility::Output<bool> *get_output_dp_right();

			IC_LTD_2601G_11();
		private:
			struct DPOut : public utility::Output<bool> {
				void set(bool const p) override;
				DPOut(IC_LTD_2601G_11 *const parent, uint8_t const digit);
			private:
				IC_LTD_2601G_11 *parent = nullptr;
				uint8_t digit = 0;
			} output_dp_left, output_dp_right;
			utility::Output<bool> *digiout_anode[2] = {nullptr, nullptr};
			IC_74HC595 *ic_shiftreg = nullptr;
			uint16_t refreshinterval = 1000 / 120;
			ic_ldt_2601g_11_fontdata::Font font;
			Timer1k timer;
			uint8_t digitdata[2] = {0xff, 0xff};
			uint8_t nextdigit = 0;

			uint8_t find_digit(char const c) const;
		};

		namespace ic_ldt_2601g_11_fontdata {
			constexpr ConstDigit decimal_const[] = {
				{'0', 1,
					1,  1,
					  0,
					1,  1,
					  1,  0 },
				{'1', 0,
					0,  1,
					  0,
					0,  1,
					  0,  0 },
				{'2', 1,
					0,  1,
					  1,
					1,  0,
					  1,  0 },
				{'3', 1,
					0,  1,
					  1,
					0,  1,
					  1,  0 },
				{'4', 0,
					1,  1,
					  1,
					0,  1,
					  0,  0 },
				{'5', 1,
					1,  0,
					  1,
					0,  1,
					  1,  0 },
				{'6', 1,
					1,  0,
					  1,
					1,  1,
					  1,  0 },
				{'7', 1,
					0,  1,
					  0,
					0,  1,
					  0,  0 },
				{'8', 1,
					1,  1,
					  1,
					1,  1,
					  1,  0 },
				{'9', 1,
					1,  1,
					  1,
					0,  1,
					  1,  0 }
			};
			constexpr size_t decimal_len = sizeof decimal_const / sizeof(ConstDigit);
			
			constexpr SerialDigit conv_constdigit_to_serialdigit(ConstDigit const p) {
				return {p.key, static_cast<uint8_t>(p.a << 0 | p.b << 1 | p.c << 2 | p.d << 3 | p.e << 4 | p.f << 5 | p.g << 6 | p.dp << 7)};
			}

			template <size_t ...seq>
			struct decimal_serial_t {
				static SerialDigit const pgm_arr[sizeof...(seq)];
			};
			template <size_t ...seq>
			SerialDigit const decimal_serial_t<seq...>::pgm_arr[] PROGMEM = {conv_constdigit_to_serialdigit(decimal_const[seq])...};
			using decimal_serial = decimal_serial_t<0, 1, 2, 3, 4, 5, 6, 7, 8, 9>;			
			extern Font decimal_font;
		}
	}
}
