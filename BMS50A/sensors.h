/*
 * sensors.h
 *
 * Created: 21/04/2019 12:34:12 PM
 *  Author: teddy
 */ 

#pragma once

#include <libmodule/utility.h>
#include <generalhardware.h>

namespace bms {
	using Sensor_t = libmodule::utility::Input<float>;
	using DigiSensor_t = libmodule::utility::Input<bool>;
	namespace sensor {
		struct CellVoltage : public Sensor_t {
			float get() const override;
			CellVoltage(ADC_MUXPOS_t const muxpos, float const scaler);
		private:
			libmicavr::ADCChannel ch_adc;
			float scaler_recipracle;
		};
		
		struct BatteryTemperature : public Sensor_t {
			float get() const override;
			BatteryTemperature(ADC_MUXPOS_t const muxpos);
		private:
			libmicavr::ADCChannel ch_adc;
		};

		struct ACS_CurrentSensor : public Sensor_t {
			float get() const override;
			float get_and_calibrate();
			void calibrate();
			ACS_CurrentSensor(float const min_current, float const max_current);
		private:
			virtual float get_sensorvoltage_unaltered() const = 0;
			float const sensor_current_min;
			float const sensor_current_max;
			float calibration_sensoroutput_offset_linear_scaled = 0;
		};

		struct Current1A : public ACS_CurrentSensor {
			Current1A(ADC_MUXPOS_t const muxpos);
		private:
			float get_sensorvoltage_unaltered() const override;
			libmicavr::ADCChannel ch_adc;
		};	

		struct Current12A : public ACS_CurrentSensor {
			Current12A(ADC_MUXPOS_t const muxpos);
		private:
			float get_sensorvoltage_unaltered() const override;
			libmicavr::ADCChannel ch_adc;
		};

		struct Current50A : public ACS_CurrentSensor {
			Current50A(ADC_MUXPOS_t const muxpos);
		private:
			float get_sensorvoltage_unaltered() const override;
			libmicavr::ADCChannel ch_adc;
		};

		struct CurrentOptimised : public Sensor_t {
			float get() const override;
			float get_and_calibrate();
		};

		struct Battery : public DigiSensor_t {
			bool get() const override;
			Battery(ADC_MUXPOS_t const muxpos);
		private:
			libmicavr::ADCChannel ch_adc;
		};
	}
	namespace snc {
		extern Sensor_t *cellvoltage[6];
		extern Sensor_t *temperature;
		extern sensor::ACS_CurrentSensor *current1A;
		extern sensor::ACS_CurrentSensor *current12A;
		extern sensor::ACS_CurrentSensor *current50A;
		extern sensor::CurrentOptimised *current_optimised;
		extern DigiSensor_t *batterypresent;
		//Allocates memory for sensors
		void setup();
		//Calibrates sensors that can be calibrated
		void calibrate();
	}
}
