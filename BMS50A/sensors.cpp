/*
 * sensors.cpp
 *
 * Created: 21/04/2019 1:55:14 PM
 *  Author: teddy
 */ 

#include "sensors.h"
#include "config.h"
#include <math.h>

bms::Sensor_t *bms::snc::cellvoltage[6];
bms::Sensor_t *bms::snc::temperature;
bms::sensor::ACS_CurrentSensor *bms::snc::current1A;
bms::sensor::ACS_CurrentSensor *bms::snc::current12A;
bms::sensor::ACS_CurrentSensor *bms::snc::current50A;
bms::sensor::CurrentOptimised *bms::snc::current_optimised;
bms::DigiSensor_t *bms::snc::batterypresent;

bms::Sensor_t *&vcc = bms::snc::cellvoltage[0];
volatile float zero = 0.0f;

void bms::snc::setup()
{
	cellvoltage[0] = new bms::sensor::CellVoltage(ADC_MUXPOS_AIN0_gc, 15.0f / (11.0f + 15.0f));
	cellvoltage[1] = new bms::sensor::CellVoltage(ADC_MUXPOS_AIN1_gc, 16.0f / 30.0f);
	cellvoltage[2] = new bms::sensor::CellVoltage(ADC_MUXPOS_AIN2_gc, 24.0f / 43.0f);
	cellvoltage[3] = new bms::sensor::CellVoltage(ADC_MUXPOS_AIN3_gc, 33.0f / 62.0f);
	cellvoltage[4] = new bms::sensor::CellVoltage(ADC_MUXPOS_AIN4_gc, 43.0f / 75.0f);
	cellvoltage[5] = new bms::sensor::CellVoltage(ADC_MUXPOS_AIN5_gc, 51.0f / 91.0f);
	temperature = new bms::sensor::BatteryTemperature(ADC_MUXPOS_AIN7_gc);
	current1A = new bms::sensor::Current1A(ADC_MUXPOS_AIN12_gc);
	current12A = new bms::sensor::Current12A(ADC_MUXPOS_AIN13_gc);
	current50A = new bms::sensor::Current50A(ADC_MUXPOS_AIN6_gc);
	current_optimised = new bms::sensor::CurrentOptimised;
	batterypresent = new bms::sensor::Battery(ADC_MUXPOS_AIN14_gc);
}

void bms::snc::cycle_read()
{
	for(uint8_t i = 0; i < 6; i++) cellvoltage[i]->cycle_read();
	temperature->cycle_read();
	current1A->cycle_read();
	current12A->cycle_read();
	current50A->cycle_read();
	current_optimised->cycle_read();
	batterypresent->cycle_read();
}

void bms::snc::calibrate()
{
	current1A->calibrate();
	current12A->calibrate();
	current50A->calibrate();
}

template <typename T>
constexpr T calculation_adc_pinvoltage(T const vref, T const max, T const value) {
	return (value / max) * vref;
}

template <typename T>
constexpr T calculation_opamp_subtractor_input_vp(T const input_vm, T const output, T const r1, T const rf) {
	return input_vm + ((output * r1) / rf);
}

template <typename T>
constexpr T calculation_voltagedivier_output(T const input, T const r1, T const r2) {
	return input * (r2 / (r1 + r2));
}

template <typename T>
constexpr T calculation_currentsensor_current(T const output, T const min_current, T const max_current, T const min_output, T const max_output) {
	return ((max_current - min_current) / (max_output - min_output)) * (output - ((min_output + max_output) / 2));
}

template <typename T>
constexpr T calculation_mv_to_degreesC(T const mv) {
	//Taken from temperature sensor datasheet
	T result = 2230.8 - mv;
	result *= (4 * 0.00433);
	result += square(-13.582);
	result = 13.582 - sqrt(result);
	result /= (2 * -0.00433);
	return result + 30;
}

bms::sensor::CellVoltage::CellVoltage(ADC_MUXPOS_t const muxpos, float const scaler)
 : ch_adc(muxpos, ADC_REFSEL_INTREF_gc, VREF_ADC0REFSEL_2V5_gc, ADC_SAMPNUM_ACC16_gc), scaler_recipracle(1.0f / scaler) {}


float bms::sensor::CellVoltage::get_sensor_value()
{
	//If there have not been any samples yet, return 3.7
	if(ch_adc.get_samplecount() == 0) return 3.7f;
	return calculation_adc_pinvoltage<float>(2.5f, 0x3ff, ch_adc.get()) * scaler_recipracle;
}

float bms::sensor::BatteryTemperature::get_sensor_value()
{
	//Return 25 degrees if no samples
	if(ch_adc.get_samplecount() == 0) return 25.0f;
	//Scalar reciprocal is (R1 + R2) / R2
	constexpr float scaler_recipracle = (10.0f + 22.0f) / 22.0f; 
	//For readability
	float sensorvoltage_mv = calculation_adc_pinvoltage<float>(2.5f, 0x3ff, ch_adc.get()) * scaler_recipracle * 1000.0f;
	return calculation_mv_to_degreesC<float>(sensorvoltage_mv);
}

bms::sensor::BatteryTemperature::BatteryTemperature(ADC_MUXPOS_t const muxpos)
 : ch_adc(muxpos, ADC_REFSEL_INTREF_gc, VREF_ADC0REFSEL_2V5_gc, ADC_SAMPNUM_ACC16_gc) {}

float bms::sensor::ACS_CurrentSensor::get_sensor_value()
{
	float sensorvoltage_calibrated = get_sensorvoltage_unaltered() + (vcc->get() * calibration_sensoroutput_offset_linear_scaled);
	return calculation_currentsensor_current<float>(sensorvoltage_calibrated, sensor_current_min, sensor_current_max, 0.3, vcc->get() - 0.3);
}

float bms::sensor::ACS_CurrentSensor::get_and_calibrate()
{
	volatile float current = get();
	if(current < 0.0f) {
		calibrate();
		return 0.0f;
	}
	return current;
}

void bms::sensor::ACS_CurrentSensor::calibrate()
{
	//When calibrate is called, there should be 0A of current.
	//Solve equation so that expected_output = sensorvoltage + (x * vcc)
	float cvcc = vcc->get();
	float expected_output = cvcc / 2;
	calibration_sensoroutput_offset_linear_scaled = (expected_output - get_sensorvoltage_unaltered()) / cvcc;
}

bms::sensor::ACS_CurrentSensor::ACS_CurrentSensor(float const min_current, float const max_current)
: sensor_current_min(min_current), sensor_current_max(max_current) {}

bms::sensor::Current1A::Current1A(ADC_MUXPOS_t const muxpos)
 : ACS_CurrentSensor(-12.5, 12.5), ch_adc(muxpos, ADC_REFSEL_VDDREF_gc, VREF_ADC0REFSEL_2V5_gc, ADC_SAMPNUM_ACC64_gc) {}

float bms::sensor::Current1A::get_sensorvoltage_unaltered() const
{
	if(ch_adc.get_samplecount() == 0) return vcc->get() / 2;
	constexpr float scaler_recipracle = 1.0f / (1.0f + 51.0f / 30.0f);
	float pinvoltage = calculation_adc_pinvoltage<float>(vcc->get(), 0x3ff, ch_adc.get());
	float subtractor_output = pinvoltage * scaler_recipracle;
	float input_vm = calculation_voltagedivier_output<float>(vcc->get(), config::resistor_r1, config::resistor_r2);
	return calculation_opamp_subtractor_input_vp<float>(input_vm, subtractor_output, config::resistor_r37_r38, 24);
}

bms::sensor::Current12A::Current12A(ADC_MUXPOS_t const muxpos)
 : ACS_CurrentSensor(-12.5, 12.5), ch_adc(muxpos, ADC_REFSEL_VDDREF_gc, VREF_ADC0REFSEL_2V5_gc, ADC_SAMPNUM_ACC64_gc) {}

float bms::sensor::Current12A::get_sensorvoltage_unaltered() const
{
	if(ch_adc.get_samplecount() == 0) return vcc->get() / 2;
	float pinvoltage = calculation_adc_pinvoltage<float>(vcc->get(), 0x3ff, ch_adc.get());
	float input_vm = calculation_voltagedivier_output<float>(vcc->get(), config::resistor_r1, config::resistor_r2);
	return calculation_opamp_subtractor_input_vp<float>(input_vm, pinvoltage, config::resistor_r37_r38, 24);
}

bms::sensor::Current50A::Current50A(ADC_MUXPOS_t const muxpos)
 : ACS_CurrentSensor(-75, 75), ch_adc(muxpos, ADC_REFSEL_VDDREF_gc, VREF_ADC0REFSEL_2V5_gc, ADC_SAMPNUM_ACC64_gc) {}

float bms::sensor::Current50A::get_sensorvoltage_unaltered() const
{
	if(ch_adc.get_samplecount() == 0) return vcc->get() / 2;
	float pinvoltage = calculation_adc_pinvoltage<float>(vcc->get(), 0x3ff, ch_adc.get());
	float input_vm = calculation_voltagedivier_output<float>(vcc->get(), config::resistor_r1, config::resistor_r2);
	return calculation_opamp_subtractor_input_vp<float>(input_vm, pinvoltage, config::resistor_r55_r56, 24);
}

bool bms::sensor::Battery::get_sensor_value()
{
	//Return not connected if no samples
	if(ch_adc.get_samplecount() == 0) return false;
	constexpr float scalar_recipracle = (680.0f + 30.0f) / 30.0f;
	//Get battery voltage
	float batteryvoltage = calculation_adc_pinvoltage<float>(2.5f, 0x3ff, ch_adc.get()) * scalar_recipracle;
	//If battery voltage is higher than 12, consider connected
	return batteryvoltage >= 12.0f;
}

bms::sensor::Battery::Battery(ADC_MUXPOS_t const muxpos) : ch_adc(muxpos, ADC_REFSEL_INTREF_gc, VREF_ADC0REFSEL_2V5_gc) {}

float bms::sensor::CurrentOptimised::get_sensor_value()
{
	float currents[3] = {snc::current1A->get(), snc::current12A->get(), snc::current50A->get()};
	if(currents[2] <= 11.0f) {
		if(currents[1] <= config::current_cutoff_sensor_1A) {
			return currents[0];
		}
		return currents[1];
	}
	return currents[2];
}

float bms::sensor::CurrentOptimised::get_and_calibrate()
{
	float currents[3] = {snc::current1A->get_and_calibrate(), snc::current12A->get_and_calibrate(), snc::current50A->get_and_calibrate()};
	if(currents[2] <= 11.0f) {
		if(currents[1] <= config::current_cutoff_sensor_1A) {
			return currents[0];
		}
		return currents[1];
	}
	return currents[2];
}
