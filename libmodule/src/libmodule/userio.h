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

namespace libmodule {
namespace userio {
	namespace {
		template <typename Chrono_t>
		struct Chrono_tick;

		//Determines tick_t from either a Stopwatch or Timer
		template<template <size_t, typename> class Chrono_t, size_t tickFrequency_c, typename tick_t>
		struct Chrono_tick<Chrono_t<tickFrequency_c, tick_t>> {
			using type = tick_t;
		};
	}

	namespace Blinker {
		struct Pattern {
			uint16_t ontime;
			uint16_t offtime;
			uint16_t resttime;
			uint8_t count;
			bool repeat : 1;
			bool inverted : 1;
		};

		enum class Mode : uint8_t {
			Solid,
			Blink,
		};
	}

	//Note: Unlike ButtonTimer, BlinkerTimer does not support timer overflows
	template <typename Timer_t = Timer1k>
	class BlinkerTimer  {
	public:
		using Pattern = Blinker::Pattern;
		using Mode = Blinker::Mode;
		using Output = utility::Output<bool>;
		//Consider moving pattern out of template
		
		void update();
		
		void set_state(bool const state);
		void run_pattern(Pattern const &pattern);

		Pattern currentPattern() const;
		Mode currentMode() const;

		BlinkerTimer(Output *const out = nullptr);
		Output *pm_out;
	private:
		Timer_t pm_timer;
		Pattern pm_pattern;
		Mode pm_mode;
		struct {
			uint8_t count;
			//State could just be calculated from count
			bool state : 1;
		} pm_patternstate;
	};

	template <typename Stopwatch_t = Stopwatch1k, typename tick_t = typename Chrono_tick<Stopwatch_t>::type, typename in_t = bool>
	class ButtonTimer  {
		using stopwatch_tick_t = typename Chrono_tick<Stopwatch_t>::type;
		//Assumed stopwatch_tick_t is an unsigned int of unknown length
		static constexpr tick_t stopwatchMaxTicks_c = utility::fullMask(sizeof(stopwatch_tick_t));
	public:
		using Input = utility::Input<in_t>;
		using InStates = utility::InStates<in_t>;
		//Will handle calling button update function (perhaps should change since ButtonTimer doesn't take full ownership?)
		void update();
		//Sets the input to use
		void set_input(Input const *const input);
		Input const *get_input() const;
		//Returns held time
		operator tick_t();
		tick_t heldTime();
		tick_t releasedTime();
		//Returns true if the button has been held for more than time, but only once per press (note: only works for one time value)
		bool pressedFor(tick_t const time);
		//Returns true if the button has been released for more than time, but only once per release (note: only works for one time value)
		bool releasedFor(tick_t const time);
		ButtonTimer(Input const *const input = nullptr);
	private:
		InStates pm_instates;
		Stopwatch_t pm_stopwatch;
		//Ticks clocked over by overflowing pm_stopwatch
		tick_t pm_ticks = 0;
		bool pm_checked : 1;
	};

	//Type alias (consider moving to global namespace)
	using BlinkerTimer1k = BlinkerTimer<Timer1k>;
	using ButtonTimer1k = ButtonTimer<Stopwatch1k>;
} //userio

} //libmodule


template <typename Timer_t /*= Timer1k*/>
void libmodule::userio::BlinkerTimer<Timer_t>::update()
{
	//TODO: Add out == nullptr check
	if(pm_mode == Mode::Blink) {
		if(pm_timer.finished) {
			//Start off state
			if(pm_patternstate.state) {
				//If the LED was on, turn off
				pm_out->set(pm_pattern.inverted);
				//Wait for the offtime
				pm_timer = pm_pattern.offtime;
				pm_patternstate.count++;
			}
			//Start on state
			else {
				if(pm_patternstate.count >= pm_pattern.count) {
					//If it was a oneshot, set back to solid and exit
					if(!pm_pattern.repeat) {
						pm_mode = Mode::Solid;
						return;
					}
					//If it is to repeat, then reset the states
					pm_patternstate.count = 0;
					pm_patternstate.state = true;
					pm_timer = pm_pattern.resttime;
				}
				else {
					//If the LED was off, turn on
					pm_out->set(!pm_pattern.inverted);
					pm_timer = pm_pattern.ontime;
				}
			}
			pm_patternstate.state = !pm_patternstate.state;
			pm_timer.start();
		}
	}
}

//---BlinkerTimer---

template <typename Timer_t /*= Timer1k*/>
void libmodule::userio::BlinkerTimer<Timer_t>::set_state(bool const state)
{
	pm_mode = Mode::Solid;
	pm_out->set(state);
}


template <typename Timer_t /*= Timer1k*/>
void libmodule::userio::BlinkerTimer<Timer_t>::run_pattern(Pattern const &pattern)
{
	pm_pattern = pattern;
	pm_mode = Mode::Blink;
	pm_patternstate.count = 0;
	pm_patternstate.state = false;
	pm_timer.reset();
	pm_timer.finished = true;
}

template <typename Timer_t /*= Timer1k*/>
libmodule::userio::Blinker::Pattern libmodule::userio::BlinkerTimer<Timer_t>::currentPattern() const
{
	return pm_pattern;
}

template <typename Timer_t /*= Timer1k*/>
libmodule::userio::Blinker::Mode libmodule::userio::BlinkerTimer<Timer_t>::currentMode() const
{
	return pm_mode;
}


template <typename Timer_t /*= Timer1k*/>
libmodule::userio::BlinkerTimer<Timer_t>::BlinkerTimer(Output *const out /*= nullptr*/) : pm_out(out) {}

//---ButtonTimer---

template <typename Stopwatch_t, typename tick_t /*= typename Stopwatch_tick<Stopwatch_t>::type*/, typename in_t>
libmodule::userio::ButtonTimer<Stopwatch_t, tick_t, in_t>::operator tick_t()
{
	return heldTime();
}

template <typename Stopwatch_t, typename tick_t /*= typename Stopwatch_tick<Stopwatch_t>::type*/, typename in_t>
tick_t libmodule::userio::ButtonTimer<Stopwatch_t, tick_t, in_t>::heldTime()
{
	//Potential for error here since it's not atomic
	return pm_instates.held ? pm_ticks + pm_stopwatch.ticks : 0;
}

template <typename Stopwatch_t, typename tick_t /*= typename Stopwatch_tick<Stopwatch_t>::type*/, typename in_t>
tick_t libmodule::userio::ButtonTimer<Stopwatch_t, tick_t, in_t>::releasedTime()
{
	return pm_instates.held ? 0 : pm_ticks + pm_stopwatch.ticks;
}

template <typename Stopwatch_t, typename tick_t /*= typename Stopwatch_tick<Stopwatch_t>::type*/, typename in_t>
bool libmodule::userio::ButtonTimer<Stopwatch_t, tick_t, in_t>::pressedFor(tick_t const time)
{
	if(!pm_checked && heldTime() >= time) {
		return pm_checked = true;
	}
	return false;
}


template <typename Stopwatch_t, typename tick_t /*= typename Stopwatch_tick<Stopwatch_t>::type*/, typename in_t>
bool libmodule::userio::ButtonTimer<Stopwatch_t, tick_t, in_t>::releasedFor(tick_t const time)
{
	if(!pm_checked && releasedTime() >= time) {
		return pm_checked = true;
	}
	return false;
}

template <typename Stopwatch_t, typename tick_t /*= typename Stopwatch_tick<Stopwatch_t>::type*/, typename in_t>
void libmodule::userio::ButtonTimer<Stopwatch_t, tick_t, in_t>::update()
{
	pm_instates.update();
	//If the button has just been pressed or released, reset states
	if(pm_instates.pressed || pm_instates.released) {
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

template <typename Stopwatch_t /*= Stopwatch1k*/, typename tick_t /*= typename Chrono_tick<Stopwatch_t>::type*/, typename in_t /*= bool*/>
void libmodule::userio::ButtonTimer<Stopwatch_t, tick_t, in_t>::set_input(Input const *const input)
{
	pm_instates.input = input;
}


template <typename Stopwatch_t /*= Stopwatch1k*/, typename tick_t /*= typename Chrono_tick<Stopwatch_t>::type*/, typename in_t /*= bool*/>
typename libmodule::userio::ButtonTimer<Stopwatch_t, tick_t, in_t>::Input const * libmodule::userio::ButtonTimer<Stopwatch_t, tick_t, in_t>::get_input() const
{
	return pm_instates.input;
}

template <typename Stopwatch_t, typename tick_t /*= typename Stopwatch_tick<Stopwatch_t>::type*/, typename in_t>
libmodule::userio::ButtonTimer<Stopwatch_t, tick_t, in_t>::ButtonTimer(Input const *const input) : pm_instates(input)
{
	pm_stopwatch.start();
}
