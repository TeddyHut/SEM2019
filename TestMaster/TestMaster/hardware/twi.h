/*
 * twi.h
 *
 * Created: 30/11/2018 2:33:39 PM
 *  Author: teddy
 */ 

#pragma once

namespace hw {
	class TWIMaster {
	public:
		enum class Result : uint8_t {
			//Operation still in progress
			Wait,
			//Operation was a success
			Success,
			//NACK was received
			NACKReceived,
			//There was no response from the device
			NoResponse,
			//Some other error
			Error,
		};
		//Get the current status of the TWIMaster
		virtual Result result() const = 0;

		//Writes to slave until NACK or (len > 0 ? len : '\0')
		virtual void writeBuffer(uint8_t const addr, uint8_t const buf[], uint8_t const len = 0) = 0;
		//Reads from slave until NACK or len (when len > 0)
		virtual void readBuffer(uint8_t const addr, uint8_t buf[], uint8_t const len = 0) = 0;

		//Write to slave writebuf, and then with a repeated start read from slave into readbuf (until NACK or len)
		virtual void writeReadBuffer(uint8_t const addr, uint8_t const writebuf[], uint8_t const writelen, uint8_t const readbuf[], uint8_t const readlen = 0) = 0;

		//Write to slave the register address, and then write data
		virtual void writeToAddress(uint8_t const addr, uint8_t const regaddr, uint8_t const buf[], uint8_t const len = 0) = 0;
		//Write to slave the register address, and then read data
		virtual void readFromAddress(uint8_t const addr, uint8_t const regaddr, uint8_t buf[], uint8_t const len = 0) = 0;

		//Check whether a slave with with the respective address is on the bus
		virtual void checkForAddress(uint8_t const addr) = 0;

		enum class CalllbackType : uint8_t {
			WriteBuffer_Complete,
			ReadBuffer_Complete,
			WriteReadBuffer_Complete,
			WriteToAddress_Complete,
			ReadFromAddress_Complete,
			CheckForAddress_Complete,
		};
		using Callback_t = void (*)(CalllbackType const);
		//Register callback for when an operation completes
		virtual void registerCallback(Callback_t const callback) = 0;
	};

	class TWIMaster0 : public TWIMaster {
	public:
		enum class Operation : uint8_t {
			
		};

	};

	bool operator bool(TWIMaster::Result const result);
}
