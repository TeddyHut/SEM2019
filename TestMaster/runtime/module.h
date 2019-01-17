/*
 * module.h
 *
 * Created: 10/12/2018 2:39:40 PM
 *  Author: teddy
 */ 

#pragma once

#include <string.h>
#include <libmodule/metadata.h>
#include <libmodule/utility.h>
#include "../hardware/twi.h"
#include "../runtime/rttwi.h"

//---Module Protocol---
//When writing to a module, the first byte specifies the register address
//After that, the system will write to that addresss
//When reading from a module, the first byte will be 5E, after that it reads from the position

namespace libmodule {
	namespace module {
		namespace metadata {
			namespace horn {
				extern rt::twi::RegisterDesc RegMetadata[];
				extern rt::twi::ModuleRegMeta TWIDescriptor;
			}
		}
	}
}

namespace rt {
	namespace twi {
		struct ModuleDescriptor {
			uint8_t addr;
			uint8_t signature;
			uint8_t id;
		};
		class ModuleScanner : public TWIScanner {
		public:
			//Returns based on state
			// - Found: 0x80 | address
			// - Not found, but scanning/reading: 0x00
			// - Not found, and not scanning: 0x7f
			uint8_t found() const;
			//Returns the module that was found assuming found() is true
			ModuleDescriptor foundModule() const;
			void update() override;
			ModuleScanner(hw::TWIMaster &twimaster);
		private:
			enum class State {
				Idle,
				Scanning,
				Reading,
				Found,
			} pm_state = State::Idle;
			//Overrides from TWIScanner, instead of just checking for the address will attempt to read 1 byte
			void addressCheck(uint8_t const addr) override;
			ModuleDescriptor pm_descriptor;
			uint8_t readbuf[5];
		};
	}

	namespace module {
		//Module format: {5E, 8A, sig, id, name[8], status, settings, ...}
		
		class Master {
		public:
			uint8_t get_signature() const;
			uint8_t get_id() const;
			char const *get_name() const;
			
			bool get_active() const;
			bool get_operational() const;

			void set_led(bool const state);
			void set_power(bool const state);

			void update();

			Master(hw::TWIMaster &twimaster, uint8_t const twiaddr, libmodule::utility::Buffer &buffer, twi::ModuleRegMeta const &regs, size_t const updateInterval);
			virtual ~Master() = default;
		//Need to access this in main, and trying to save space
		//protected:
			twi::MasterBufferManager buffermanager;
			libmodule::utility::Buffer &buffer;
			bool pmf_run_init = true;
		};
		class Horn : public Master {
		public:
			void set_state(bool const state);
			Horn(hw::TWIMaster &twimaster, uint8_t const twiaddr, size_t const updateInterval = 1000 / 30);
		protected:
			libmodule::utility::StaticBuffer<libmodule::module::metadata::com::offset::_size> buffer;
		};
	}
}
