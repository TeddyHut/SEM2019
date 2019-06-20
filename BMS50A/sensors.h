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
	//Should really just call this SensorBuffer or just type buffer or something
	template <typename T>
	struct CycleSensor : public libmodule::utility::Input<T> {
		//Will return the value of the sensor for this cycle
		T get() const override;
		//Will read the sensor value from hardware and store it for the cycle
		void cycle_read();
	protected:
		//Should return the value of the sensor as found in hardware
		virtual T get_sensor_value() = 0;
		T cycle_value;
	};

	using Sensor_t = CycleSensor<float>;
	using DigiSensor_t = CycleSensor<bool>;
	namespace sensor {
		struct CellVoltage : public Sensor_t {
			CellVoltage(ADC_MUXPOS_t const muxpos, float const scaler);
		private:
			float get_sensor_value() override;
			libmicavr::ADCChannel ch_adc;
			float scaler_recipracle;
		};
		
		struct BatteryTemperature : public Sensor_t {
			BatteryTemperature(ADC_MUXPOS_t const muxpos);
		private:
			float get_sensor_value() override;
			libmicavr::ADCChannel ch_adc;
		};

		struct ACS_CurrentSensor : public Sensor_t {
			float get_and_calibrate();
			void calibrate();
			ACS_CurrentSensor(float const min_current, float const max_current);
		private:
			float get_sensor_value() override;
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
			float get_sensor_value() override;
			float get_and_calibrate();
		};

		struct Battery : public DigiSensor_t {
			bool get_sensor_value() override;
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
		//Reads new values into sensors
		void cycle_read();
		//Calibrates sensors that can be calibrated
		void calibrate();
	}
}

template <typename T>
T bms::CycleSensor<T>::get() const
{
	return cycle_value;
}

template <typename T>
void bms::CycleSensor<T>::cycle_read()
{
	cycle_value = get_sensor_value();
}
