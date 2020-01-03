//Created: 6/12/2018 1:12:28 AM

//copydoc doesn't work for some reason when trying to copy a file, unless I'm doing something wrong.
//\copydoc utility.h
/** \file
 * \brief Source file for utility.h.
 * \details See \ref utility.h for more information.
 * \date Created 2018-12-06
 * \author Teddy.Hut
 */

#include "utility.h"

/** This function is automatically called whenever `new` is called.
 * 
 * Calls `malloc()` in an `ATOMIC_BLOCK`.
 * If `malloc()` fails, calls hw::panic().
 *
 * \param [in] len The size of the block to allocate (in bytes).
 *
 * \return Pointer to memory allocated with `malloc()`.
 */
void *operator new(unsigned int len) {
	void *rtrn;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		rtrn = malloc(len);
	}
	if(rtrn == nullptr) libmodule::hw::panic();
	return rtrn;
}


/** This function is automatically called whenever placement `new` is called.
 * 
 * Returns the pointer with no checks.
 *
 * \param [in] len Size of the allocated block (in bytes).
 * \param [in] ptr Pre-allocated pointer for use with placement new.
 * 
 * \return Returns \p ptr unchanged.
 */
void * operator new(unsigned int len, void *ptr)
{
	return ptr;
}

/** This function is automatically called whenever `delete` is called.
 * 
 * Calls \c free() in an \c ATOMIC_BLOCK.
 * 
 * \param [in] ptr Pointer passed to free.
 * \param [in] len Size of the allocated block (in bytes). Unused.
 */
void operator delete(void * ptr, unsigned int len)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		free(ptr);
	}
}


/** This function is called when a pure virtual function with no implementation is called.
 */
void __cxa_pure_virtual()
{
	libmodule::hw::panic();
}

//toggle() is documented in utility.h
void libmodule::utility::Output<bool>::toggle() {}

/** Internally \link write(void const *const, size_t const, size_t const) write \endlink is called, so a write callback for the byte changed will be generated. If there is no change, no write operation is performed so no callback is generated.
 * \param [in] pos Position offset of byte containing bit to write.
 * \param [in] sig Significance of bit to write (in range [0, 7]).
 * \param [in] state Value to set bit to.
 */
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

//Probably could rephrase this comment block a bit.
/** The byte written is a binary OR of \p mask and the byte at \p pos.
 * \n Internally [write] is called, so a write callback for the byte changed will be generated. If there is no change, no write operation is performed so no callback is generated.
 * \param [in] pos Position offset of byte containing bits set.
 * \param [in] mask Byte with bits set in positions to be set in target byte.
 * [write]: \ref write(void const *const, size_t const, size_t const)
 */
void libmodule::utility::Buffer::bit_set_mask(size_t const pos, uint8_t const mask)
{
	uint8_t val = pm_ptr[pos] | mask;
	if(val != pm_ptr[pos])
		write(static_cast<void const *>(&val), sizeof(uint8_t), pos);
}

/** Equivalent to #bit_set(\p pos, \p sig, \c false).
 * \param [in] pos Position offset of byte containing bit to clear.
 * \param [in] sig Significance of bit to clear (in range [0, 7]).
 * 
 */
void libmodule::utility::Buffer::bit_clear(size_t const pos, uint8_t const sig)
{
	bit_set(pos, sig, false);
}

/** The byte written is a binary AND of the inverse of \p mask and the byte at \p pos.
 * \n Internally [write] is called, so a write callback for the byte changed will be generated. If there is no change, no write operation is performed so no callback is generated.
 * \param [in] pos Position offset of byte containing bits to clear.
 * \param [in] mask Byte with bits set in positions to be cleared in target byte.
 [write]: \ref write(void const *const, size_t const, size_t const)
 */
void libmodule::utility::Buffer::bit_clear_mask(size_t const pos, uint8_t const mask)
{
	uint8_t val = pm_ptr[pos] & ~mask;
	if(val != pm_ptr[pos])
		write(static_cast<void const *>(&val), sizeof(uint8_t), pos);
}

/** Internally [read] is called, so a read callback for the byte read will be generated.
 * \param [in] pos Position offset of byte containing bit to read.
 * \param [in] sig Significance of bit to read (in range [0, 7]).
 * \return Bit value.
 [read]: \ref read(void *const, size_t const, size_t const) const
 */
bool libmodule::utility::Buffer::bit_get(size_t const pos, uint8_t const sig) const
{
	uint8_t val;
	read(static_cast<void *>(&val), sizeof(uint8_t), pos);
	return val & 1 << sig;
}

/** Equivalent to [write](\ref write(void const *const, size_t const, size_t const))(\p buf, \p len, \a #pm_pos).
 * \param [in] buf Pointer to the source memory for the write.
 * \param [in] len Number of bytes to write.
 */
void libmodule::utility::Buffer::write(void const *const buf, size_t const len)
{
	write(buf, len, pm_pos);
}

/** If an invalid transfer is specified or there is a memory transfer error, hw::panic() is called.
 * \n The write operation will not wrap around to the start if the end is reached - this is considered an invalid transfer.
 * \n \a #pm_pos is set to the position past the last byte of the write operation.
 * \n If \a #m_callbacks is not \c nullptr, Callbacks::buffer_writeCallback is called.
 * \param [in] buf Pointer to the source memory for the write.
 * \param [in] len Number of bytes to write.
 * \param [in] pos The position offset from \a #pm_ptr for the write.
 * \sa invalidTransfer
 */
void libmodule::utility::Buffer::write(void const *const buf, size_t const len, size_t const pos)
{
	//Presently this will not wrap around to the start and write remaining data if the end is reached
	if(invalidTransfer(pos, len) || !memcpy(pm_ptr + pos, buf, len))
		hw::panic();
	pm_pos = pos + len;
	if(m_callbacks != nullptr)
		m_callbacks->buffer_writeCallback(buf, len, pos);
}

/** Equivalent to [read](\ref read(void *const, size_t const))(\p buf, \p len, \a #pm_pos).
 * \param [out] buf Pointer to the destination memory for the read.
 * \param [in] len Number of bytes to read.
 */
void libmodule::utility::Buffer::read(void *const buf, size_t const len)
{
	read(buf, len, pm_pos);
}

/** \a #pm_pos is set to the position past the last byte of the read operation.
 * \n Internally calls [read const](\ref read(void *const, size_t const, size_t const) const) so a read callback will be generated.
 * \param [out] buf Pointer to the destination memory for the read.
 * \param [in] len Number of bytes to read.
 * \param [in] pos The position offset from \a #pm_ptr for the read.
 */
void libmodule::utility::Buffer::read(void *const buf, size_t const len, size_t const pos)
{
	pm_pos = pos + len;
	static_cast<Buffer const *>(this)->read(buf, len, pos);
}

/** If an invalid transfer is specified or there is a memory transfer error, hw::panic() is called.
 * \n The read operation will not wrap around to the start if the end is reached - this is considered an invalid transfer.
 * \n If \a #m_callbacks is not \c nullptr, Callbacks::buffer_readCallback is called.
 * \note As this is a \c const overload, \a #pm_pos is not changed.
 * \param [out] buf Pointer to the destination memory for the read.
 * \param [in] len Number of bytes to read.
 * \param [in] pos The position offset from \a #pm_ptr for the read.
 * \sa invalidTransfer
 */
void libmodule::utility::Buffer::read(void *const buf, size_t const len, size_t const pos) const
{
	if(invalidTransfer(pos, len) || !memcpy(buf, pm_ptr + pos, len))
		hw::panic();
	if(m_callbacks != nullptr)
		m_callbacks->buffer_readCallback(buf, len, pos);
}

/** \param [in] ptr \a #pm_ptr initialiser.
 * \param [in] len \a #pm_len initialiser.
 */
libmodule::utility::Buffer::Buffer(void *const ptr, size_t const len) : pm_ptr(static_cast<uint8_t *>(ptr)), pm_len(len) {}

//For some reason the dontinclude/line commands didn't work. Only snippet worked.
/** Checks for invalid position, whether the data will be too large, and for an overflow.
 * The specific checks that are conducted are given in utility.cpp:
 * \snippet utility.cpp invalidTransfer logic
 * \param [in] pos The position offset from \a #pm_ptr for the transfer.
 * \param [in] len The length of the transfer (in bytes).
 * \return \c true if an invalid transfer is detected. \c false otherwise.
 * \bug I think Doxygen has a bug (or I'm not using it correctly). The markers below shouldn't show in the documentation.
 */
bool libmodule::utility::Buffer::invalidTransfer(size_t const pos, size_t const len) const
{
	//Check for invalid position, data too large, and overflow.
	//Note: This is still breakable, there is at least one condition I didn't check
	//One may also be redundant
/// [invalidTransfer logic]
	return pos >= pm_len || (pm_len - pos) < len || (pos + len) < pos || (pm_len - len) >= pm_len;
/// [invalidTransfer logic]
}
