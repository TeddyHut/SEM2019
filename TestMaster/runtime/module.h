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
				extern rt::twi::RegisterDesc RegMetadata[3];
				static constexpr size_t RegCount = sizeof RegMetadata / sizeof(rt::twi::RegisterDesc);
				extern rt::twi::ModuleRegMeta TWIDescriptor;
			}
			//These do not have TWI descriptors because the member of SpeedMonitorManager is used
			namespace speedmonitormanager {
				extern rt::twi::RegisterDesc RegMetadata[4];
				static constexpr size_t RegCount = sizeof RegMetadata / sizeof(rt::twi::RegisterDesc);
			}
			namespace speedmonitor {
				extern rt::twi::RegisterDesc RegMetadata[3];
				static constexpr size_t RegCount = sizeof RegMetadata / sizeof(rt::twi::RegisterDesc);
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

		template <typename sample_t>
		class SpeedMonitorManager : public Master {
		public:
			void update();

			uint8_t get_sample_pos(uint8_t const mtr) const;
			sample_t get_sample(uint8_t const mtr, uint8_t const sample) const;
			libmodule::module::metadata::speedmonitor::rps_t get_rps_constant(uint8_t const mtr) const;

			SpeedMonitorManager(hw::TWIMaster &twimaster, uint8_t const twiaddr, size_t const updateInterval = 1000 / 30);
			~SpeedMonitorManager();
			uint8_t m_instancecount = 0;
			uint8_t m_samplecount = 0;
			bool m_ready = false;
		private:
			uint8_t get_bufferoffset_monitor(uint8_t const mtr) const;

			libmodule::utility::Buffer buffer;
			rt::twi::ModuleRegMeta pm_regdescriptor;
			uint8_t *pm_oldbuffer = nullptr;
		};

		class MotorMover : public Master {
		public:
			void set_engaged(bool const state);
			MotorMover(hw::TWIMaster &twimaster, uint8_t const twiaddr, size_t const updateInterval = 1000 / 30);
		private:
			libmodule::utility::StaticBuffer<libmodule::module::metadata::motormover::offset::_size> buffer;
		};
}
}

template <typename sample_t>
void rt::module::SpeedMonitorManager<sample_t>::update()
{
	using namespace libmodule::module;
	uint8_t samplesize = (buffer.serialiseRead<uint8_t>(metadata::com::offset::Status) & metadata::speedmonitor::mask::status::SampleSize) >> metadata::speedmonitor::sig::status::SampleSize;
	//Cannot work with SpeedMonitors of different sample size
	if(samplesize != sizeof(sample_t)) return;

	uint8_t samplecount = buffer.serialiseRead<uint8_t>(metadata::speedmonitor::offset::manager::SampleCount);
	uint8_t instancecount = buffer.serialiseRead<uint8_t>(metadata::speedmonitor::offset::manager::InstanceCount);
	//This is used to make sure that the buffer has properly updated (from reading information) before making changes
	if(samplecount > 0 && instancecount > 0 && m_samplecount != samplecount) {
		m_instancecount = instancecount;
		m_samplecount = samplecount;
		uint8_t const total_len = get_bufferoffset_monitor(instancecount);
		//Keep pointer of old buffer since it will need to be freed after the BufferManager transfers to the new one
		pm_oldbuffer = buffer.pm_ptr;
		//Give buffermanager new buffer to transfer to
		buffermanager.m_new_buffer.pm_ptr = static_cast<uint8_t *>(malloc(total_len));
		if(buffermanager.m_new_buffer.pm_ptr == nullptr) libmodule::hw::panic();
		buffermanager.m_new_buffer.pm_len = total_len;
		//Zero the new buffer
		memset(buffermanager.m_new_buffer.pm_ptr, 0, total_len);
		//Copy Manager contents of old buffer into new buffer
		memcpy(buffermanager.m_new_buffer.pm_ptr, pm_oldbuffer, metadata::speedmonitor::offset::manager::_size);
	}
	if(buffermanager.m_new_buffer.pm_ptr == nullptr && buffermanager.m_new_buffer.pm_len > 0) {
		//Buffer will be updated with new information since twimaster takes a reference
		free(pm_oldbuffer);
		pm_oldbuffer = nullptr;
		//Indicate that the transfer has completed
		buffermanager.m_new_buffer.pm_len = 0;
		//Fill out new register information (buffermanager uses a reference, so updating local one is fine)
		pm_regdescriptor.count = metadata::speedmonitormanager::RegCount + m_instancecount * metadata::speedmonitor::RegCount;
		pm_regdescriptor.regs = static_cast<rt::twi::RegisterDesc *>(realloc(pm_regdescriptor.regs, pm_regdescriptor.count * sizeof(rt::twi::RegisterDesc)));
		//Copy in instance metadata
		for(uint8_t i = 0; i < m_instancecount; i++) {
			uint8_t instance_reg_zero = metadata::speedmonitormanager::RegCount + i * metadata::speedmonitor::RegCount;
			memcpy(&(pm_regdescriptor.regs[instance_reg_zero]), metadata::speedmonitor::RegMetadata, sizeof metadata::speedmonitor::RegMetadata);
			//Set the buffer size in the buffer register. Buffer should be the last reg (therefore RegCount - 1)
			pm_regdescriptor.regs[instance_reg_zero + metadata::speedmonitor::RegCount - 1].len = sizeof(sample_t) * m_samplecount;
		}
		//Update position information for new regs
		pm_regdescriptor.processPositions();
		m_ready = true;
	}
}

template <typename sample_t>
rt::module::SpeedMonitorManager<sample_t>::SpeedMonitorManager(hw::TWIMaster &twimaster, uint8_t const twiaddr, size_t const updateInterval /*= 1000 / 30*/)
 : Master(twimaster, twiaddr, buffer, pm_regdescriptor, updateInterval)
{
	using namespace libmodule::module;
	//Allocate memory and copy in the register metadata for the manager
	pm_regdescriptor.regs = static_cast<rt::twi::RegisterDesc *>(malloc(sizeof metadata::speedmonitormanager::RegMetadata));
	memcpy(pm_regdescriptor.regs, metadata::speedmonitormanager::RegMetadata, sizeof metadata::speedmonitormanager::RegMetadata);
	pm_regdescriptor.count = metadata::speedmonitormanager::RegCount;
	//Need to call these here since there was no memory allocated when these were called in MasterBufferManager
	pm_regdescriptor.processPositions();
	pm_regdescriptor.allNextUpdate();
	//Allocate memory for buffer
	buffer.pm_ptr = static_cast<uint8_t *>(malloc(metadata::speedmonitor::offset::manager::_size));
	buffer.pm_len = metadata::speedmonitor::offset::manager::_size;
	memset(buffer.pm_ptr, 0, metadata::speedmonitor::offset::manager::_size);
	//Run the buffermanager
	buffermanager.run();
}

template <typename sample_t>
rt::module::SpeedMonitorManager<sample_t>::~SpeedMonitorManager()
{
	using namespace libmodule::module;
	if(pm_oldbuffer != nullptr && pm_oldbuffer != buffer.pm_ptr)
		free(pm_oldbuffer);
	free(buffer.pm_ptr);
	free(pm_regdescriptor.regs);
}

template <typename sample_t>
uint8_t rt::module::SpeedMonitorManager<sample_t>::get_bufferoffset_monitor(uint8_t const mtr) const
{
	using namespace libmodule::module;
	uint8_t const monitor_len = metadata::speedmonitor::offset::instance::SampleBuffer + sizeof(sample_t) * m_samplecount;
	return metadata::speedmonitor::offset::manager::_size + mtr * monitor_len;
}

template <typename sample_t>
uint8_t rt::module::SpeedMonitorManager<sample_t>::get_sample_pos(uint8_t const mtr) const
{
	using namespace libmodule::module;
	if(mtr >= m_instancecount)
		return 0;
	return buffer.serialiseRead<uint8_t>(get_bufferoffset_monitor(mtr) + metadata::speedmonitor::offset::instance::SamplePos);
}

template <typename sample_t>
sample_t rt::module::SpeedMonitorManager<sample_t>::get_sample(uint8_t const mtr, uint8_t const sample) const
{
	using namespace libmodule::module;
	if(mtr >= m_instancecount || sample >= m_samplecount)
		return 0;
	return buffer.serialiseRead<sample_t>(get_bufferoffset_monitor(mtr) + metadata::speedmonitor::offset::instance::SampleBuffer + sample * sizeof(sample_t));
}

template <typename sample_t>
libmodule::module::metadata::speedmonitor::rps_t rt::module::SpeedMonitorManager<sample_t>::get_rps_constant(uint8_t const mtr) const
{
	using namespace libmodule::module;
	if(mtr >= m_instancecount)
		return 0;
	return buffer.serialiseRead<metadata::speedmonitor::rps_t>(get_bufferoffset_monitor(mtr) + metadata::speedmonitor::offset::instance::Constant_RPS);
}
