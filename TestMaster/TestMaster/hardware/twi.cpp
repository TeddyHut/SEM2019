/*
 * twi.cpp
 *
 * Created: 30/11/2018 2:33:25 PM
 *  Author: teddy
 */ 

bool hw::operator bool(TWIMaster::Result const result)
{
	return result != TWIMaster::Result::Wait;
}
