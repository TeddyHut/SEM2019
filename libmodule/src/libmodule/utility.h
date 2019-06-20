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
void *operator new(unsigned int len, void *ptr);
void operator delete(void * ptr, unsigned int len);
extern "C" {
void __cxa_pure_virtual();
}

//This one is useful enough to have in the global namespace
template<typename int_t = uint8_t, typename enum_t>
constexpr int_t ecast(enum_t const p) {
	return static_cast<int_t>(p);
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

	template <>
	struct Output<bool> {
		virtual void set(bool const p) = 0;
		virtual void toggle();
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
	const T& tmin(const T& a, const T& b)
	{
		return (b < a) ? b : a;
	}
	template<typename T>
	const T& tmax(const T& a, const T& b)
	{
		return (a < b) ? b : a;
	}
	template <typename T>
	constexpr bool within_range_inclusive(T const &p, T const &min, T const &max) {
		return (p >= min) && (p <= max);
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
	
	constexpr char digit_to_ascii(uint8_t const digit) {
		return digit + '0';
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
		bool previous : 1;
		bool held : 1;
		bool pressed : 1;
		bool released : 1;
	};
	
	template <typename T, typename count_t = uint8_t>
	class Vector {
	protected:
		T *data = nullptr;
		count_t count = 0;
	public:
		//Add a new element to the end - probably invalidates references.
		void push_back(T const &p);
		//Add a new element at pos - invalidates references.
		void insert(T const &p, count_t const pos);
		//Remove an element matching p. Invalidates references.
		void remove(T const &p);
		//Remove eleent at pos. Invalidates references.
		void remove_pos(count_t const pos);
		//Resize to fit size number of elements. Invalidates references.
		void resize(count_t const size);
		
		count_t size() const;

		T &operator[](count_t const pos);
		T const &operator[](count_t const pos) const;

		//TODO: Add copy/move assignment operators
		Vector(Vector const &p);
		Vector(Vector &&p);
		Vector(count_t const size = 0);
		virtual ~Vector();
	};

	//Keeps a list of all the instances of class T as it is constructed/destructed
	template<typename T, typename count_t = uint8_t>
	class InstanceList {
	protected:
		using il_count_t = count_t;
		static Vector<T *, count_t> il_instances;
	public:
		InstanceList();
		virtual ~InstanceList();
	};

	template<typename T, typename count_t>
	Vector<T *, count_t> InstanceList<T, count_t>::il_instances;

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
		T serialiseRead(size_t const pos) const;
		template <typename T>
		void serialiseRead(T &type);
		template <typename T>
		void serialiseRead(T &type, size_t const pos);
		//Potential TODO: Have it so that a reference to the data containing the type is returned
		//Therefore when the type is modified the data is also

		void bit_set(size_t const pos, uint8_t const sig, bool const state = true);
		//Sets all bits that are set in mask at pos
		void bit_set_mask(size_t const pos, uint8_t const mask);
		void bit_clear(size_t const pos, uint8_t const sig);
		//Clears all bits that are set in mask at pos
		void bit_clear_mask(size_t const pos, uint8_t const mask);
		bool bit_get(size_t const pos, uint8_t const sig) const;

		void write(void const *const buf, size_t const len);
		void write(void const *const buf, size_t const len, size_t const pos);
		void read(void *const buf, size_t const len);
		void read(void *const buf, size_t const len, size_t const pos);
		void read(void *const buf, size_t const len, size_t const pos) const;

		Buffer(void *const ptr = nullptr, size_t const len = 0);
		Buffer(Buffer const &p) = default;
		
		uint8_t *pm_ptr;
		size_t pm_len;
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

template <typename T, typename count_t /*= uint8_t*/>
void libmodule::utility::Vector<T, count_t>::push_back(T const &p)
{
	insert(p, count);
}

template <typename T, typename count_t /*= uint8_t*/>
void libmodule::utility::Vector<T, count_t>::insert(T const &p, count_t const pos)
{
	//Cannot insert at end + 1
		if(pos > count) hw::panic();
	//If memory is not allocated (size is 0)
	if(count == 0 && data == nullptr) {
		data = static_cast<T *>(malloc(sizeof(T)));
		count++;
	}
	else {
		//Reallocate memory with space for the new element
		data = static_cast<T *>(realloc(static_cast<void *>(data), sizeof(T) * ++count));
	}
	if(data == nullptr) hw::panic();
	//Move the following elements out of the way
	memmove(data + pos + 1, data + pos, sizeof(T) * (count - pos - 1));
	//Construct element using placement-new and copy-constructor
	new(&(data[pos])) T(p);
}

template <typename T, typename count_t /*= uint8_t*/>
void libmodule::utility::Vector<T, count_t>::remove(T const &p)
{
	//Remove all elements that match
	for(count_t i = 0; i < count; i++) {
		if(p == data[i])
			remove_pos(i);
	}
}

template <typename T, typename count_t /*= uint8_t*/>
void libmodule::utility::Vector<T, count_t>::remove_pos(count_t const pos)
{
	//Deallocate the element at pos
	if(pos >= count) hw::panic();
	data[pos].~T();
	//If now empty, free memory
	if(--count == 0) {
		free(data);
		data = nullptr;
	}
	else {
		//Move the memory on top of the element to fill in the gap
		if(memmove(data + pos, data + pos + 1, sizeof(T) * (count - pos)) == nullptr) hw::panic();
		//Reallocate memory without the old element
		data = static_cast<T *>(realloc(static_cast<void *>(data), sizeof(T) * count));
	}
}

template <typename T, typename count_t /*= uint8_t*/>
void libmodule::utility::Vector<T, count_t>::resize(count_t const size)
{
	if(size > count) {
		//If memory is not allocated
		if(count == 0 && data == nullptr)
			data = static_cast<T *>(malloc(sizeof(T) * size));
		else
			data = static_cast<T *>(realloc(static_cast<void *>(data), sizeof(T) * size));
		if(data == nullptr) hw::panic();
		//Default initialize objects
		for(count_t i = count; i < size; i++) {
			new(&(data[i])) T;
		}
		count = size;
	}
	else if(size < count) {
		//Destruct objects to be removed
		for(count_t i = size; i < count; i++) {
			data[i].~T();
		}
		//If now empty, free memory
		if(size == 0) {
			free(data);
			data = nullptr;
		}
		else {
			//Resize memory
			data = static_cast<T *>(realloc(static_cast<void *>(data), sizeof(T) * size));
			if(data == nullptr) hw::panic();
		}
		count = size;
	}
}

template <typename T, typename count_t /*= uint8_t*/>
count_t libmodule::utility::Vector<T, count_t>::size() const
{
	return count;
}

template <typename T, typename count_t /*= uint8_t*/>
T & libmodule::utility::Vector<T, count_t>::operator[](count_t const pos)
{
	if(pos >= count) hw::panic();
	return data[pos];
}

template <typename T, typename count_t /*= uint8_t*/>
T const & libmodule::utility::Vector<T, count_t>::operator[](count_t const pos) const
{
	if(pos >= count) hw::panic();
	return data[pos];
}

template <typename T, typename count_t /*= uint8_t*/>
libmodule::utility::Vector<T, count_t>::Vector(Vector const &p) : count(p.count)
{
	if(count > 0) {
		//Allocate new memory
		data = static_cast<T *>(malloc(sizeof(T) * count));
		if(data == nullptr) hw::panic();
		//Copy construct objects from p
		for(count_t i = 0; i < count; i++) {
			new(&(data[i])) T(p.data[i]);
		}
	}
}

template <typename T, typename count_t /*= uint8_t*/>
libmodule::utility::Vector<T, count_t>::Vector(Vector &&p) : data(p.data), count(p.count)
{
	p.data = nullptr;
	p.count = 0;
}

template <typename T, typename count_t /*= uint8_t*/>
libmodule::utility::Vector<T, count_t>::Vector(count_t const size)
{
	resize(size);
}

template <typename T, typename count_t /*= uint8_t*/>
libmodule::utility::Vector<T, count_t>::~Vector()
{
	resize(0);
}

template<typename T, typename count_t>
libmodule::utility::InstanceList<T, count_t>::InstanceList()
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		il_instances.push_back(static_cast<T *>(this));
	}
}

template<typename T, typename count_t /*= uint8_t*/>
libmodule::utility::InstanceList<T, count_t>::~InstanceList()
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		il_instances.remove(static_cast<T *>(this));
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
T libmodule::utility::Buffer::serialiseRead(size_t const pos) const
{
	T rtrn;
	read(static_cast<void *>(&rtrn), sizeof(T), pos);
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
