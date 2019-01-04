/*
 * twi.h
 *
 * Created: 30/11/2018 2:33:39 PM
 *  Author: teddy
 */ 

#pragma once

#include <avr/io.h>
#include "utility.h"

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
			//Arbitration was lost
			ArbitrationLost,
			//Some other error
			Error,
		};
		//Check whether the TWIMaster is not in the "Wait" result state (since user-defined conversions won't work for enums)
		virtual bool attention() const = 0;
		//Get the current status of the TWIMaster
		virtual Result result() const = 0;
		//Check whether the TWIMaster is ready for the next operation
		virtual bool ready() const = 0;

		//Writes to slave until NACK or (len > 0 ? len : '\0' inclusive)
		virtual void writeBuffer(uint8_t const addr, uint8_t const buf[], uint8_t const len = 0) = 0;
		//Reads from slave until '\0' or len (when len > 0)
		virtual void readBuffer(uint8_t const addr, uint8_t buf[], uint8_t const len = 0) = 0;

		//Write to slave writebuf, and then with a repeated start read from slave into readbuf (until NACK or len)
		virtual void writeReadBuffer(uint8_t const addr, uint8_t const writebuf[], uint8_t const writelen, uint8_t readbuf[], uint8_t const readlen = 0) = 0;

		//Write to slave the register address, and then write data
		virtual void writeToAddress(uint8_t const addr, uint8_t const regaddr, uint8_t const buf[], uint8_t const len = 0) = 0;
		//Write to slave the register address, and then read data
		virtual void readFromAddress(uint8_t const addr, uint8_t const regaddr, uint8_t buf[], uint8_t const len = 0) = 0;

		//Check whether a slave with with the respective address is on the bus
		virtual void checkForAddress(uint8_t const addr) = 0;

		enum class CallbackType : uint8_t {
			WriteBuffer_Complete,
			ReadBuffer_Complete,
			WriteReadBuffer_Complete,
			WriteToAddress_Complete,
			ReadFromAddress_Complete,
			CheckForAddress_Complete,
		};
		using Callback_t = void (*)(CallbackType const);
		//Register callback for when an operation completes
		virtual void registerCallback(Callback_t const callback) = 0;
	};

	void isr_twi_master();

	class TWIMaster0 : public TWIMaster {
		friend void isr_twi_master();
	public:
		bool attention() const override;
		Result result() const override;
		inline bool ready() const override;

		void writeBuffer(uint8_t const addr, uint8_t const buf[], uint8_t const len = 0) override;
		void readBuffer(uint8_t const addr, uint8_t buf[], uint8_t const len = 0) override;

		void writeReadBuffer(uint8_t const addr, uint8_t const writebuf[], uint8_t const writelen, uint8_t readbuf[], uint8_t const readlen = 0) override;

		void writeToAddress(uint8_t const addr, uint8_t const regaddr, uint8_t const buf[], uint8_t const len = 0) override;
		void readFromAddress(uint8_t const addr, uint8_t const regaddr, uint8_t buf[], uint8_t const len = 0) override;

		void checkForAddress(uint8_t const addr) override;

		void registerCallback(Callback_t const callback) override;

		TWIMaster0();
	private:
		//Check whether the end of a write/read should occur
		template <typename value_t, typename len_t>
		static constexpr bool endOfBuffer(value_t const val, len_t const len, len_t const pos);
		//Interrupt management
		static TWIMaster0 *instance;
		void handleISR();

		//Enums
		enum class Operation : uint8_t {
			None,
			WriteBuffer,
			ReadBuffer,
			WriteReadBuffer,
			WriteToAddress,
			ReadFromAddress,
			CheckForAddress,
		} volatile pm_operation = Operation::None;

		//This is only really used to determine whether it was an address packet sent last
		enum class State : uint8_t {
			None,
			AddressPacket,
		} volatile pm_state = State::None;

		enum class Direction : uint8_t {
			Write = 0,
			Read = 1,
		};

		//Starts a transaction (writes to MADDR)
		inline void startTransaction(uint8_t const addr, Direction const direction);
		//Resets state and opertion
		inline void reset_state_operation();
		//Makes user callback if not nullptr
		void makeCallback() const;

		
		volatile Result pm_result;
		CallbackType pm_callbackType;

		struct {
			uint8_t *buf = nullptr;
			uint8_t len = 0;
			uint8_t pos = 0;
		} volatile pm_readbuf;
		struct {
			uint8_t const *buf = nullptr;
			uint8_t len = 0;
			uint8_t pos = 0;
		} volatile pm_writebuf;
		uint8_t pm_toAddress_regaddr = 0;

		Callback_t pm_callback = nullptr;
	};

	//Could make this callback based instead of update based
	//Scans the TWI bus until a device responds
	class TWIScanner : public utility::UpdateFn {
	public:
		//Scans the bus from startaddress to endaddress, until a device responds. If oneshot is false, it will loop back and scan again.
		void scan(uint8_t const startaddress = 1, uint8_t const endaddress = 127, bool const oneshot = true);
		//Returns based on state
		// - Found: 0x80 | address
		// - Not found, but scanning: 0x00
		// - Not found, and not scanning: 0x7f
		uint8_t found() const;
		void update() override;

		TWIScanner(TWIMaster &twimaster);
	protected:
		virtual void addressCheck(uint8_t const addr);
		TWIMaster &twimaster;
		uint8_t pm_startaddress : 7;
		bool pm_found : 1;
		uint8_t pm_endaddress : 7;
		bool pm_oneshot : 1;
		uint8_t pm_currentaddress : 7;
		bool pm_scanning : 1;
	};

	namespace inst {
		extern TWIMaster0 twiMaster0;
	}
}

template <typename array_t, typename len_t>
constexpr bool hw::TWIMaster0::endOfBuffer(array_t const array, len_t const len, len_t const pos)
{
	if(len > 0)
		return pos >= len;
	if(pos == 0)
		return false;
	return array[pos - 1] == 0;
}
