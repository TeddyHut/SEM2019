/*
 * userio.h
 *
 * Created: 3/12/2018 3:58:14 PM
 *  Author: teddy
 */ 

#pragma once

#include <avr/io.h>

#include "utility.h"
#include "timer.h"

namespace hw {
	class GPIOOut {
		//Sets the state of the pin
		virtual void setState(bool const state) = 0;
	};
	class Blinker {
	public:
		//Consider making constexpr
		struct Pattern {
			uint16_t ontime;
			uint16_t offtime;
			uint16_t resttime;
			uint8_t count;
			bool repeat : 1;
			bool inverted : 1;
		};

		//Run the blink pattern specified
		virtual void blinkPattern(Pattern const &pattern) = 0;
		//Get the current pattern
		virtual Pattern currentPattern() const = 0;
	};
	class LED : public utility::UpdateFn, public GPIOOut, public Blinker {
	public:
		enum class Mode : uint8_t {
			Solid,
			Blink,
		};
		void setState(bool const state) override;
		void blinkPattern(Pattern const &pattern) override;
		Pattern currentPattern() const override;
		Mode currentMode() const;
		//Call once per frame (used for blinker)
		void update() override;

		//Pin refers to the bit position, not the mask
		LED(PORT_t &port, uint8_t const pin);
	private:
		//Having this generates a warning because --pack-structs is specified globally
		PORT_t &pm_hwport;
		uint8_t const pm_hwpin;
		Pattern pm_pattern;
		Mode pm_mode = Mode::Solid;
		Timer1k pm_timer;
		struct {
			uint8_t count;
			//State could just be calculated from count 
			bool state : 1;
		} pm_patternstate;

		//Does not change the mode
		void transparent_setState(bool const state);
	};

	class Button : public utility::UpdateFn {
	public:
		bool pressed : 1;
		bool held : 1;
		bool released : 1;

		void update() override;
		Button(PORT_t &port, uint8_t pin, bool const onlevel = false, bool const pullup = true);
	private:
		bool pm_previous : 1;
		PORT_t &pm_hwport;
		uint8_t const pm_hwpin;
	};

	namespace {
		template <typename Stepwatch_t>
		struct Stopwatch_tick;

		template<size_t tickFrequency_c, typename tick_t>
		struct Stopwatch_tick<Stopwatch<tickFrequency_c, tick_t>> {
			using type = tick_t;
		};
	}

	template <typename Stopwatch_t = Stopwatch1k, typename tick_t = typename Stopwatch_tick<Stopwatch_t>::type>
	class ButtonTimer : public utility::UpdateFn {
		using stopwatch_tick_t = typename Stopwatch_tick<Stopwatch_t>::type;
		//Assumed stopwatch_tick_t is an unsigned int of unknown length
		static constexpr tick_t stopwatchMaxTicks_c = utility::fullMask(sizeof(stopwatch_tick_t));
	public:
		//Returns held time
		operator tick_t();
		tick_t heldTime();
		tick_t releasedTime();
		//Returns true if the button has been held for more than time, but only once per press (note: only works for one time value)
		bool pressedFor(tick_t const time);
		//Returns true if the button has been released for more than time, but only once per release (note: only works for one time value)
		bool releasedFor(tick_t const time);
		void update() override;
		//Will handle calling button update function (perhaps should change since ButtonTimer doesn't take full ownership?)
		ButtonTimer(Button &button);
		Button &button;
	private:
		Stopwatch_t pm_stopwatch;
		//Ticks clocked over by overflowing pm_stopwatch
		tick_t pm_ticks = 0;
		bool pm_checked : 1;
	};
	//Type alias (consider moving to global namespace)
	using ButtonTimer1k = ButtonTimer<Stopwatch1k>;
}

template <typename Stopwatch_t, typename tick_t /*= typename Stopwatch_tick<Stopwatch_t>::type*/>
hw::ButtonTimer<Stopwatch_t, tick_t>::operator tick_t()
{
	return heldTime();
}

template <typename Stopwatch_t, typename tick_t /*= typename Stopwatch_tick<Stopwatch_t>::type*/>
tick_t hw::ButtonTimer<Stopwatch_t, tick_t>::heldTime()
{
	//Potential for error here since it's not atomic
	return button.held ? pm_ticks + pm_stopwatch.ticks : 0;
}

template <typename Stopwatch_t, typename tick_t /*= typename Stopwatch_tick<Stopwatch_t>::type*/>
tick_t hw::ButtonTimer<Stopwatch_t, tick_t>::releasedTime()
{
	return button.held ? 0 : pm_ticks + pm_stopwatch.ticks;
}

template <typename Stopwatch_t, typename tick_t /*= typename Stopwatch_tick<Stopwatch_t>::type*/>
bool hw::ButtonTimer<Stopwatch_t, tick_t>::pressedFor(tick_t const time)
{
	if(!pm_checked && heldTime() >= time) {
		return pm_checked = true;
	}
	return false;
}


template <typename Stopwatch_t, typename tick_t /*= typename Stopwatch_tick<Stopwatch_t>::type*/>
bool hw::ButtonTimer<Stopwatch_t, tick_t>::releasedFor(tick_t const time)
{
	if(!pm_checked && releasedTime() >= time) {
		return pm_checked = true;
	}
	return false;
}

template <typename Stopwatch_t, typename tick_t /*= typename Stopwatch_tick<Stopwatch_t>::type*/>
void hw::ButtonTimer<Stopwatch_t, tick_t>::update()
{
	button.update();
	//If the button has just been pressed or released, reset states
	if(button.pressed || button.released) {
		pm_stopwatch = 0;
		pm_ticks = 0;
		pm_checked = false;
	}
	//If stopwatch has reached its max ticks, add this total to pm_ticks
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		if(pm_stopwatch.ticks >= stopwatchMaxTicks_c) {
			pm_ticks += pm_stopwatch.ticks;
			pm_stopwatch = 0;
		}
	}
}

template <typename Stopwatch_t, typename tick_t /*= typename Stopwatch_tick<Stopwatch_t>::type*/>
hw::ButtonTimer<Stopwatch_t, tick_t>::ButtonTimer(Button &button) : button(button)
{
	pm_stopwatch.start();
}
