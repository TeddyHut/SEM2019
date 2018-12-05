/*
 * utility.h
 *
 * Created: 3/12/2018 1:08:00 PM
 *  Author: teddy
 */ 

#pragma once

#include <stdlib.h>
#include <avr/io.h>
#include <util/atomic.h>

//Because avr-gcc is missing basic C++ components
void operator delete(void * ptr, unsigned int len);
extern "C" {
void __cxa_pure_virtual();
}

namespace hw {
	namespace PINPOS {
		enum e {
			LED_RED = 4,
			LED_GREEN = 5,
		};
	}
	namespace PINMASK {
		enum e {
			LED_RED = 1 << PINPOS::LED_RED,
			LED_GREEN = 1 << PINPOS::LED_GREEN,
		};
	}
	//Turn both LEDs on and break into debugger
	inline void panic() {
		PORTA.DIRSET = PINMASK::LED_RED | PINMASK::LED_GREEN;
		PORTA.OUTSET = PINMASK::LED_RED | PINMASK::LED_GREEN;
		asm("BREAK");
		while(true);
	}
}

namespace utility {
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
	InstanceList<T, count_t> ** utility::InstanceList<T, count_t>::il_instances = nullptr;

	template<typename T, typename count_t /*= uint8_t*/>
	count_t utility::InstanceList<T, count_t>::il_count = 0;

	template<typename T, typename count_t>
	utility::InstanceList<T, count_t>::InstanceList()
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
	utility::InstanceList<T, count_t>::~InstanceList()
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
}
