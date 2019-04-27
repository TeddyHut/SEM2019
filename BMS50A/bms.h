/*
 * bms.h
 *
 * Created: 20/04/2019 2:57:39 PM
 *  Author: teddy
 */ 

#pragma once

#include <libmodule/utility.h>
#include <libmodule/userio.h>

#include "sensors.h"

namespace bms {
	enum class ConditionID {
		None,
		Generic,
		CellUndervoltage_0,
		CellUndervoltage_1,
		CellUndervoltage_2,
		CellUndervoltage_3,
		CellUndervoltage_4,
		CellUndervoltage_5,
		CellOvervoltage_0,
		CellOvervoltage_1,
		CellOvervoltage_2,
		CellOvervoltage_3,
		CellOvervoltage_4,
		CellOvervoltage_5,
		OverTemperature,
		OverCurrent,
		Battery,
	};
	struct Condition {
		virtual bool get_ok() const = 0;

		uint16_t cd_timeout;
		libmodule::Timer1k cd_timer;
		ConditionID cd_id;
		Condition(ConditionID const id, uint16_t const timeout);
	};
	class ConditionDaemon : public libmodule::utility::Input<bool> {
	public:
		Condition *get_signal_cause() const;
		bool get() const override;
		void update();

		void set_enabled(bool const enable);

		ConditionDaemon();

		libmodule::utility::Vector<Condition *> conditions;
	private:
		libmodule::utility::Output<bool> *digiout_signal = nullptr;
		Condition *signal_cause = nullptr;
		bool enabled : 1;
		bool errorsignal : 1;
	};

	namespace condition {
		struct CellUndervoltage : public Condition {
			bool get_ok() const override;
			CellUndervoltage(float const min, uint8_t const index);
			float min;
			uint8_t index;
		};
		struct CellOvervoltage : public Condition {
			bool get_ok() const override;
			CellOvervoltage(float const max, uint8_t const index);
			float max;
			uint8_t index;
		};
		struct OverTemperature : public Condition {
			bool get_ok() const override;
			OverTemperature(float const max);
			float max;
		};
		struct OverCurrent : public Condition {
			bool get_ok() const override;
			OverCurrent(float const max);	
			float max;
		};
		struct Battery : public Condition {
			bool get_ok() const override;
			Battery();
			bool enabled = false;
		};
	}
	
	struct Settings {
		float cell_min;
		float cell_max;
		float temperature_max;
		float current_max;
		uint16_t timeout_cell_uv;
		uint16_t timeout_cell_ov;
		uint16_t timeout_temperature;
		uint16_t timeout_current;
	};

	class BMS {
	public:
		using DigiOut_t = libmodule::utility::Output<bool>;
		void update();

		//When enabling will
		// - Reset condition daemon
		// - Enable the battery presence condition
		// - Flip the relay on
		//When disabling will
		// - Reset condition daemon
		// - Disable the battery presence condition
		// - Flip the relay off
		void set_enabled(bool const enable);
		void set_digiout_relayLeft(DigiOut_t *const p);
		void set_digiout_relayRight(DigiOut_t *const p);

		//Returns the current error signal, even if in disabled mode
		bool get_error_signal() const;
		//Returns whether the BMS is enabled or not
		bool get_enabled() const;
		//Gets the current error ID, even if in disabled mode
		ConditionID get_current_error_id() const;
		//Returns the ConditionID that caused a flip from enabled to disabled
		ConditionID get_disabled_error_id() const;
		BMS();
	private:
		void flip_relay(bool const on);

		//States
		bool enabled = false;
		ConditionID disabled_error_id = ConditionID::None;

		//Pins
		libmodule::userio::BlinkerTimer1k btimer_relayLeft;
		libmodule::userio::BlinkerTimer1k btimer_relayRight;

		//Conditions
		ConditionDaemon conditiondaemon;
		condition::CellUndervoltage cd_cell_uv[6];
		condition::CellOvervoltage cd_cell_ov[6];
		condition::OverTemperature cd_overtemperature;
		condition::OverCurrent cd_overcurrent;
		condition::Battery cd_battery;

		static libmodule::userio::Blinker::Pattern pattern_flip;
	};
}
