/*
 * utility.h
 *
 * Created: 3/12/2018 1:08:00 PM
 *  Author: teddy
 */ 

#pragma once

#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <util/atomic.h>

//Because avr-gcc is missing basic C++ components
void *operator new(unsigned int len);
void operator delete(void * ptr, unsigned int len);
extern "C" {
void __cxa_pure_virtual();
}

namespace libmodule {

namespace hw {
	void panic();
}

namespace utility {
	template <typename T>
	struct Input {
		virtual T get() const = 0;
	};
	template <typename T>
	struct Output {
		virtual void set(T const p) = 0;
	};

	template <typename T>
	constexpr T fullMask(T const size) {
		T rtrn = 0;
		for(size_t i = 0; i < size; i++) {
			rtrn |= 0xff << (i * 8);
		}
		return rtrn;
	}
	//Straight from here: https://en.cppreference.com/w/cpp/algorithm/min
	template <typename T>
	const T& min(const T& a, const T& b)
	{
		return (b < a) ? b : a;
	}

	//Using *mem instead of mem[] to allow for <void> as data_t
	template <typename len_t, typename data_t>
	data_t *memsizematch(data_t *mem, len_t const currentlen, len_t const matchlen) {
		if(mem == nullptr || currentlen != matchlen) {
			if(mem != nullptr)
				free(static_cast<void *>(mem));
			mem = static_cast<data_t *>(malloc(matchlen));
			if(mem == nullptr) hw::panic();
		}
		return mem;
	}
	
	/*
	//Generic class with "update" method
	class UpdateFn {
	public:
		virtual void update() = 0;
	};
	*/

	template <typename T>
	struct InStates {
		using Input_t = Input<T>;
		void update();
		InStates(Input_t const *const input = nullptr);

		Input_t const *input;
		//Determined by a static_cast<bool>(T)
		bool previous; // : 1;
		bool held; // : 1;
		bool pressed; // : 1;
		bool released; // : 1;
	};

	//Keeps a list of all the instances of class T as it is constructed/destructed
	template<typename T, typename count_t = uint8_t>
	class InstanceList {
	protected:
		using il_count_t = count_t;
		static InstanceList **il_instances;
		static count_t il_count;
	public:
		InstanceList();
		virtual ~InstanceList();
	};

	template<typename T, typename count_t>
	InstanceList<T, count_t> ** InstanceList<T, count_t>::il_instances = nullptr;

	template<typename T, typename count_t /*= uint8_t*/>
	count_t InstanceList<T, count_t>::il_count = 0;

	class Buffer {
	public:
		//If m_callbacks != nullptr, these will be called on every read/write
		class Callbacks {
			friend Buffer;
			virtual void buffer_writeCallack(void const *const buf, size_t const len, size_t const pos) = 0;
			//The 'buf' in readCallback is the buffer being read into
			virtual void buffer_readCallback(void *const buf, size_t const len, size_t const pos) = 0;
		};
		template <typename T>
		Buffer &operator<<(T const &rhs);
		template <typename T>
		Buffer &operator>>(T &rhs);

		//Serialise type given and write to the buffer, at current position
		template <typename T>
		void serialiseWrite(T const &type);
		//Serialise type given and write to the buffer, at position 'pos'
		template <typename T>
		void serialiseWrite(T const &type, size_t const pos);

		//Could do const overloads of all of these that don't change pm_pos
		template <typename T>
		T serialiseRead();
		template <typename T>
		T serialiseRead(size_t const pos);
		template <typename T>
		void serialiseRead(T &type);
		template <typename T>
		void serialiseRead(T &type, size_t const pos);
		//Potential TODO: Have it so that a reference to the data containing the type is returned
		//Therefore when the type is modified the data is also

		void bitSet(size_t const pos, uint8_t const sig, bool const state = true);
		void bitSetMask(size_t const pos, uint8_t const mask);
		void bitClear(size_t const pos, uint8_t const sig);
		void bitClearMask(size_t const pos, uint8_t const mask);
		bool bitGet(size_t const pos, uint8_t const sig) const;

		void write(void const *const buf, size_t const len);
		void write(void const *const buf, size_t const len, size_t const pos);
		void read(void *const buf, size_t const len);
		void read(void *const buf, size_t const len, size_t const pos);
		void read(void *const buf, size_t const len, size_t const pos) const;

		Buffer(void *const ptr, size_t const len);
		
		uint8_t *pm_ptr;
		size_t const pm_len;
		size_t pm_pos = 0;
		Callbacks *m_callbacks = nullptr;
	private:
		inline bool invalidTransfer(size_t const pos, size_t const len) const;
	};

	template<size_t len_c>
	class StaticBuffer : public Buffer {
	public:		
		StaticBuffer();
	protected:
		uint8_t pm_buf[len_c];
	};
} //utility
} //libmodule

template<typename T, typename count_t>
libmodule::utility::InstanceList<T, count_t>::InstanceList()
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		//Reallocate memory with space for the new element
		il_instances = static_cast<InstanceList **>(realloc(static_cast<void *>(il_instances), sizeof(InstanceList *) * ++il_count));
		if(il_instances == nullptr) hw::panic();
		//Add the element to the end
		il_instances[il_count - 1] = this;
	}
}

template<typename T, typename count_t /*= uint8_t*/>
libmodule::utility::InstanceList<T, count_t>::~InstanceList()
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		//Find the position of the element to be removed
		count_t pos = 0;
		for(; pos < il_count; pos++) {
			if(il_instances[pos] == this) break;
		}
		//Cannot find this
		if(pos >= il_count) hw::panic();

		//Move the memory past the element to fill in the gap
		if(memmove(il_instances + pos, il_instances + pos + 1, sizeof(InstanceList *) * (--il_count - pos)) == nullptr) hw::panic();
		//Reallocate memory without the old element
		il_instances = static_cast<InstanceList **>(realloc(static_cast<void *>(il_instances), sizeof(InstanceList *) * il_count));
		if(il_instances == nullptr) hw::panic();
	}
}

template <typename T>
libmodule::utility::Buffer & libmodule::utility::Buffer::operator<<(T const &rhs)
{
	serialiseWrite(rhs);
	return *this;
}

template <typename T>
libmodule::utility::Buffer & libmodule::utility::Buffer::operator>>(T &rhs)
{
	//Making this use type deduction prevents it from selecting the size_t overload
	serialiseRead(rhs);
	return *this;
}

template <typename T>
void libmodule::utility::Buffer::serialiseWrite(T const &type)
{
	serialiseWrite(type, pm_pos);
}

template <typename T>
void libmodule::utility::Buffer::serialiseWrite(T const &type, size_t const pos)
{
	//Presently this will not wrap around to the start and write remaining data if the end is reached
	write(static_cast<void const *>(&type), sizeof(T), pos);
}

template <typename T>
T libmodule::utility::Buffer::serialiseRead()
{
	T rtrn;
	serialiseRead<T>(rtrn, pm_pos);
	return rtrn;
}

template <typename T>
T libmodule::utility::Buffer::serialiseRead(size_t const pos)
{
	//This assumes implicitly constructible (or is it default constructible/ To tired...)
	//The alternative is to put the data into a buffer, and then reinterpret it as the type... seems dodgy.
	T rtrn;
	serialiseRead<T>(rtrn, pos);
	return rtrn;
}

template <typename T>
void libmodule::utility::Buffer::serialiseRead(T &type)
{
	serialiseRead<T>(type, pm_pos);
}

template <typename T>
void libmodule::utility::Buffer::serialiseRead(T &type, size_t const pos)
{
	read(static_cast<void *>(&type), sizeof(T), pos);
}

template<size_t len_c>
libmodule::utility::StaticBuffer<len_c>::StaticBuffer() : Buffer(pm_buf, len_c) {}

template <typename T>
void libmodule::utility::InStates<T>::update()
{
	if(input != nullptr)
		held = static_cast<bool>(input->get());
	//Explicitly using boolean states helps with readability
	pressed = previous == false && held == true;
	released = previous == true && held == false;
	previous = held;
}


template <typename T>
libmodule::utility::InStates<T>::InStates(Input_t const *const input) : input(input) {
	previous = false;
	held = false;
	pressed = false;
	released = false;
}
