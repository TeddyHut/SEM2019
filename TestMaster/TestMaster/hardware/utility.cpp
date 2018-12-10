/*
 * utility.cpp
 *
 * Created: 6/12/2018 1:12:28 AM
 *  Author: teddy
 */ 

#include <avr/io.h>
#include "utility.h"

void operator delete(void * ptr, unsigned int len)
{
	//May need to free memory here
}

void __cxa_pure_virtual()
{
	hw::panic();
}

PORT_t &hw::PINPORT::LED_RED = PORTA;
PORT_t &hw::PINPORT::LED_GREEN = PORTA;
PORT_t &hw::PINPORT::BUTTON_TEST = PORTA;
PORT_t &hw::PINPORT::BUTTON_HORN = PORTB;
