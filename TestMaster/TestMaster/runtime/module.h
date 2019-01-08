/*
 * module.h
 *
 * Created: 10/12/2018 2:39:40 PM
 *  Author: teddy
 */ 

#pragma once

#include <string.h>
#include "../hardware/utility.h"
#include "../hardware/twi.h"
#include "rttwi.h"

//---Module Protocol---
//When writing to a module, the first byte specifies the register address
//After that, the system will write to that addresss
//When reading from a module, the first byte will be 5E, after that it reads from the position

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
			uint8_t readbuf[4];
		};
	}

	namespace module {
		//Module format: {5E, 8A, sig, id, name[8], status, settings, ...}
		namespace metadata {
			namespace com {
				constexpr size_t NameLength = 8;
				constexpr uint8_t Header[] = {0x5E, 0x8A}; //Sort of "SEMA"
				namespace offset {
					enum e {
						Header = 0,
						Signature = Header + sizeof com::Header,
						ID,
						Name,
						Status = Name + NameLength,
						Settings,
						_size,
					};
				}
				namespace sig {
					namespace status {
						enum e {
							Active = 0,
							Operational = 1,
						};
					}
					namespace settings {
						enum e {
							Power = 0,
							LED = 1,
						};
					}
				}
			}
			namespace horn {
				//TODO: Investigate inline for these
				extern twi::RegisterDesc RegMetadata[];
				extern twi::ModuleRegMeta TWIDescriptor;
				namespace sig {
					namespace settings {
						enum e {
							HornState = 2,
						};
					}
				}
			}
		}
		class Master : public utility::UpdateFn {
		public:
			uint8_t get_signature() const;
			uint8_t get_id() const;
			char const *get_name() const;
			
			bool get_active() const;
			bool get_operational() const;

			void set_led(bool const state);
			void set_power(bool const state);

			void update() override;

			Master(hw::TWIMaster &twimaster, uint8_t const twiaddr, utility::Buffer &buffer, twi::ModuleRegMeta const &regs, size_t const updateInterval);
			virtual ~Master() = default;
		//Need to access this in main, and trying to save space
		//protected:
			twi::MasterBufferManager buffermanager;
			utility::Buffer &buffer;
		};
		class Horn : public Master {
		public:
			void set_state(bool const state);
			Horn(hw::TWIMaster &twimaster, uint8_t const twiaddr, size_t const updateInterval = 1000 / 30);
		protected:
			utility::StaticBuffer<metadata::com::offset::_size> buffer;
		};
	}
}
