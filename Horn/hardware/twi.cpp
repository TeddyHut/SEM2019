/*
 * twi.cpp
 *
 * Created: 8/01/2019 9:51:27 PM
 *  Author: teddy
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>

#include "twi.h"

hw::TWISlave0 * hw::TWISlave0::instance = nullptr;
hw::TWISlave0 hw::inst::twiSlave0;

ISR(TWI0_TWIS_vect) {
	hw::isr_twi_slave0();
}

void hw::isr_twi_slave0()
{
	if(TWISlave0::instance != nullptr)
		TWISlave0::instance->handle_isr();
}

bool hw::TWISlave0::communicating() const 
{
	return pm_state == State::Transaction;
}

bool hw::TWISlave0::attention() const 
{
	return pm_result != Result::Wait;
}

libmodule::twi::TWISlave::Result hw::TWISlave0::result() const
{
	return pm_result;
}

void hw::TWISlave0::reset()
{
	pm_result = Result::Wait;
}

libmodule::twi::TWISlave::TransactionInfo hw::TWISlave0::lastTransaction()
{
	if(pm_result == Result::Received || pm_result == Result::Sent)
		pm_result = Result::Wait;
	return pm_previoustransaction;
}

void hw::TWISlave0::set_callbacks(Callbacks *const callbacks)
{
	pm_callbacks = callbacks;
}

void hw::TWISlave0::set_address(uint8_t const addr)
{
	TWI0.SADDR = addr << 1;
}

void hw::TWISlave0::set_recvBuffer(uint8_t buf[], uint8_t const len)
{
	//TODO: See if something like pm_recvbuf = {buf, len, pos};
	pm_recvbuf.buf = buf;
	pm_recvbuf.len = len;
	enableCheck();
}

void hw::TWISlave0::set_sendBuffer(uint8_t const buf[], uint8_t const len)
{
	pm_sendbuf.buf = buf;
	pm_sendbuf.len = len;
	enableCheck();
}

 hw::TWISlave0::TWISlave0()
{
	instance = this;
	//Enable data interrupt, address interrupt, stop interrupt
	//Note: TWI0 is note enabled here, but in enableCheck()
	//TODO: Try turning smart mode on
	TWI0.SCTRLA = TWI_DIEN_bm | TWI_APIEN_bm | TWI_PIEN_bm;
}

void hw::TWISlave0::handle_isr()
{
	//If a bus error or collision has occurred
	if(TWI0.SSTATUS & (TWI_COLL_bm | TWI_BUSERR_bm)) {
		//Reset the transaction and wait for new start condition
		pm_state = State::Idle;
		pm_result = Result::Error;
		pm_currenttransaction.buf = nullptr;
		pm_currenttransaction.len = 0;
		//Clear error and interrupt flags
		auto flags = TWI0.SSTATUS;
		TWI0.SSTATUS = flags & (TWI_DIF_bm | TWI_APIF_bm | TWI_COLL_bm | TWI_BUSERR_bm);
		return;
	}
	 
	//---Address interrupt or stop interrupt---
	if(TWI0.SSTATUS & TWI_APIF_bm) {
		//---End of transaction---
		//If either a stop bit or a repeated start has occurred, there will already be an operation in progress
		if(pm_state == State::Transaction) {
			//Fill previous transaction struct
			pm_previoustransaction = pm_currenttransaction;
			pm_previoustransaction.len = pm_bufpos;
			//Clear current transaction struct (technically not necessary)
			pm_currenttransaction.buf = nullptr;
			pm_currenttransaction.len = 0;
			//Set result as appropriate
			pm_result = (pm_previoustransaction.dir == TransactionInfo::Type::Send ? Result::Sent : Result::Received);
			//Make callbacks
			if(pm_callbacks != nullptr) {
				if(pm_previoustransaction.dir == TransactionInfo::Type::Send)
					pm_callbacks->sent(pm_previoustransaction.buf, pm_previoustransaction.len);
				else
					pm_callbacks->received(pm_previoustransaction.buf, pm_previoustransaction.len);
			}
		}

		//If a stop bit occurred
		if(!(TWI0.SSTATUS & TWI_AP_bm)) {
			pm_state = State::Idle;
			//Maybe use COMPTRANS here
			//Clear address interrupt and return
			TWI0.SSTATUS = TWI_APIF_bm;
			return;
		}

		//Set states for a new transaction
		pm_state = State::Transaction;
		pm_bufpos = 0;

		//If the DIR bit is set, it is a master read operation (slave sending data)
		if(TWI0.SSTATUS & TWI_DIR_bm) {
			pm_currenttransaction.dir = TransactionInfo::Type::Send;
			pm_currenttransaction.buf = pm_sendbuf.buf;
		}
		else {
			pm_currenttransaction.dir = TransactionInfo::Type::Receive;
			pm_currenttransaction.buf = pm_recvbuf.buf;
		}
		//Even if buffers haven't been set, still acknowledge the address packet
		TWI0.SCTRLB = TWI_ACKACT_ACK_gc | TWI_SCMD_RESPONSE_gc;
		//If DIR is recv, ACK will be sent and the next byte will be received
		//If DIR is send, ACK will be sent and a data interrupt will occur (to supply data)
	}

	//---Data interrupt--- (could use pm_currenttransaction.buf instead of recvbuf and sendbuf in here)
	if(TWI0.SSTATUS & TWI_DIF_bm) {
		//If sending
		if(TWI0.SSTATUS & TWI_DIR_bm) {
			//If this is not the first byte sent, check whether the master sent a NACK
			if(pm_bufpos > 0 && TWI0.SSTATUS & TWI_RXACK_bm)
				//If NACK received, resend the previous byte (a stop bit should follow though)
				//May need to use a COMPTRANS command here instead
				pm_bufpos--;
			//If len has been reached, send 0
			if(pm_bufpos + 1 >= pm_sendbuf.len)
				TWI0.SDATA = 0;
			//If not, send data in buffer
			else
				TWI0.SDATA = pm_sendbuf.buf[pm_bufpos++];
			//Clear the data interrupt (since no response command is issued)
			TWI0.SSTATUS = TWI_DIF_bm;
		}
		//If receiving (note: in this implementation NACK is sent for the past the end byte)
		else {
			//If the end has been reached, respond with NACK and discard data
			if(pm_bufpos >= pm_recvbuf.len) {
				pm_result = Result::NACKSent;
				TWI0.SCTRLB = TWI_ACKACT_NACK_gc | TWI_SCMD_RESPONSE_gc;
			}
			//Otherwise, accept data and respond with ACK
			else {
				pm_recvbuf.buf[pm_bufpos++] = TWI0.SDATA;
				TWI0.SCTRLB = TWI_ACKACT_ACK_gc | TWI_SCMD_RESPONSE_gc;
			}
		}
	}
}

void hw::TWISlave0::enableCheck()
{
	//See header for description
	if(pm_sendbuf.buf != nullptr && pm_sendbuf.len > 0 && pm_recvbuf.buf != nullptr && pm_recvbuf.buf > 0)
		TWI0.SCTRLA |= TWI_ENABLE_bm;
	else
		TWI0.SCTRLA &= ~TWI_ENABLE_bm;
}

libmodule::twi::TWISlave::TransactionInfo::TransactionInfo(volatile TransactionInfo const &p0) : dir(p0.dir), buf(p0.buf), len(p0.len) {}

void libmodule::twi::TWISlave::TransactionInfo::operator=(TransactionInfo const &p0) volatile
{
	dir = p0.dir;
	buf = p0.buf;
	len = p0.len;
}
