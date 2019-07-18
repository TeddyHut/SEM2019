/*
 * extrahardware.cpp
 *
 * Created: 16/07/2019 2:57:27 PM
 *  Author: teddy
 */ 

#include "extrahardware.h"
#include <util/delay.h>

/* SegDisplay Functional Description
 * 1. Push display data
 * 2. TX interrupt occurs
 * 3. Latch shift reg
 * 4. Disable TX interrupt
 * 5. Push ff (turn display off)
 * 6. Timer interrupt occurs
 * 7. Latch reg
 * 8. Switch displays
 * 9. Enable TX interrupt
 * 10. Repeat
 */

/* - Push display data
 * - TX interrupt occurs
 * - Toggle RCLK
 * - If interrupt occurred after pushing off data, exit
 * - Otherwise, push off data
 * - */

constexpr uint16_t calculate_tcb2_ccmp(float const freq) {
	return (F_CPU / 2) / freq;
}

extrahardware::SegDisplay * extrahardware::SegDisplay::currentinstance = nullptr;

ISR(USART1_TXC_vect) {
	extrahardware::SegDisplay::currentinstance->handle_isr_tx();
}

ISR(TCB2_INT_vect) {
	extrahardware::SegDisplay::currentinstance->handle_isr_tcb();
}

void extrahardware::SegDisplay::handle_isr_tx()
{
	//Clear interrupt flag (should do this automatically?)
	USART1.STATUS = USART_TXCIF_bm;
	//If off was pushed, turn off RCLK and return
	if(pushed_off) {
		//RCLK off (latched for 'digit')
		PORTC.OUTCLR = 1 << 1;
		return;
	}
	//RCLK on (latched for 'digit') 
	PORTC.OUTSET = 1 << 1;
	//Push 'off'
	pushed_off = true;
	USART1.TXDATAL = 0xff;

}

void extrahardware::SegDisplay::handle_isr_tcb()
{
	//Clear interrupt flag
	TCB2.INTFLAGS = TCB_CAPT_bm;

	//If off was pushed, latch off and push data for next digit
	if(pushed_off) {
		//RCLK on (latched for 'off')
		PORTC.OUTSET = 1 << 1;
		//Push 'digit'
		pushed_off = false;
		USART1.TXDATAL = digitdata[nextdigit];
		//Toggle next digit
		nextdigit ^= 1;

		//Wait 4 bits before turning off RCLK and switching digits
		TCB2.CCMP = calculate_tcb2_ccmp(config_baudrate / 4);
		return;
	}
	//If digit data is currently being pushed, turn off RCLK and toggle digit pin to get ready for latching next digit
	//Toggle digit select pin, RCLK off (latched for 'off')
	PORTC.OUTTGL = (1 << 3) | (1 << 1);
	//Go back to digit switch frequency, and continue from where counter left off
	TCB2.CCMP = calculate_tcb2_ccmp(config_frequency_digitswitch);
	TCB2.CNT = calculate_tcb2_ccmp(config_baudrate / 4);// + TCB2.CNT;
}

extrahardware::SegDisplay::SegDisplay()
{
	currentinstance = this;
	//Enable invert for SER, RCLK, SRCLK and select
	PORTC.DIRSET = 0x0f;
	PORTC.PIN0CTRL = PORT_INVEN_bm;
	PORTC.PIN1CTRL = PORT_INVEN_bm;
	PORTC.PIN2CTRL = PORT_INVEN_bm;
	PORTC.PIN3CTRL = PORT_INVEN_bm;
	//Select first digit
	PORTC.OUTSET = 1 << 3;

	//---Use USART in master SPI mode for pushing data
	//Enable transmit complete interrupt
	USART1.CTRLA = USART_TXCIE_bm;
	//Enable transmitter
	USART1.CTRLB = USART_TXEN_bm;
	//Master SPI mode, MSB first, leading edge
	USART1.CTRLC = USART_CMODE_MSPI_gc;
	//100kHz buad rate
	USART1.BAUD = (F_CPU / (2 * config_baudrate)) << 6;

	//---Enable TCB2 as character switch interrupt
	//Set to 120Hz (overall refresh rate of 60Hz)
	TCB2.CCMP = calculate_tcb2_ccmp(config_frequency_digitswitch);
	//Default to periodic interrupt mode. Enable CAPT interrupt.
	TCB2.INTCTRL = TCB_CAPT_bm;
	//Enable run in standby, and enable (clock / 2)
	TCB2.CTRLA = TCB_RUNSTDBY_bm | TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;
}
