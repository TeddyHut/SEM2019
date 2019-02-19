// bmsresistorselector.cpp : Defines the entry point for the console application.
//

//Determines the best resistors to use for each of the subtractor amplifiers

#include "stdafx.h"

#include <iostream>
#include <vector>
#include <array>
#include <algorithm>
#include <cmath>

namespace config {
	constexpr float max_cell_voltage = 4.3;
	constexpr float min_cell_voltage = 2.8;
	constexpr float reference_voltage = 2.5;
	constexpr float target_current = 0.0002;
	constexpr float target_ratio = reference_voltage / max_cell_voltage;
}

constexpr std::array<float, 25> e24_values = {
	1.0, 1.1, 1.2, 1.3, 1.5, 1.6, 1.8, 2.0, 2.2, 2.4, 2.7, 3.0, 3.3, 3.6, 3.9, 4.3, 4.7, 5.1, 5.6, 6.2, 6.8, 7.5, 8.2, 9.1, 10.0
};

struct Res {
	unsigned int cell = 0;
	float r1 = 0;
	float r2 = 0;
	float ideal_r2 = 0;
	float ratio = 0;
	float max_voltage = 0;
	float max_output = 0;
	float current = 0;
	float min_fb_current = 0;
	float min_fb_voltage = 0;
	void print(std::ostream &out) {
		out << "Cell: " << cell << '\n';
		out << "R1: " << r1 << '\n';
		out << "R2: " << r2 << '\n';
		out << "Ideal R2: " << ideal_r2 << '\n';
		out << "Ratio: " << ratio << '\n';
		out << "Max Voltage: " << max_voltage << '\n';
		out << "Max Output: " << max_output << '\n';
		out << "Current: " << current * 1000.0f << '\n';
		out << "Min fb current: " << min_fb_current * 1000.0f << '\n';
		out << "Min fb voltage: " << min_fb_voltage << '\n';
		out << '\n' << std::endl;
	}
};

Res determine_cell(unsigned int const cell) {
	Res res;
	res.cell = cell;
	res.max_voltage = (cell + 1) * config::max_cell_voltage;
	for (float i = 1.0f;; i *= 10.0f) {
		for (auto r1 : e24_values) {
			//Get the correct multiplier
			res.r1 = r1 * i;
			//Determine R2 for a perfect ratio
			res.ideal_r2 = res.r1 / config::target_ratio;
			//Determine R2 divisor for comparison of e24 (e.g. if R2 is 5600 divisor will be 1000)
			float r2_divisor = std::pow(10.0f, std::floor(std::log10(res.ideal_r2)));
			//Determine the actual R2
			auto itr = std::find_if(e24_values.begin(), e24_values.end(), [&](auto p0) { return p0 >= res.ideal_r2 / r2_divisor; });
			res.r2 = *itr * r2_divisor;
			//Calculate other values
			res.ratio = res.r1 / res.r2;
			res.max_output = config::max_cell_voltage * res.ratio;
			res.current = res.max_voltage / (res.r1 + res.r2);

			//Calculate FB voltage difference at min
			//Minimum cell output relative to GND
			float min_voltage = (cell + 1) * config::min_cell_voltage;
			//Minimum output of opamp
			float min_output = config::min_cell_voltage * res.ratio;
			//Minimum voltage at opamp nodes
			float min_node = min_voltage * (res.r2 / (res.r1 + res.r2));
			//Voltage across Vfb
			res.min_fb_voltage = min_node - min_output;
			//Current across Vfb
			res.min_fb_current = min_voltage / res.r2;
			if (res.current <= config::target_current) return res;
		}
	}
}

int main()
{
	for(unsigned int i = 0; i < 6; i++) {
		determine_cell(i).print(std::cout);
	}
    return 0;
}

