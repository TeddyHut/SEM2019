/*
 * rttwi.h
 *
 * Created: 5/01/2019 3:42:49 PM
 *  Author: teddy
 */ 

#pragma once

#include <stdint.h>

#include <libmodule/utility.h>
#include <libmodule/timer.h>
#include "../hardware/twi.h"

namespace rt {
	namespace twi {
		//Could make this callback based instead of update based
		//Scans the TWI bus until a device responds
		class TWIScanner {
		public:
			//Scans the bus from startaddress to endaddress, until a device responds. If oneshot is false, it will loop back and scan again.
			void scan(uint8_t const startaddress = 1, uint8_t const endaddress = 127, bool const oneshot = true, uint8_t const startfrom = 0xff);
			//Returns based on state
			// - Found: 0x80 | address
			// - Not found, but scanning: 0x00
			// - Not found, and not scanning: 0x7f
			uint8_t found() const;
			virtual void update();

			TWIScanner(hw::TWIMaster &twimaster);
		protected:
			virtual void addressCheck(uint8_t const addr);

			hw::TWIMaster &twimaster;
			//An attempt to save some program space
			uint8_t pm_startaddress;// : 7;
			uint8_t pm_endaddress;// : 7;
			uint8_t pm_currentaddress;// : 7;
			bool pm_oneshot : 1;
			bool pm_found : 1;
			bool pm_scanning : 1;
		};

		//Each register has the following properties
		// - Direction: Read or write
		// - RegularUpdate:
		//    - If read, will read the data every cycle, otherwise only reads once.
		//    - If write, writes the data every cycle, otherwise writes when data
		// - NextUpdate: Whether it should be updated on the next cycle

		struct RegisterDesc {
			//A pos of 0xff will cause the position to be automatically determined based on the previous position
			//Note: This position determination is only done on construction of MasterBufferManager (or when "processPositions" is called in ModuleRegData)
			bool write : 1;
			bool regularUpdate : 1;
			bool nextUpdate : 1;
			uint8_t len : 5;
			uint8_t pos = 0xff;
		};

		//This should probably be some sort of generic array class
		struct ModuleRegMeta {
			void processPositions() const;
			void allNextUpdate() const;

			RegisterDesc *regs = nullptr;
			uint8_t count = 0;
		};

		//Manages a Buffer to be used on the TWI bus, assumes a register system is used
		class MasterBufferManager : public libmodule::utility::Buffer::Callbacks {
		public:
			void update();

			void run();
			void stop();
			//Default to 30Hz
			MasterBufferManager(hw::TWIMaster &twimaster, uint8_t const twiaddr, libmodule::utility::Buffer &buffer, ModuleRegMeta const &regs, size_t const updateInterval = 1000 / 30, uint8_t const headerCount = 0, bool const run = false);
			
			//The TWI address of the slave
			uint8_t m_twiaddr;
			//The register description data
			ModuleRegMeta const &m_regs;
			//Interval between cycles (in ms)
			size_t m_updateInterval;
			//The number of bytes to ignore when reading before actual data (e.g. ignore module 0x5E)
			uint8_t m_headersize;
			//The number of consecutive cycles where at least 1 error has occurred
			uint8_t m_consecutiveCycleErrors : 7;
		private:
			bool pm_cycleError : 1;
			//Warnings were given when these were bitfields (even though they were big enough)
			enum class State {
				Off,
				Waiting,
				ProcessingCycle,
			} pm_state = State::Off;
			enum class TransactionType {
				None,
				Write,
				Read,
			} pm_currentTransaction = TransactionType::None;

			uint8_t pm_regIndex = 0;
			hw::TWIMaster &twimaster;
			libmodule::utility::Buffer &buffer;
			libmodule::Timer1k pm_timer;
			struct {
				uint8_t *buf = nullptr;
				uint8_t len;
				//Location in client buffer of first byte
				uint8_t bufferoffset;
			} pm_readbuf;

			uint8_t findRegIndexFromBufferPos(size_t const pos) const;
			//Finds adjacent regs that will need the same operation (e.g. finds regs that are sequential all waiting for write)
			//Note: Returns 'past the end' position [x, y)
			uint8_t findRegIndexOfLastSimilar(uint8_t const regpos) const;

			void buffer_writeCallack(void const *const buf, size_t const len, size_t const pos) override;
			void buffer_readCallback(void *const buf, size_t const len, size_t const pos) override;
		};
	}
}