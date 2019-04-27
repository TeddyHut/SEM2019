/*
 * utility.cpp
 *
 * Created: 6/12/2018 1:12:28 AM
 *  Author: teddy
 */ 

#include "utility.h"

void *operator new(unsigned int len) {
	return malloc(len);
}

void * operator new(unsigned int len, void *ptr)
{
	return ptr;
}

void operator delete(void * ptr, unsigned int len)
{
	free(ptr);
}

void __cxa_pure_virtual()
{
	libmodule::hw::panic();
}

void libmodule::utility::Output<bool>::toggle() {}

void libmodule::utility::Buffer::bit_set(size_t const pos, uint8_t const sig, bool const state /*= true*/)
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

void libmodule::utility::Buffer::bit_set_mask(size_t const pos, uint8_t const mask)
{
	uint8_t val = pm_ptr[pos] | mask;
	if(val != pm_ptr[pos])
		write(static_cast<void const *>(&val), sizeof(uint8_t), pos);
}

void libmodule::utility::Buffer::bit_clear(size_t const pos, uint8_t const sig)
{
	bit_set(pos, sig, false);
}

void libmodule::utility::Buffer::bit_clear_mask(size_t const pos, uint8_t const mask)
{
	uint8_t val = pm_ptr[pos] & ~mask;
	if(val != pm_ptr[pos])
		write(static_cast<void const *>(&val), sizeof(uint8_t), pos);
}

bool libmodule::utility::Buffer::bit_get(size_t const pos, uint8_t const sig) const
{
	uint8_t val;
	read(static_cast<void *>(&val), sizeof(uint8_t), pos);
	return val & 1 << sig;
}

void libmodule::utility::Buffer::write(void const *const buf, size_t const len)
{
	write(buf, len, pm_pos);
}

void libmodule::utility::Buffer::write(void const *const buf, size_t const len, size_t const pos)
{
	//Presently this will not wrap around to the start and write remaining data if the end is reached
	if(invalidTransfer(pos, len) || !memcpy(pm_ptr + pos, buf, len))
		hw::panic();
	pm_pos = pos + len;
	if(m_callbacks != nullptr)
		m_callbacks->buffer_writeCallack(buf, len, pos);
}

void libmodule::utility::Buffer::read(void *const buf, size_t const len)
{
	read(buf, len, pm_pos);
}

void libmodule::utility::Buffer::read(void *const buf, size_t const len, size_t const pos)
{
	pm_pos = pos + len;
	static_cast<Buffer const *>(this)->read(buf, len, pos);
}

void libmodule::utility::Buffer::read(void *const buf, size_t const len, size_t const pos) const
{
	if(invalidTransfer(pos, len) || !memcpy(buf, pm_ptr + pos, len))
		hw::panic();
	if(m_callbacks != nullptr)
		m_callbacks->buffer_readCallback(buf, len, pos);
}

libmodule::utility::Buffer::Buffer(void *const ptr, size_t const len) : pm_ptr(static_cast<uint8_t *>(ptr)), pm_len(len) {}

bool libmodule::utility::Buffer::invalidTransfer(size_t const pos, size_t const len) const
{
	//Check for invalid position, data too large, and overflow.
	//Note: This is still breakable, there is at least one condition I didn't check
	//One may also be redundant
	return pos >= pm_len || (pm_len - pos) < len || (pos + len) < pos || (pm_len - len) >= pm_len;
}
