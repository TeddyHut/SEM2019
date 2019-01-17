/*
 * twi.cpp
 *
 * Created: 30/11/2018 2:33:25 PM
 *  Author: teddy
 */ 

#include "twi.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <libmodule/utility.h>

void hw::isr_twi_master()
{
	if(TWIMaster0::instance != nullptr)
		TWIMaster0::instance->handleISR();
}

ISR(TWI0_TWIM_vect) {
	hw::isr_twi_master();
}

//TWIMaster0 instance
hw::TWIMaster0 hw::inst::twiMaster0;

//Derived from equation in datasheet
constexpr uint8_t calculateBaud(float const f_cpu, float const f_scl, float const t_rise) {
	return static_cast<uint8_t>(f_cpu / (2.0f * f_scl) - (f_cpu * t_rise) / 2 - 5.0f);
}

bool hw::TWIMaster0::attention() const 
{
	return pm_result != TWIMaster::Result::Wait;
}

hw::TWIMaster::Result hw::TWIMaster0::result() const 
{
	return pm_result;
}

bool hw::TWIMaster0::ready() const
{
	//If the bus state is idle then ready
	return (TWI0.MSTATUS & TWI_BUSSTATE_gm) == TWI_BUSSTATE_IDLE_gc;
}

void hw::TWIMaster0::writeBuffer(uint8_t const addr, uint8_t const buf[], uint8_t const len /*= 0*/)
{
	pm_callbackType = CallbackType::WriteBuffer_Complete;
	pm_operation = Operation::WriteBuffer;
	//TODO: Work out some way to use a list initializer on this guy (complains about a volatile error)
	//pm_writebuf = {buf, len, 0};
	pm_writebuf.buf = buf;
	pm_writebuf.len = len;
	pm_writebuf.pos = 0;
	startTransaction(addr, Direction::Write);
}

void hw::TWIMaster0::readBuffer(uint8_t const addr, uint8_t buf[], uint8_t const len /*= 0*/)
{
	pm_callbackType = CallbackType::ReadBuffer_Complete;
	pm_operation = Operation::ReadBuffer;
	pm_readbuf.buf = buf;
	pm_readbuf.len = len;
	pm_readbuf.pos = 0;
	startTransaction(addr, Direction::Read);
}

void hw::TWIMaster0::writeReadBuffer(uint8_t const addr, uint8_t const writebuf[], uint8_t const writelen, uint8_t readbuf[], uint8_t const readlen /*= 0*/)
{
	pm_callbackType = CallbackType::WriteReadBuffer_Complete;
	pm_operation = Operation::WriteReadBuffer;
	pm_writebuf.buf = writebuf;
	pm_writebuf.len = writelen;
	pm_writebuf.pos = 0;
	pm_readbuf.buf = readbuf;
	pm_readbuf.len = readlen;
	pm_readbuf.pos = 0;
	startTransaction(addr, Direction::Write);
}

void hw::TWIMaster0::writeToAddress(uint8_t const addr, uint8_t const regaddr, uint8_t const buf[], uint8_t const len /*= 0*/)
{
	pm_callbackType = CallbackType::WriteToAddress_Complete;
	pm_operation = Operation::WriteToAddress;
	pm_toAddress_regaddr = regaddr;
	pm_writebuf.buf = buf;
	pm_writebuf.len = len;
	pm_writebuf.pos = 0;
	startTransaction(addr, Direction::Write);
}

void hw::TWIMaster0::readFromAddress(uint8_t const addr, uint8_t const regaddr, uint8_t buf[], uint8_t const len /*= 0*/)
{
	pm_callbackType = CallbackType::ReadFromAddress_Complete;
	pm_operation = Operation::ReadFromAddress;
	pm_toAddress_regaddr = regaddr;
	pm_readbuf.buf = buf;
	pm_readbuf.len = len;
	pm_readbuf.pos = 0;
	startTransaction(addr, Direction::Write);
}

void hw::TWIMaster0::checkForAddress(uint8_t const addr)
{
	pm_callbackType = CallbackType::CheckForAddress_Complete;
	pm_operation = Operation::CheckForAddress;
	startTransaction(addr, Direction::Write);
}

void hw::TWIMaster0::registerCallback(Callback_t const callback)
{
	pm_callback = callback;
}

hw::TWIMaster0::TWIMaster0()
{
	instance = this;
	//Set baud rate to 100kHz
	TWI0.MBAUD = calculateBaud(F_CPU, 100000, 0);
	//Enable the TWI as master, set bus timeout to 100us, enable read and write interrupts
	TWI0.MCTRLA = TWI_ENABLE_bm | TWI_TIMEOUT_200US_gc | TWI_RIEN_bm | TWI_WIEN_bm;
}

hw::TWIMaster0 *hw::TWIMaster0::instance = nullptr;

void hw::TWIMaster0::handleISR()
{
	//---Check for errors---
	//Clear flags (auto clearing doesn't seem to work consistently, or I'm just bad)
	auto flags = TWI0.MSTATUS & (TWI_RIF_bm | TWI_WIF_bm);
	TWI0.MSTATUS = flags;
	//Arbitration lost
	if(TWI0.MSTATUS & TWI_ARBLOST_bm) {
		pm_result = Result::ArbitrationLost;
		reset_state_operation();
		//Clear the arbitration lost condition
		TWI0.MCTRLB = TWI_MCMD_NOACT_gc;
		makeCallback();
		return;
	}
	//Bus error
	if(TWI0.MSTATUS & TWI_BUSERR_bm) {
		pm_result = Result::Error;
		reset_state_operation();
		//Clear the bus error condition (for whatever reason it seems this one has to be explicitly cleared, but the datasheet says otherwise)
		TWI0.MCTRLB = TWI_MCMD_NOACT_gc;
		TWI0.MSTATUS = TWI_BUSERR_bm;
		makeCallback();
		return;
	}

	//---Write interrupt---
	if(flags & TWI_WIF_bm) {
		//If NACK received, send stop bit
		if(TWI0.MSTATUS & TWI_RXACK_bm) {
			//If this is an interrupt from an address packet, set NoResponse (Note: No matter whether RW was set, no response always sets WIF)
			pm_result = (pm_state == State::AddressPacket ? Result::NoResponse : Result::NACKReceived);
			reset_state_operation();
			//For some reason busstate does not always go back to idle with a stop command... what gives?
			TWI0.MCTRLB = TWI_MCMD_STOP_gc;
			//Reset bus state to idle (really should not need to do this...)
			/**/TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;
			makeCallback();
			return;
		}

		switch(pm_operation) {
		default:
			libmodule::hw::panic();
			break;
		case Operation::WriteBuffer:
			//If the last byte was sent, send stop bit
			if(endOfBuffer(pm_writebuf.buf, pm_writebuf.len, pm_writebuf.pos)) {
				pm_result = Result::Success;
				reset_state_operation();
				TWI0.MCTRLB = TWI_MCMD_STOP_gc;
				/**/TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;
				makeCallback();
				break;
			}
			//Send the next byte
			TWI0.MDATA = pm_writebuf.buf[pm_writebuf.pos++];
			break;
		case Operation::WriteReadBuffer:
			//If the last byte was sent, send repeated start
			if(endOfBuffer(pm_writebuf.buf, pm_writebuf.len, pm_writebuf.pos)) {
				//Set next operation to ReadBuffer
				pm_operation = Operation::ReadBuffer;
				pm_state = State::AddressPacket;
				TWI0.MADDR |= static_cast<uint8_t>(Direction::Read);
				//Return here so that pm_state stays as AddressPacket
				return;
			}
			//Send the next byte
			TWI0.MDATA = pm_writebuf.buf[pm_writebuf.pos++];
			break;
		case Operation::WriteToAddress:
			//Change operation to WriteBuffer for the rest
			pm_operation = Operation::WriteBuffer;
			//Send register address
			TWI0.MDATA = pm_toAddress_regaddr;
			break;
		case Operation::ReadFromAddress:
			//Abuse WriteReadBuffer endOfBufferCheck
			pm_writebuf.len = 1;
			pm_writebuf.pos = 1;
			//Set next operation to WriteReadBuffer (but skipping the write stage)
			pm_operation = Operation::WriteReadBuffer;
			//Send register address
			TWI0.MDATA = pm_toAddress_regaddr;
			break;
		case Operation::CheckForAddress:
			//If it gets to here the slave responded
			pm_result = Result::Success;
			reset_state_operation();
			//Send stop bit
			TWI0.MCTRLB = TWI_MCMD_STOP_gc;
			makeCallback();
			break;
		}
	}
	//---Read interrupt---
	else if(flags & TWI_RIF_bm) {
		switch(pm_operation) {
		default:
			libmodule::hw::panic();
			break;
		case Operation::ReadBuffer:
			//Note: The TWI interface will automatically receive the first byte after a successful address packet
			pm_readbuf.buf[pm_readbuf.pos] = TWI0.MDATA;
			//Determine whether to stop reading data
			bool finished;
			if(pm_readbuf.len == 0)
				finished = (pm_readbuf.buf[pm_readbuf.pos++] == 0);
			else
				finished = ++pm_readbuf.pos >= pm_readbuf.len;
			
			//If finished, send NACK and stop (master should finish transaction with NACK)
			if(finished) {
				pm_result = Result::Success;
				reset_state_operation();
				//Send NACK/stop
				TWI0.MCTRLB = TWI_ACKACT_NACK_gc | TWI_MCMD_STOP_gc;
				///**/TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;
				makeCallback();
				break;
			}
			//Send ACK and receive next byte
			TWI0.MCTRLB = TWI_MCMD_RECVTRANS_gc;
			break;
		}
	}
	//Address packet sent, either way
	pm_state = State::None;
}

void hw::TWIMaster0::startTransaction(uint8_t const addr, Direction const direction)
{
	pm_result = Result::Wait;
	pm_state = State::AddressPacket;
	TWI0.MADDR = addr << 1 | static_cast<uint8_t>(direction);
}

void hw::TWIMaster0::reset_state_operation()
{
	pm_operation = Operation::None;
	pm_state = State::None;
}

void hw::TWIMaster0::makeCallback() const
{
	if(pm_callback != nullptr)
		pm_callback(pm_callbackType);
}
