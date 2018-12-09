/*
 * userio.cpp
 *
 * Created: 3/12/2018 3:58:28 PM
 *  Author: teddy
 */ 

#include "userio.h"

void hw::LED::setState(bool const state)
{
	transparent_setState(state);	
	pm_mode = Mode::Solid;
}

void hw::LED::blinkPattern(Pattern const &pattern)
{
	pm_pattern = pattern;
	pm_mode = Mode::Blink;
	pm_patternstate.count = 0;
	pm_patternstate.state = false;
	pm_timer.reset();
	pm_timer.finished = true;
}

hw::Blinker::Pattern hw::LED::currentPattern() const 
{
	return pm_pattern;
}

hw::LED::Mode hw::LED::currentMode() const
{
	return pm_mode;
}

void hw::LED::update()
{
	if(pm_mode == Mode::Blink) {
		if(pm_timer.finished) {
			//Start off state
			if(pm_patternstate.state) {
				//If the LED was on, turn off
				transparent_setState(pm_pattern.inverted);
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
					transparent_setState(!pm_pattern.inverted);
					pm_timer = pm_pattern.ontime;
				}
			}
			pm_patternstate.state = !pm_patternstate.state;
			pm_timer.start();
		}
	}
}

hw::LED::LED(PORT_t &port, uint8_t const pin) : pm_hwport(port), pm_hwpin(pin) {
	pm_hwport.DIRSET = 1 << pm_hwpin;
	pm_hwport.OUTCLR = 1 << pm_hwpin;
}

void hw::LED::transparent_setState(bool const state)
{
	if(state)
		pm_hwport.OUTSET = 1 << pm_hwpin;
	else
		pm_hwport.OUTCLR = 1 << pm_hwpin;
}
