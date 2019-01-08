/*
 * utility.cpp
 *
 * Created: 6/12/2018 1:12:28 AM
 *  Author: teddy
 */ 

#include <avr/io.h>
#include "utility.h"

void *operator new(unsigned int len) {
	return malloc(len);
}

void operator delete(void * ptr, unsigned int len)
{
	free(ptr);
}

void __cxa_pure_virtual()
{
	hw::panic();
}

PORT_t &hw::PINPORT::LED_RED = PORTA;
PORT_t &hw::PINPORT::LED_GREEN = PORTA;
PORT_t &hw::PINPORT::BUTTON_TEST = PORTA;
PORT_t &hw::PINPORT::BUTTON_HORN = PORTB;

void utility::Buffer::bitSet(size_t const pos, uint8_t const sig, bool const state /*= true*/)
{
	uint8_t val;
	if(state)
		val = pm_ptr[pos] | 1 << sig;
	else
		val = pm_ptr[pos] & ~(1 << sig);
	//If there was a change, then write it (this will give callback)
	if(val != pm_ptr[pos])
		write(static_cast<void const *>(&val), sizeof(uint8_t), pos);
}

void utility::Buffer::bitSetMask(size_t const pos, uint8_t const mask)
{
	uint8_t val = pm_ptr[pos] | mask;
	if(val != pm_ptr[pos])
		write(static_cast<void const *>(&val), sizeof(uint8_t), pos);
}

void utility::Buffer::bitClear(size_t const pos, uint8_t const sig)
{
	bitSet(pos, sig, false);
}

void utility::Buffer::bitClearMask(size_t const pos, uint8_t const mask)
{
	uint8_t val = pm_ptr[pos] & ~mask;
	if(val != pm_ptr[pos])
		write(static_cast<void const *>(&val), sizeof(uint8_t), pos);
}

bool utility::Buffer::bitGet(size_t const pos, uint8_t const sig) const
{
	uint8_t val;
	read(static_cast<void *>(&val), sizeof(uint8_t), pos);
	return val & 1 << sig;
}

void utility::Buffer::write(void const *const buf, size_t const len)
{
	write(buf, len, pm_pos);
}

void utility::Buffer::write(void const *const buf, size_t const len, size_t const pos)
{
	//Presently this will not wrap around to the start and write remaining data if the end is reached
	if(invalidTransfer(pos, len) || !memcpy(pm_ptr + pos, buf, len))
		hw::panic();
	pm_pos = pos + len;
	if(m_callbacks != nullptr)
		m_callbacks->buffer_writeCallack(buf, len, pos);
}

void utility::Buffer::read(void *const buf, size_t const len)
{
	read(buf, len, pm_pos);
}

void utility::Buffer::read(void *const buf, size_t const len, size_t const pos)
{
	pm_pos = pos + len;
	static_cast<Buffer const *>(this)->read(buf, len, pos);
}

void utility::Buffer::read(void *const buf, size_t const len, size_t const pos) const
{
	if(invalidTransfer(pos, len) || !memcpy(buf, pm_ptr + pos, len))
		hw::panic();
	if(m_callbacks != nullptr)
		m_callbacks->buffer_readCallback(buf, len, pos);
}


utility::Buffer::Buffer(void *const ptr, size_t const len) : pm_ptr(static_cast<uint8_t *>(ptr)), pm_len(len) {}

bool utility::Buffer::invalidTransfer(size_t const pos, size_t const len) const
{
	//Check for invalid position, data too large, and overflow.
	//Note: This is still breakable, there is at least one condition I didn't check
	//One may also be redundant
	return pos >= pm_len || (pm_len - pos) < len || (pos + len) < pos || (pm_len - len) >= pm_len;
}
