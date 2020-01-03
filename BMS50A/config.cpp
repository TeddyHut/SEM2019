//Created: 24/04/2019 2:07:07 AM

/** \file
 \brief Source file for config.h.
 \date Created 2019-04-25
 \author Teddy.Hut
 */

#include "config.h"

#include <generalhardware.h>

config::Settings config::settings;

//This is handy to keep in mind: https://stackoverflow.com/questions/2008398/is-it-possible-to-print-out-the-size-of-a-c-class-at-compile-time/2008577
//And so is this: file:///C:/Users/teddy/Documents/Resources/cppreference/reference/en/cpp/language/data_members.html
/**
 Writes \c this casted as `void *` to EEPROM offest 0x00 using libmicavr::EEPManager::write_buffer(). [Here](https://en.cppreference.com/w/cpp/language/data_members) is a handy section to read for why the save is done this way (Settings is a \em StandardLayoutType).
 \note \c sizeof(float) is 4 bytes.
 */
void config::Settings::save() const
{
	//Write this object to the start of EEPROM (const_cast monkaS... should be fine though)
	libmicavr::EEPManager::write_buffer({const_cast<Settings *>(this), sizeof *this}, 0);
}


/**
 If the byte at EEPROM address 0x00 is not equal to #write_indicator, then it is assumed that a Settings object has never been saved to EEPROM. In that case, all members are set to their default values.
 \n In a \em StandardLayoutType, we can \c reinterpret_cast \c this to the first non-static non-bitfield data member. Since #write_indicator is the first non-static data member, it will be the first byte written, and also the first byte loaded.
 \sa save()
 */
void config::Settings::load()
{
	//Check to see whether EEPROM stores settings or not
	uint8_t eep_write_indicator;
	libmodule::utility::Buffer buffer(&eep_write_indicator, sizeof eep_write_indicator);
	libmicavr::EEPManager::read_buffer(buffer, 0, sizeof eep_write_indicator);
	//If settings are present in EEPROM, read them into this
	if(eep_write_indicator == write_indicator) {
		buffer.pm_ptr = reinterpret_cast<decltype(buffer.pm_ptr)>(this);
		buffer.pm_len = sizeof *this;
		libmicavr::EEPManager::read_buffer(buffer, 0, sizeof *this);
	}
	//If not, this expression will either: Cause copy-elision (ideal), cause a trivial move assignment (less idea - this just copies). Either way the class will return to the defaults.
	else *this = Settings();
}
