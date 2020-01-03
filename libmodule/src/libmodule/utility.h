//Created: 3/12/2018 1:08:00 PM

/** \file
 * \brief Independent utilities and conveniences.
 * \details utility.h contains utility classes and functions (e.g. memory management, generic classes) that do not depend on any other libmodule files.
 * 
 * \date Created 2018-12-03
 * \author Teddy.Hut
 */

#pragma once

#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <util/atomic.h>

/** \defgroup cppfunctions C++ Required Functions
 * \brief Necessary C++ functions with no compiler implementation.
 *
 * `avr-gcc` does not seem to implicitly define the global replacements of `operator new` or `operator delete`, unlike most other compilers.
 * See [here](https://en.cppreference.com/w/cpp/memory/new/operator_new#Global_replacements) for more information.
 * \n `avr-gcc` also has no C++ standard library, so `<new>`, where these functions are usually defined, does not exist and therefore cannot be included.
 * This means they must be implemented manually.
 * @{
 */

///[atomic] C++ `new` implementation.
void *operator new(unsigned int len);
///C++ placement `new` implementation.
void *operator new(unsigned int len, void *ptr);
///[atomic] C++ `delete` implementation.
void operator delete(void * ptr, unsigned int len);
extern "C" {
///GCC pure `virtual` function implementation.
void __cxa_pure_virtual();
}

/**@}*/

//This one is useful enough to have in the global namespace
/**
 * \brief Casts a typed \c enum to an \c int.
 *
 * While \c int_t does not have to be an integer, that's how it's usually used.
 * \tparam int_t Type to cast to.
 * \tparam enum_t [implicit] \c enum type.
 * \param [in] p \c enum value.
 * \return \p p casted as \c enum_t.
 * \remark \c ecast is useful enough to be in the global namespace.
 */
template<typename int_t = uint8_t, typename enum_t>
constexpr int_t ecast(enum_t const p) {
	return static_cast<int_t>(p);
}

namespace libmodule {

namespace hw {
	/** \brief Called when an unrecoverable error occurs.
	 *
	 * This function should be project dependent.
	 * \todo Make a default 'weak' implementation.
	 */
	void panic();
}

///Primary namespace of utility.h
namespace utility {
	/** \brief Abstract input template class.
	 *
	 * This class is used for various input abstractions throughout libmodule.
	 * Having a simple class like this prevents repetition of the same structure in other places.
	 * \sa InStates userio::ButtonTimer userio::RapidInput
	 * \tparam T Input type.
	 * \author Teddy.Hut
	 */
	template <typename T>
	struct Input {
		/** \brief Gets the value of the input.
		 * \return Value of the input.
		 */
		virtual T get() const = 0;
	};

	/** \brief Abstract output template class.
	 *
	 * This class is used for various output abstractions throughout libmodule.
	 * Having a simple class like this prevents repetition of the same structure in other places.
	 * \sa userio::BlinkerTimer
	 * \tparam T Output type.
	 * \author Teddy.Hut
	 */
	template <typename T>
	struct Output {
		/** \brief Sets the value of the output.
		 * \param [in] p Output value.
		*/
		virtual void set(T const p) = 0;
	};

	/** \brief Specialisation of utility::Output for \c bool.
	 *
	 * This specialisation adds the \c toggle() function. When using a digital output, sometimes one only wants to toggle the output.
	 * A \c toggle() function allows inherited classes to optimise for cases where a toggle is needed, if possible.
	 * \author Teddy.Hut
	 */
	template <>
	struct Output<bool> {
		///\copydoc libmodule::utility::Output::set(T const p)
		virtual void set(bool const p) = 0;
		/** \brief Toggles the output if implemented by an inherited class.
		* \note Unless an inherited class overrides this function, it does nothing.
		*/
		virtual void toggle();
	};

	/** \brief Create a value consisting partly or entirely of binary 1s.
	 *
	 * The value is created by progressively or-ing \c 0xff starting from the least significant byte (on the right) and moving to the most significant byte (on the left).
	 *
	 * \tparam T Type to return.
	 * \param [in] size Size of value (in bytes).
	 * \return Created value.
	 */
	template <typename T>
	constexpr T fullMask(T const size) {
		T rtrn = 0;
		for(size_t i = 0; i < size; i++) {
			rtrn |= 0xff << (i * 8);
		}
		return rtrn;
	}
	/** \brief Returns the smaller of two values.
	 *
	 * Values are compared using `operator <`.
	 * \n This function was taken from [here](https://en.cppreference.com/w/cpp/algorithm/min).
	 * \tparam T [implicit] Type of values to compare.
	 * \param [in] a Value 1.
	 * \param [in] b Value 2.
	 * \return The smaller of \p a and \p b, or \p a if they are equal.
	 */
	template <typename T>
	const T& tmin(const T& a, const T& b)
	{
		return (b < a) ? b : a;
	}
	/** \brief Returns the greater of two values.
	 *
	 * Values are compared using `operator <`.
	 * \n This function was taken from [here](https://en.cppreference.com/w/cpp/algorithm/max).
	 * \tparam T [implicit] Type of values to compare.
	 * \param [in] a Value 1.
	 * \param [in] b Value 2.
	 * \return The greater of \p a and \p b, or \p a if they are equal.
	 */
	template<typename T>
	const T& tmax(const T& a, const T& b)
	{
		return (a < b) ? b : a;
	}
	//Consider using a maths typeset for the return statement documentation.
	/** \brief Determines if a value is in a range (inclusive).
	 *
	 * \p p is compared to \p min using `operator >=`. \n\p p is compared to \p max using `operator <=`.
	 * \tparam T [implicit] Type of values to compare.
	 * \param [in] p Value to check.
	 * \param [in] min Minimum value in range.
	 * \param [in] max Maximum value in range.
	 * \return \c true if \p p is in range [\p min, \p max], \c false otherwise.
	 */
	template <typename T>
	constexpr bool within_range_inclusive(T const &p, T const &min, T const &max) {
		return (p >= min) && (p <= max);
	}
	
	/** \brief Ensures `mem` points to a memory block of size `matchlen`.
	 *
	 * If \p mem is \c nullptr, a new block is allocated.
	 * \n If \p currentlen equals \p matchlen, then \p mem is returned unchanged.
	 * \n If \p currentlen does not equal \p matchlen, \p mem is freed and a new block is allocated and returned.
	 * \n If a new block fails to allocate, hw::panic() is called.
	 * \warning This function will not preserve the contents of the previous memory block.
	 * \tparam len_t [implicit] Type of length parameters.
	 * \tparam data_t [implicit] Type of data.
	 * \param [in] mem Pointer to \c nullptr or a block of memory allocated using `malloc()`.
	 * \param [in] currentlen The size of the block of memory \p mem points to (in bytes).
	 * \param [in] matchlen The size of the block to match (in bytes).
	 * \return Pointer to a memory block with size \p matchlen.
	 */
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
	
	/** \brief Converts a decimal digit to ASCII.
	 *
	 * For example, if the input was \c 3, the output would be \c '3'.
	 * \note There is no check that the input is valid.
	 * \param [in] digit Value in range [0, 9].
	 * \return Value in range ['0', '9'].
	 */
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

	/** \brief Keeps track of boolean changes to an input.
	 * \tparam T Type to keep track of.
	 * \author Teddy.Hut
	 */
	template <typename T>
	struct InStates {
		///Internal type alias.
		using Input_t = Input<T>;
		///Updates members.
		void update();
		///Constructor. Assigns an input to the instance.
		InStates(Input_t const *const input = nullptr);

		///Pointer to input class to track.
		Input_t const *input;
		bool previous : 1; ///< Previous boolean input state.
		bool held : 1; ///< `true` if input is `true`. `false` otherwise.
		bool pressed : 1; ///< `true` if input has changed from `false` to `true` in the last update.
		bool released : 1; ///< `true` if input has changed from `true` to `false` in the last update.
	};
	
	/** \brief Dynamic element container.
	 *
	 * Vector is in some ways similar to [`std::vector`](https://en.cppreference.com/w/cpp/container/vector) but much more basic.
	 * \n Memory is allocated using `malloc()`, and is stored in a continuous memory block. `realloc()` is used to change size.
	 * \todo Add copy/move assignment operators.
	 * \tparam T Type to store.
	 * \tparam count_t Integer type used for indexing.
	 * \author Teddy.Hut
	 */
	template <typename T, typename count_t = uint8_t>
	class Vector {
	protected:
		///Pointer to internal memory block used for storage.
		T *data = nullptr;
		///Number of elements stored.
		count_t count = 0;
	public:
		///[atomic] Adds an element to the end.
		void push_back(T const &p);
		///[atomic] Inserts an element at \p pos.
		void insert(T const &p, count_t const pos);
		///Remove all elements matching \p p.
		void remove(T const &p);
		///[atomic] Removes the element at \p pos.
		void remove_pos(count_t const pos);
		///Resizes the memory block to fit \p size elements.
		void resize(count_t const size);
		
		///Returns the number of stored elements.
		count_t size() const;

		///Element access operator.
		T &operator[](count_t const pos);
		///\copydoc operator[](count_t const)
		T const &operator[](count_t const pos) const;

		///[atomic] Copy constructor.
		Vector(Vector const &p);
		///Move constructor.
		Vector(Vector &&p);
		///[atomic] Constructs with space for \p size elements.
		Vector(count_t const size = 0);
		///[atomic] Destructor. `free()`s memory.
		virtual ~Vector();
	};

	/** \brief Keeps a list that contains pointers to all instances of itself and its subclasses.
	 *
	 * The instance list (\a #il_instances) is a \c static data member. Therefore, the class is a template class with type \c T so a subclass can template instantiate a unique InstanceList for itself. This allows InstanceList to keep track of instances of that unique subclass, without the need to repeat code.
	 * \n Instances are added on object construction, and removed on object destruction.
	 * #### Example Use Case
	 * Say you wanted to keep a list of all instances of the hypothetical class \c Hugo. You could declare \c Hugo as follows:
	 * ~~~{.cpp}
	 class Hugo : public InstanceList<Hugo> {
	 };
	 * ~~~
	 * Now \c Hugo::il_instances will hold a list of all the instances.
	 * \tparam T Type of subclass to keep track of.
	 * \tparam count_t Integer type used for indexing.
	 * \author Teddy.Hut
	 */
	template<typename T, typename count_t = uint8_t>
	class InstanceList {
	protected:
		///Type alias to allow subclasses to access \c count_t.
		using il_count_t = count_t;
		///Vector that holds instance pointers.
		static Vector<T *, count_t> il_instances;
	public:
		///[atomic] Constructor. Adds \c this to end of the instance list.
		InstanceList();
		///[atomic] Destructor. Removes \c this from the instance list.
		virtual ~InstanceList();
	};

	template<typename T, typename count_t>
	Vector<T *, count_t> InstanceList<T, count_t>::il_instances;

	/** \brief Utility wrapper for a user provided memory block.
	 *
	 * Buffer offers basic serialisation of any data type, easy bit manipulation operations, and read/write callbacks via Buffer::Callbacks.
	 * \n The current write/read position is stored and updated automatically using \a #pm_pos.
	 * \n To set the data pointer and length, access the data members \a #pm_ptr and \a #pm_len directly.
	 * \n Buffer does not do any dynamic memory management.
	 * \n If an invalid transfer occurs (e.g. out of bounds), hw::panic() is called.
	 * \todo Consider whether it is worth adding read member functions that return a reference to the object being read.
	 *       That way, when modifying the returned object, the buffer is also modified.
	 * \author Teddy.Hut
	 */
	class Buffer {
	public:
		/** \brief Abstract callback class for Buffer.
		 *
		 * Inherit and overwrite the member functions of this class to use the read and write callbacks of Buffer.
		 * \note Both functions must be implemented in the subclass to prevent a pure virtual function call.
		 */
		class Callbacks {
			/** Buffer is friended since #buffer_writeCallback and #buffer_readCallback are private members of Callbacks.
			*/
			friend Buffer;
			/** \brief Write callback for Buffer.
			 *
			 * If Buffer::m_callbacks is not \c nullptr, this function will be called **after** any write operation.
			 * \param [in] buf Pointer to the source memory for the write.
			 * \param [in] len Number of bytes written.
			 * \param [in] pos The position offset from destination Buffer::pm_ptr for the write.
			 */
			virtual void buffer_writeCallback(void const *const buf, size_t const len, size_t const pos) = 0;
			//The 'buf' in readCallback is the buffer being read into
			/** \brief Read callback for Buffer.
			 *
			 * If Buffer::m_callbacks is not \c nullptr, this function will be called **after** any read operation.
			 * \param [in] buf Pointer to the destination memory for the read (memory being read *into*).
			 * \param [in] len Number of bytes read.
			 * \param [in] pos The position offset from source Buffer::pm_ptr for the read.
			 */
			virtual void buffer_readCallback(void *const buf, size_t const len, size_t const pos) = 0;
		};


		///Template stream insertion operator.
		//Doxygen required that the return value be fully qualified to associate with the detailed description below.
		template <typename T>
		libmodule::utility::Buffer &operator<<(T const &rhs);
		///Template stream extraction operator
		template <typename T>
		libmodule::utility::Buffer &operator>>(T &rhs);

		///Serialise type given and write to the buffer at \a #pm_pos.
		template <typename T>
		void serialiseWrite(T const &type);
		///Serialise type given and write to the buffer at position \p pos.
		template <typename T>
		void serialiseWrite(T const &type, size_t const pos);

		///Read data from the buffer at \a #pm_pos into a given type.
		template <typename T>
		T serialiseRead();
		
		///Read data from the buffer at \p pos into a given type.
		template <typename T>
		T serialiseRead(size_t const pos);
		///\copybrief serialiseRead(size_t const)
		template <typename T>
		T serialiseRead(size_t const pos) const;
		
		///Read data from the buffer at \a #pm_pos into \p type.
		template <typename T>
		void serialiseRead(T &type);
		
		///Read data from the buffer at \p pos into \p type.
		template <typename T>
		void serialiseRead(T &type, size_t const pos);
		
		///Writes the value \p state to bit \p sig in byte \p pos.
		void bit_set(size_t const pos, uint8_t const sig, bool const state = true);
		///Sets all bits set in \p mask at byte \p pos.
		void bit_set_mask(size_t const pos, uint8_t const mask);
		///Clears bit \p sig of byte \p pos.
		void bit_clear(size_t const pos, uint8_t const sig);
	
		///Clears all bits set in \p mask at byte \p pos.
		void bit_clear_mask(size_t const pos, uint8_t const mask);
		///Returns bit \p sig in byte \p pos.
		bool bit_get(size_t const pos, uint8_t const sig) const;

		///Writes data from \p buf to internal buffer at \a #pm_pos.
		void write(void const *const buf, size_t const len);
		///Writes data from \p buf to internal buffer at \p pos.
		void write(void const *const buf, size_t const len, size_t const pos);
		///Reads data from internal buffer at \a #pm_pos to \p buf.
		void read(void *const buf, size_t const len);
		///Reads data from internal buffer at \p pos to \p buf.
		void read(void *const buf, size_t const len, size_t const pos);
		///\copybrief read(void *const, size_t const, size_t const)
		void read(void *const buf, size_t const len, size_t const pos) const;

		///Constructor.
		Buffer(void *const ptr = nullptr, size_t const len = 0);
		///Copy constructor.
		Buffer(Buffer const &p) = default;
		
		/// \brief Pointer to a user provided block of memory.
		/// \details To set this member, access it directly or use the constructor.
		uint8_t *pm_ptr;
		/// \brief Size of the block of memory \a #pm_ptr points to (in bytes).
		/// \details To set this member, access it directly or use the constructor.
		size_t pm_len;
		/// \brief The internal buffer position.
		/// \details This value is an offset (in bytes) from the start of the buffer. After any read or write operation, this value will be the last written/read position + 1.
		size_t pm_pos = 0;
		/// \brief Pointer to callback object.
		/// \details If not equal to \c nullptr, the Callbacks members will be called after any read or write operation.
		/// \sa Callbacks 
		Callbacks *m_callbacks = nullptr;
	private:
		/// Called before any read or write operation to check for out-of-bounds conditions.
		inline bool invalidTransfer(size_t const pos, size_t const len) const;
	};

	//Static may or may not be the most correct word here. Stack may be better in some way.
	/** \brief Buffer that provides a statically allocated block of memory.
	 *
	 * StaticBuffer is a way to use the conveniences of Buffer on a static block of memory.
	 * \n \a Buffer::pm_ptr and \a Buffer::pm_len will be set by StaticBuffer.
	 * \tparam len_c Size of the memory block (in bytes).
	 * \author Teddy.Hut
	 */
	template<size_t len_c>
	class StaticBuffer : public Buffer {
	public:
		///Constructor.
		StaticBuffer();
	protected:
		///Static memory block.
		uint8_t pm_buf[len_c];
	};
} //utility
} //libmodule

//Could have the names in the second comment line be links. See also formatting could be different.
/** Adds \p p to the end (past the greatest position index) of the vector. A copy of \p p is stored.
 * \n Equivalent to #insert(\p p, \a #count).
 * \note Invalidates references to elements.
 * \param [in] p Element to add.
 * \sa Vector::insert
 */
template <typename T, typename count_t /*= uint8_t*/>
void libmodule::utility::Vector<T, count_t>::push_back(T const &p)
{
	insert(p, count);
}

/** This member function is enclosed in an `ATOMIC_BLOCK`.
 * \n If an element is already in \p pos, it and all following elements are moved one position toward the end.
 * \n If \p pos is out of range (greater than \a #count) or memory allocation fails, hw::panic() is called.
 * \n A copy of the element is created using `T`'s copy constructor.
 * \note Invalidates references to elements.
 * \param [in] p Element to insert.
 * \param [in] pos Position to insert element.
 */
template <typename T, typename count_t /*= uint8_t*/>
void libmodule::utility::Vector<T, count_t>::insert(T const &p, count_t const pos)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
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
}

/** \p p is compared to internal elements using `operator ==`.
 * \n All elements matching \p p are removed. If no elements match \p p, the function returns with no changes.
 * \n Internally calls remove_pos(), so removal is atomic.
 * \note Invalidates references to elements if deletion occurs.
 * \param [in] p Object equal to those being removed.
 */
template <typename T, typename count_t /*= uint8_t*/>
void libmodule::utility::Vector<T, count_t>::remove(T const &p)
{
	//Remove all elements that match
	for(count_t i = 0; i < count; i++) {
		if(p == data[i]) remove_pos(i);
	}
}

/** This member function is enclosed in an `ATOMIC_BLOCK`.
 * \n `T`s destructor is called on the element before it is deleted.
 * \n If \p pos is not in range or memory allocation fails, hw::panic() is called.
 * \note Invalidates reference to elements.
 * \param [in] pos Position of element to remove.
 */
template <typename T, typename count_t /*= uint8_t*/>
void libmodule::utility::Vector<T, count_t>::remove_pos(count_t const pos)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
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
			//Check for realloc error
			if(data == nullptr) hw::panic();
		}
	}
}

/** This member function is enclosed in an `ATOMIC_BLOCK`.
 * \n If the new size is greater than the current size, elements are default constructed on the end of the vector.
 * \n If the new size is smaller than the current size, elements are destructed from the end of the vector.
 * \n If memory allocation fails, hw::panic() is called.
 * \note Invalidates references to elements if size changes.
 * \param [in] size Number of elements the vector should hold.
 */
template <typename T, typename count_t /*= uint8_t*/>
void libmodule::utility::Vector<T, count_t>::resize(count_t const size)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
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
}

template <typename T, typename count_t /*= uint8_t*/>
count_t libmodule::utility::Vector<T, count_t>::size() const
{
	return count;
}

/** If \p pos is out of bounds, hw::panic() is called.
 * \param [in] pos Position of element to access.
 * \return Reference to element at \p pos.
 */
template <typename T, typename count_t /*= uint8_t*/>
T & libmodule::utility::Vector<T, count_t>::operator[](count_t const pos)
{
	if(pos >= count) hw::panic();
	return data[pos];
}

/** \return This reference is \c const.
 */
template <typename T, typename count_t /*= uint8_t*/>
T const & libmodule::utility::Vector<T, count_t>::operator[](count_t const pos) const
{
	if(pos >= count) hw::panic();
	return data[pos];
}

/** This member function is enclosed in an `ATOMIC_BLOCK`.
 * \n Elements are copied using `T`'s copy constructor.
 * \n If memory allocation fails, hw::panic() is called.
 * \param [in] p Vector to copy.
 */
template <typename T, typename count_t /*= uint8_t*/>
libmodule::utility::Vector<T, count_t>::Vector(Vector const &p) : count(p.count)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
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
}
/** \p p has \a #data set to \c nullptr and \a #count set to \c 0.
 * \param [in,out] p Source vector.
 */
template <typename T, typename count_t /*= uint8_t*/>
libmodule::utility::Vector<T, count_t>::Vector(Vector &&p) : data(p.data), count(p.count)
{
	p.data = nullptr;
	p.count = 0;
}

/** Calls #resize(\c size). New elements are default constructed.
 * \param [in] size Number of elements the vector should hold.
 * \sa Vector::resize
 */
template <typename T, typename count_t /*= uint8_t*/>
libmodule::utility::Vector<T, count_t>::Vector(count_t const size)
{
	resize(size);
}

/** Calls #resize(\c 0).
 * \sa Vector::resize
 */
template <typename T, typename count_t /*= uint8_t*/>
libmodule::utility::Vector<T, count_t>::~Vector()
{
	resize(0);
}

/** This member function is enclosed in an `ATOMIC_BLOCK`.
 */
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

/** Equivalent to #serialiseWrite(\p rhs).
 * \tparam T [implicit] Type to write.
 * \param [in] rhs Object to write.
 * \return Reference to \c this.
 * \sa serialiseWrite(T const &)
 */
template <typename T>
libmodule::utility::Buffer & libmodule::utility::Buffer::operator<<(T const &rhs)
{
	serialiseWrite(rhs);
	return *this;
}

/** Equivalent to \link serialiseRead(T&) serialiseRead\endlink(\p rhs).
 * \tparam T [implicit] Type to read into.
 * \param [out] rhs Object to read into.
 * \return Reference to \c this.
 * \sa serialiseRead(T &)
 */
template <typename T>
libmodule::utility::Buffer & libmodule::utility::Buffer::operator>>(T &rhs)
{
	//Making this use type deduction prevents it from selecting the size_t overload
	serialiseRead(rhs);
	return *this;
}


/** Equivalent to \link serialiseWrite(T const &, size_t const) serialiseWrite\endlink(\p type, \a #pm_pos).
 * \tparam T [implicit] Type to write.
 * \param [in] type Object to write.
 * \sa serialiseWrite(T const &, size_t const)
 */
template <typename T>
void libmodule::utility::Buffer::serialiseWrite(T const &type)
{
	serialiseWrite(type, pm_pos);
}

/** No fancy serialisation techniques are used. The address of \p type is simply converted to a `void const *`, and the length is `sizeof(T)`.
 * \n Calls #write(void const *const, size_t const, size_t const).
 * \tparam T [implicit] Type to write.
 * \param [in] type Object to write.
 * \param [in] pos Position offset to write (in bytes).
 * \sa write(void const *const, size_t const, size_t const)
 */
template <typename T>
void libmodule::utility::Buffer::serialiseWrite(T const &type, size_t const pos)
{
	//Presently this will not wrap around to the start and write remaining data if the end is reached
	write(static_cast<void const *>(&type), sizeof(T), pos);
}


/** Calls #serialiseRead<T>(T &, size_t const) and returns result.
 * \tparam T Type to read into.
 * \return Object created by read.
 * \sa serialiseRead<T>(T &, size_t const)
 */
template <typename T>
T libmodule::utility::Buffer::serialiseRead()
{
	T rtrn;
	serialiseRead<T>(rtrn, pm_pos);
	return rtrn;
}


/** Calls #serialiseRead<T>(T &, size_t const pos) and returns result.
 * \tparam T Type to read into.
 * \param [in] pos Position offset to read from (in bytes).
 * \return Object created by read.
 */
template <typename T>
T libmodule::utility::Buffer::serialiseRead(size_t const pos)
{
	//This assumes implicitly constructible (or is it default constructible/ To tired...)
	//The alternative is to put the data into a buffer, and then reinterpret it as the type... seems dodgy.
	T rtrn;
	serialiseRead<T>(rtrn, pos);
	return rtrn;
}

/** Calls \link read(void *const, size_t const, size_t const) const read const \endlink to read data. 
 * \note This const overload does not update \a #pm_pos.
 * \tparam T Type to read into.
 * \param [in] pos Position offset to read from (in bytes).
 * \return Object created by read.
 * \sa serialiseRead(T &, size_t const)
 */
template <typename T>
T libmodule::utility::Buffer::serialiseRead(size_t const pos) const
{
	T rtrn;
	read(static_cast<void *>(&rtrn), sizeof(T), pos);
	return rtrn;
}

/** Equivalent to \link serialiseRead<T>(T &, size_t const) serialiseRead\endlink(\p type, \a #pm_pos).
 * \tparam T Type to read into.
 * \param [out] type Object to read into.
 */
template <typename T>
void libmodule::utility::Buffer::serialiseRead(T &type)
{
	serialiseRead<T>(type, pm_pos);
}

/** Calls \link read(void *const, size_t const, size_t const) read\endlink to read data.
 * No fancy serialisation techniques are used. The address of \p type is simply converted to a `void *`, and the length is `sizeof(T)`.
 * \tparam T Type to read into.
 * \param [out] type Type to read into.
 * \param [in] pos Position offset to read from (in bytes).
 */
template <typename T>
void libmodule::utility::Buffer::serialiseRead(T &type, size_t const pos)
{
	read(static_cast<void *>(&type), sizeof(T), pos);
}

/** Sets \a Buffer::pm_ptr to \a #pm_buf and \a Buffer::pm_len to \c len_c using Buffer::Buffer.
 */
template<size_t len_c>
libmodule::utility::StaticBuffer<len_c>::StaticBuffer() : Buffer(pm_buf, len_c) {}

/** This method is intended to be called once per program cycle. It will poll the input and update the variables.
 * \n The input is polled using \link Input::get input->get()\endlink, and is converted to a boolean using `static_cast<bool>`.
 */
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

/** All members are set to false on construction.
 * \param [in] input Pointer to an object of type `Input<T>` to be tracked.
 */
template <typename T>
libmodule::utility::InStates<T>::InStates(Input_t const *const input) : input(input) {
	previous = false;
	held = false;
	pressed = false;
	released = false;
}
