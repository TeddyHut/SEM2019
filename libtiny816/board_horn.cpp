/*
 * board_horn.cpp
 *
 * Created: 16/01/2019 1:12:28 AM
 *  Author: teddy
 */ 

#include "board_horn.h"

PORT_t &libtiny816::hw::PINPORT::LED_RED = PORTA;
PORT_t &libtiny816::hw::PINPORT::LED_GREEN = PORTA;
PORT_t &libtiny816::hw::PINPORT::BUTTON_TEST = PORTA;
PORT_t &libtiny816::hw::PINPORT::BUTTON_HORN = PORTB;
PORT_t &libtiny816::hw::PINPORT::BUTTON_WHEEL = PORTB;
PORT_t &libtiny816::hw::PINPORT::SW_MODE = PORTA;
PORT_t &libtiny816::hw::PINPORT::OUT_HORN = PORTB;

void libmodule::hw::panic() {
	PORTA.DIRSET = libtiny816::hw::PINMASK::LED_RED | libtiny816::hw::PINMASK::LED_GREEN;
	PORTA.OUTSET = libtiny816::hw::PINMASK::LED_RED | libtiny816::hw::PINMASK::LED_GREEN;
	asm("BREAK");
	while(true);
}

bool libtiny816::Button::get() const 
{
	return pm_hwport.IN & 1 << pm_hwpin;
}

libtiny816::Button::Button(PORT_t &port, uint8_t pin, bool const onlevel /*= false*/, bool const pullup /*= true*/) : pm_hwport(port), pm_hwpin(pin)
{
	pm_hwport.DIRCLR = 1 << pm_hwpin;
	uint8_t pinctrlmask = 0;
	//Invert input if the button is active low
	if(!onlevel)
		pinctrlmask = PORT_INVEN_bm;
	//Enable pullup if necessary
	if(pullup)
		pinctrlmask |= PORT_PULLUPEN_bm;
	*(&pm_hwport.PIN0CTRL + pm_hwpin) = pinctrlmask;
}

void libtiny816::LED::set(bool const state)
{
	if(state)
		pm_hwport.OUTSET = 1 << pm_hwpin;
	else
		pm_hwport.OUTCLR = 1 << pm_hwpin;
}

libtiny816::LED::LED(PORT_t &port, uint8_t const pin) : pm_hwport(port), pm_hwpin(pin)
{
	pm_hwport.DIRSET = 1 << pm_hwpin;
}
