/*
 * utility.cpp
 *
 * Created: 6/12/2018 1:12:28 AM
 *  Author: teddy
 */ 

#include "utility.h"

void operator delete(void * ptr, unsigned int len)
{
	//May need to free memory here
}

void __cxa_pure_virtual()
{
	hw::panic();
}
