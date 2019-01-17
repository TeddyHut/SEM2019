/*
 * twislave.h
 *
 * Created: 16/01/2019 11:30:57 PM
 *  Author: teddy
 */ 

#pragma once

#include "utility.h"
#include "timer.h"

namespace libmodule {
	namespace twi {
		class TWISlave {
		public:
			struct Callbacks {
				//Potentially return from here to tell the TWI slave the next action
				//Could also have just one callback that takes a TransactionInfo
				virtual void sent(uint8_t const buf[], uint8_t const len) = 0;
				virtual void received(uint8_t const buf[], uint8_t const len) = 0;
			};
			enum class Result {
				//Either no transaction or transaction in progress
				Wait,
				//NACK sent (master requested too many bytes)
				NACKSent,
				//Some sort of error (probably bus error, can be ignored)
				Error,
				//Data was sent
				Sent,
				//Data was received
				Received,
			};
			struct TransactionInfo {
				enum class Type {
					Send,
					Receive,
				} dir;
				uint8_t const *buf;
				uint8_t len;

				TransactionInfo() = default;
				TransactionInfo(TransactionInfo const &) = default;
				TransactionInfo(volatile TransactionInfo const &p0);
				TransactionInfo &operator=(TransactionInfo const &) = default;
				//TransactionInfo vovlatile &operator=(TransactionInfo const &p0) volatile;
				//Using void here avoids a warning: https://stackoverflow.com/questions/13869318/gcc-warning-about-implicit-dereference
				void operator=(TransactionInfo const &p0) volatile;
			};

			//Returns true when communicating
			virtual bool communicating() const = 0;
			//Returns true if result is not Wait
			virtual bool attention() const = 0;
			//Returns the current result
			virtual Result result() const = 0;
			//Resets result to Idle
			virtual void reset() = 0;
			//Returns information about the last transaction. Will reset Result to Wait if it is either Sent or Received
			virtual TransactionInfo lastTransaction() = 0;
			//Set the callback function object
			virtual void set_callbacks(Callbacks *const callbacks) = 0;

			//Set slave TWI address
			virtual void set_address(uint8_t const addr) = 0;

			//Set the buffer to accept received data. If len is reached, a NACK will be sent (on the byte after the last)
			virtual void set_recvBuffer(uint8_t *const buf, uint8_t const len) = 0;
			//Set the buffer to send data. If len is reached, zeros will be transmitted afterwards
			virtual void set_sendBuffer(uint8_t const *const buf, uint8_t const len) = 0;
		};
		
		//Manages a register based read/write buffer to be accessed by a master
		//There is no register metadata, which means that the master could easily overwrite read-only data in the buffer
		class SlaveBufferManager  {
		public:
			void update();

			void set_twiaddr(uint8_t const twiaddr);
			void set_timeout(size_t const timeout);
			//Returns true if timeout between transactions has not been reached
			bool connected() const;

			SlaveBufferManager(TWISlave &twislave, utility::Buffer &buffer, uint8_t const header[] = nullptr, uint8_t const headerlen = 0);
		private:
			void update_sendbuf();

			TWISlave &twislave;
			utility::Buffer &buffer;
			uint8_t const *pm_header = nullptr;
			uint8_t pm_headerlen = 0;
			struct {
				uint8_t *buf = nullptr;
				uint8_t len = 0;
			} pm_sendbuf;
			struct {
				uint8_t *buf = nullptr;
				uint8_t len = 0;
			} pm_recvbuf;
			uint8_t pm_regaddr = 0;
			Timer1k pm_timer;
			size_t pm_timeout = 1000;
		};
	}
}
