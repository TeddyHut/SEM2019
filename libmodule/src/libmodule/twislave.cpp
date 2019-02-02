/*
 * twislave.cpp
 *
 * Created: 16/01/2019 11:31:36 PM
 *  Author: teddy
 */ 

#include "twislave.h"

libmodule::twi::SlaveBufferManager::SlaveBufferManager(TWISlave &twislave, utility::Buffer &buffer, uint8_t const header[] /*= nullptr*/, uint8_t const headerlen /*= 0*/)
: twislave(twislave), buffer(buffer), pm_header(header), pm_headerlen(headerlen) {
	pm_timer.start();
}

void libmodule::twi::SlaveBufferManager::update()
{
	//Have to set here because TWISlave constructor is called after SlaveBufferManager constructor
	twislave.set_callbacks(this);
	TWISlave::Result result;
	TWISlave::TransactionInfo transaction;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		result = twislave.result();
		transaction = twislave.lastTransaction();
	}

	//Any transaction will reset the timeout
	if(result == TWISlave::Result::Sent || result == TWISlave::Result::Received) {
		pm_timer = pm_timeout;
		pm_timer.start();
	}
	//If the buffer has contents
	if(buffer.pm_ptr != nullptr && buffer.pm_len > 0) {
		//Make sure that the buffers are the right lengths
		//+headerlen for header
		auto newbuf = utility::memsizematch<size_t>(pm_sendbuf.buf, pm_sendbuf.len, buffer.pm_len + pm_headerlen);
		if(newbuf != pm_sendbuf.buf) {
			pm_sendbuf.buf = newbuf;
			pm_sendbuf.len = buffer.pm_len + pm_headerlen;
			//Copy in the header, if it exists (maybe move this to when !communicating())
			if(pm_header != nullptr && pm_headerlen > 0)
				memcpy(pm_sendbuf.buf, pm_header, pm_headerlen);
			twislave.set_sendBuffer(pm_sendbuf.buf, pm_sendbuf.len);
		}
		//+1 for regaddr
		newbuf = utility::memsizematch<size_t>(pm_recvbuf.buf, pm_recvbuf.len, buffer.pm_len + 1);
		if(newbuf != pm_recvbuf.buf) {
			pm_recvbuf.buf = newbuf;
			pm_recvbuf.len = buffer.pm_len + 1;
			twislave.set_recvBuffer(pm_recvbuf.buf, pm_recvbuf.len);
		}

		//---Out/Send---
		//TODO: This would be more accurate if it checked only for twislave.sending()
		if(!twislave.communicating()) {
			update_sendbuf();
		}
		//---In/Receive---
		//Changed to callbacks because there wasn't time to process before the master requested info,
		//therefore incorrect data was sent/received.
		//Callbacks allow clock stretching.

	}
}

void libmodule::twi::SlaveBufferManager::set_twiaddr(uint8_t const twiaddr)
{
	twislave.set_address(twiaddr);
}

bool libmodule::twi::SlaveBufferManager::connected() const
{
	return !pm_timer;
}

void libmodule::twi::SlaveBufferManager::set_timeout(size_t const timeout)
{
	pm_timeout = timeout;
}

void libmodule::twi::SlaveBufferManager::update_sendbuf()
{
	//Copy in the data, with the data at regaddr first
	memcpy(pm_sendbuf.buf + pm_headerlen, buffer.pm_ptr + pm_regaddr, buffer.pm_len - pm_regaddr);
	//Set the memory past the end of the buffer to zero
	memset(pm_sendbuf.buf + pm_sendbuf.len - pm_regaddr, 0, pm_regaddr);
	//Example
	//pm_regaddr = 2
	//pm_sendbuf.len = 6
	//buffer.pm_len = 5
	//buffer: 00, 01, 02, 03, 04
	//Stage 1: 5e, aa, aa, aa, aa, aa
	//Stage 2: 5e, 02, 03, 04, aa, aa
	//Stage 3: 5e, 02, 03, 04, 00, 00
}

void libmodule::twi::SlaveBufferManager::sent(uint8_t const buf[], uint8_t const len) {}

void libmodule::twi::SlaveBufferManager::received(uint8_t const buf[], uint8_t const len)
{
	if(len > 0) {
		//First byte should be register address
		auto regaddr = pm_recvbuf.buf[0];
		//If transaction is valid
		if(regaddr < buffer.pm_len) {
			//Copy data from the client buffer into the sendbuf with the new regaddr
			pm_regaddr = regaddr;
			update_sendbuf();
			//Copy data into client buffer
			memcpy(buffer.pm_ptr + regaddr, pm_recvbuf.buf + 1, utility::tmin<uint8_t>(buffer.pm_len - regaddr, len - 1));
		}
	}
}

libmodule::twi::TWISlave::TransactionInfo::TransactionInfo(volatile TransactionInfo const &p0) : dir(p0.dir), buf(p0.buf), len(p0.len) {}

void libmodule::twi::TWISlave::TransactionInfo::operator=(TransactionInfo const &p0) volatile
{
	dir = p0.dir;
	buf = p0.buf;
	len = p0.len;
}
