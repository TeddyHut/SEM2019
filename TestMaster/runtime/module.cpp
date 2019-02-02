/*
 * module.cpp
 *
 * Created: 10/12/2018 2:40:06 PM
 *  Author: teddy
 */ 

#include <string.h>

#include "module.h"

using namespace libmodule::module;

//Common registers
rt::twi::RegisterDesc libmodule::module::metadata::horn::RegMetadata[] = {
	//write, regular, next, len, [pos]
	{false, false, false, 2 + 1 + 1 + metadata::com::NameLength}, //Header + Signature + ID + Name
	{false, true, false, 1}, //Status
	{true, false, false, 1}  //Settings
};

//Common + SpeedMonitorManager registers
rt::twi::RegisterDesc libmodule::module::metadata::speedmonitormanager::RegMetadata[] = {
	//write, regular, next, len, [pos]
	{false, false, false, 2 + 1 + 1 + metadata::com::NameLength}, //Header + Signature + ID + Name
	{false, true, false, 1}, //Status
	{true, false, false, 1}, //Settings
	{false, false, false, 1 + 1} //InstanceCount + SampleCount
};

//SpeedMonitor registers
rt::twi::RegisterDesc libmodule::module::metadata::speedmonitor::RegMetadata[] = {
	//write, regular, next, len, [pos]
	{false, false, true, sizeof(rps_t) + sizeof(cps_t)}, //RPS + TPS (next is true so that these are read when the number of instances are determined)
	{false, true, false, 1}, //Sample pos
	{false, true, false, 0} //Buffer
};

rt::twi::ModuleRegMeta libmodule::module::metadata::horn::TWIDescriptor{RegMetadata, RegCount};

uint8_t rt::twi::ModuleScanner::found() const
{
	//If found, return address and found flag
	if(pm_state == State::Found)
		return 0x80 | pm_currentaddress;
	//If not found, but still scanning return 0x00
	if(pm_state == State::Scanning || pm_state == State::Reading)
		return 0x00;
	//If idle returns 0x7f
	return 0x7f;
}

rt::twi::ModuleDescriptor rt::twi::ModuleScanner::foundModule() const
{
	return pm_descriptor;
}

void rt::twi::ModuleScanner::update()
{
	using namespace libmodule::module;
	switch(pm_state) {
	case State::Idle:
	case State::Found:
		//This really should be set in the member function "scan", but trying to save space by not duplicating the inherited one
		if(!pm_scanning)
			break;
		pm_state = State::Scanning;
		//[[fallthrough]];
	case State::Scanning:
		TWIScanner::update();
		//If a device was found on the TWI bus, and reads 0x5E (for module), then read from register address 0 header, signature and id
		if(pm_found && readbuf[0] == metadata::com::Header[0]) {
			twimaster.readFromAddress(pm_currentaddress, 0x00, readbuf, sizeof readbuf);
			pm_state = State::Reading;
		}
		break;
	case State::Reading:
		if(twimaster.attention()) {
			//Check that it was a successful read, and the header is correct
			//memcmp has +1 because 0x5E will be read twice (one for static header byte, one for reading from addr 0x00)
			if(twimaster.result() == hw::TWIMaster::Result::Success &&
			   memcmp(readbuf + 1, metadata::com::Header, sizeof metadata::com::Header) == 0) {
				//Read signature and id
				pm_descriptor.addr = pm_currentaddress;
				pm_descriptor.signature = readbuf[metadata::com::offset::Signature + 1];
				pm_descriptor.id = readbuf[metadata::com::offset::ID + 1];
				pm_state = State::Found;
			}
			//If not, resume scanning
			else {
				scan(pm_startaddress, pm_endaddress, pm_oneshot, pm_currentaddress + 1);
				pm_state = State::Scanning;
			}
		}
		break;
	}
}

rt::twi::ModuleScanner::ModuleScanner(hw::TWIMaster &twimaster) : TWIScanner(twimaster) {}

void rt::twi::ModuleScanner::addressCheck(uint8_t const addr)
{
	readbuf[0] = 0;
	//Read to check for 5E (will be the first byte sent by a module every time)
	twimaster.readBuffer(addr, readbuf, 1);
}

uint8_t rt::module::Master::get_signature() const
{
	return buffer.serialiseRead<uint8_t>(metadata::com::offset::Signature);
}

uint8_t rt::module::Master::get_id() const
{
	return buffer.serialiseRead<uint8_t>(metadata::com::offset::ID);
}

char const * rt::module::Master::get_name() const
{
	return reinterpret_cast<char const *>(buffer.pm_ptr + metadata::com::offset::Name);
}

bool rt::module::Master::get_active() const
{
	return buffer.bit_get(metadata::com::offset::Status, metadata::com::sig::status::Active);
}

bool rt::module::Master::get_operational() const
{
	return buffer.bit_get(metadata::com::offset::Status, metadata::com::sig::status::Operational);
}

void rt::module::Master::set_led(bool const state)
{
	buffer.bit_set(metadata::com::offset::Settings, metadata::com::sig::settings::LED, state);
}

void rt::module::Master::set_power(bool const state)
{
	buffer.bit_set(metadata::com::offset::Settings, metadata::com::sig::settings::Power, state);
}

void rt::module::Master::update()
{
	buffermanager.update();
}

rt::module::Master::Master(hw::TWIMaster &twimaster, uint8_t const twiaddr, libmodule::utility::Buffer &buffer, twi::ModuleRegMeta const &regs, size_t const updateInterval)
 : buffer(buffer), buffermanager(twimaster, twiaddr, buffer, regs, updateInterval, 1) {
 }

void rt::module::Horn::set_state(bool const state)
{
	buffer.bit_set(metadata::com::offset::Settings, metadata::horn::sig::settings::HornState, state);
}

rt::module::Horn::Horn(hw::TWIMaster &twimaster, uint8_t const twiaddr, size_t const updateInterval /*= 1000 / 30*/)
 : Master(twimaster, twiaddr, buffer, metadata::horn::TWIDescriptor, updateInterval)
{
	//Clear the buffer
	memset(buffer.pm_ptr, 0, buffer.pm_len);
	//Run the buffermanager
	buffermanager.run();
}
