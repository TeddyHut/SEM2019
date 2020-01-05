//Created: 12/04/2019 1:26:44 PM
 
 /** \file
 \brief Source file for generalhardware.h.
 \details See generalhardware.h for more information.
 \date Created 2019-04-12
 \author Teddy.Hut
 */

#include "generalhardware.h"

bool libmicavr::PortIn::get() const
{
	return pm_hwport.IN & 1 << pm_hwpin;
}

libmicavr::PortIn::PortIn(PORT_t &port, uint8_t pin, bool const onlevel /*= false*/, bool const pullup /*= true*/) : pm_hwport(port), pm_hwpin(pin)
{
	pm_hwport.DIRCLR = 1 << pm_hwpin;
	uint8_t pinctrlmask = 0;
	//Invert input if the PortIn is active low
	if(!onlevel)
		pinctrlmask = PORT_INVEN_bm;
	//Enable pullup if necessary
	if(pullup)
		pinctrlmask |= PORT_PULLUPEN_bm;
	*(&pm_hwport.PIN0CTRL + pm_hwpin) = pinctrlmask;
}

void libmicavr::PortOut::set(bool const state)
{
	if(state)
		pm_hwport.OUTSET = 1 << pm_hwpin;
	else
		pm_hwport.OUTCLR = 1 << pm_hwpin;
}

libmicavr::PortOut::PortOut(PORT_t &port, uint8_t const pin, bool const	invert) : pm_hwport(port), pm_hwpin(pin)
{
	pm_hwport.DIRSET = 1 << pm_hwpin;
	if(invert) *(&pm_hwport.PIN0CTRL + pm_hwpin) = PORT_INVEN_bm;
}

void libmicavr::PortOut::toggle()
{
	pm_hwport.OUTTGL = 1 << pm_hwpin;
}

ISR(ADC0_RESRDY_vect) {
	libmicavr::isr_adc();
}

void libmicavr::isr_adc()
{
	libmicavr::ADCManager::handle_isr();
}

libmicavr::ADCManager::Channel::Channel(ADC_MUXPOS_t const muxpos, ADC_REFSEL_t const refsel, VREF_ADC0REFSEL_t const vref, ADC_SAMPNUM_t const accumulation)
 : ch_muxpos(muxpos), ch_refsel(refsel), ch_vref(vref), ch_accumulation(accumulation) {
	ADCManager::channel_on_new(this);
 }

libmicavr::ADCManager::Channel::~Channel()
{
	ADCManager::channel_on_delete(this);
}

bool libmicavr::ADCManager::Channel::operator<(Channel const &p)
{
	return ch_refsel < p.ch_refsel || ch_vref < p.ch_vref || ch_muxpos < p.ch_muxpos || ch_accumulation < p.ch_accumulation;
}

bool libmicavr::ADCManager::Channel::operator>(Channel const &p)
{
	return ch_refsel > p.ch_refsel || ch_vref > p.ch_vref || ch_muxpos > p.ch_muxpos || ch_accumulation > p.ch_accumulation;
}

void libmicavr::ADCManager::next_cycle()
{
	if(!run_cycle) {
		run_cycle = true;
		if(channel.size() > 0) start_conversion(channel[0]);
	}
}

constexpr ADC_PRESC_t libmicavr::ADCManager::calculate_adc_prescale(float const f_adc, float const f_cpu)
{
	for(uint8_t i = 0; i < 8; i++) {
		if(f_cpu / (2 << i) <= f_adc) return static_cast<ADC_PRESC_t>(i);
	}
	//Return prescale 256
	return static_cast<ADC_PRESC_t>(7);
}

void libmicavr::ADCManager::handle_isr()
{
	//ISR called when ADc has finished converting
	//Determine position of current channel, to make sure it hasn't been deleted
	uint8_t currentchannel_pos = 0;
	for(; currentchannel_pos < channel.size() && channel[currentchannel_pos] != currentchannel; currentchannel_pos++);
	if(currentchannel_pos == channel.size()) return;

	//Get result (will also clear interrupt flag). ADC0.CTRLB holds accumulation bits.
	currentchannel->ch_result = ADC0.RES / (1 << ADC0.CTRLB);
	//Increment total number of results (overflow allowed, but a value of 0 is not)
	if(++currentchannel->ch_samples == 0) currentchannel->ch_samples++;

	//If last channel is reached, stop running
	if(++currentchannel_pos >= channel.size()) {
		currentchannel = nullptr;
		currentchannel_pos = 0;
		run_cycle = false;
	}
	//Otherwise, start next conversion
	else start_conversion(channel[currentchannel_pos]);
}

void libmicavr::ADCManager::channel_on_new(Channel *const ptr)
{
	//If ADC is not enabled (no conversion running), start conversion
	uint8_t i = 0;
	for(; i < channel.size() && *(channel[i]) < *ptr; i++);
	channel.insert(ptr, i);
	if(!(ADC0.CTRLA & ADC_ENABLE_bm)) start_conversion(channel[i]);
}

void libmicavr::ADCManager::channel_on_delete(Channel *const ptr)
{
	channel.remove(ptr);
	//If no channels left, disable ADC
	if(channel.size() == 0) {
		ADC0.CTRLA = 0;
	}
}

void libmicavr::ADCManager::start_conversion(Channel *const ptr)
{
	constexpr ADC_PRESC_t prescale_bits = calculate_adc_prescale(1000000, F_CPU);
	//If ADC is not enabled, configure ADC
	if(!(ADC0.CTRLA & ADC_ENABLE_bm)) {
		//1MHz ADC clock (force constexpr)
		//Enable ADC result interrupt
		ADC0.INTCTRL = ADC_RESRDY_bm;
		//Set ADC initialization delay to 32 ADC clock cycles (should be 32us, >22us), and enable random sample variation
		ADC0.CTRLD = ADC_INITDLY_DLY32_gc | ADC_ASDV_bm;
	}
	//Set channel mux
	ADC0.MUXPOS = ptr->ch_muxpos;
	if(ptr->ch_refsel == ADC_REFSEL_INTREF_gc) {
		//If reference voltage is not less than 1V, enable reduced sample capacitance.
		ADC0.CTRLC = prescale_bits | ADC_REFSEL_INTREF_gc | ((ptr->ch_vref != VREF_ADC0REFSEL_0V55_gc) ? ADC_SAMPCAP_bm : 0);
		//Set internal reference voltage (only change if needed)
		if((VREF.CTRLA & 0x7) != ptr->ch_vref) {
			VREF.CTRLA = (VREF.CTRLA & ~0x7) | ptr->ch_vref;
		}
	}
	else ADC0.CTRLC = prescale_bits | ptr->ch_refsel;
	//Set channel accumulation
	ADC0.CTRLB = ptr->ch_accumulation;

	//Enable ADC, also run in standby (if not already)
	ADC0.CTRLA = ADC_RUNSTBY_bm | ADC_ENABLE_bm;	
	//Start conversions
	ADC0.COMMAND = ADC_STCONV_bm;
	currentchannel = ptr;
}

libmodule::utility::Vector<libmicavr::ADCManager::Channel *> libmicavr::ADCManager::channel;

libmicavr::ADCManager::Channel * libmicavr::ADCManager::currentchannel = nullptr;

volatile bool libmicavr::ADCManager::run_cycle = true;

uint16_t libmicavr::ADCChannel::get() const 
{
	return ch_result;
}

uint16_t libmicavr::ADCChannel::get_samplecount() const
{
	return ch_samples;
}

libmicavr::ADCChannel::ADCChannel(ADC_MUXPOS_t const muxpos, ADC_REFSEL_t const refsel, VREF_ADC0REFSEL_t const vref /*= VREF_ADC0REFSEL_2V5_gc*/, ADC_SAMPNUM_t const accumulation /*= ADC_SAMPNUM_ACC2_gc*/)
 : Channel(muxpos, refsel, vref, accumulation) {}

/**
 \addtogroup eeprom
 \brief EEREADY interrupt flag.
 \details Calls isr_eeprom(). See [the datasheet](megaAVR 0-series datasheet.pdf#page=79) for more information about the EEREADY flag. 
 */
ISR(NVMCTRL_EE_vect) {
	libmicavr::isr_eeprom();
}

//Detailed documentation in header so that Doxygen picks it up in the EEPManager documentation.
void libmicavr::isr_eeprom()
{
	EEPManager::write_next_page();
}

/**
 The [EEBUSY bit](megaAVR 0-series datasheet.pdf#page=77) is returned.
 \returns \c true if there is a write (or other) operation in progress. \c false otherwise.
*/
bool libmicavr::EEPManager::busy()
{
	//If there is a command in progress then busy
	return NVMCTRL.STATUS & NVMCTRL_EEBUSY_bm;
}

/**
 If the EEPROM is busy, this function will block until it is not busy.
 \n If the EEPROM is not busy, this function will setup a page write and return. Subsequent pages are written using the EEREADY interrupt.
 \n Use busy() to check for write completion.
 \todo Perhaps add an optional callback for write completion.
 \details [Buffer::read](\ref libmodule::utility::Buffer::read(void *const, size_t const, size_t const) const) is used to source the data. This means read callbacks can be generated for [Buffer](\ref libmodule::utility::Buffer).
 \warning No copy of \a buffer is made. Ensure it is kept intact for the duration of the write.
 \param [in] buffer [Buffer](\ref libmodule::utility::Buffer) object to read from.
 \param [in] eeprom_offset Offset from \c EEPROM_START to write data (in bytes).
 \sa write_next_page()
*/
void libmicavr::EEPManager::write_buffer(libmodule::utility::Buffer const &buffer, uint8_t const eeprom_offset)
{
	//Not great, but block until the EEPROM is not busy (this should wait until an ongoing write_buffer has finished)
	while(NVMCTRL.STATUS & NVMCTRL_EEBUSY_bm);
	write_eeprom_position = eeprom_offset;
	write_buffer_position = 0;
	write_buffer_ptr = &buffer;
	write_next_page();
}

/**
 This function is non-blocking and will return as soon as the bytes are read (bytes are read just like normal memory).
 \n [Buffer::write](\ref libmodule::utility::Buffer::write(void const *const, size_t const, size_t const)) is used to transfer data from EEPROM to \a buffer. This means write callbacks can be generated for [Buffer](\ref libmodule::utility::Buffer).
 \param [out] buffer [Buffer](\ref libmodule::utility::Buffer) object to write to.
 \param [in] eeprom_offset Offset from \c EEPROM_START to read data (in bytes).
 \param [in] len Number of bytes to read.
*/
void libmicavr::EEPManager::read_buffer(libmodule::utility::Buffer &buffer, uint8_t const eeprom_offset, uint8_t const len)
{
	buffer.write(reinterpret_cast<void *>(EEPROM_START + eeprom_offset), len, 0);
}

/**
 Data to write is determined using #write_eeprom_position and #write_buffer_position. Both members will have the number of bytes written added to them.
 \n The number of bytes to write is the smaller of:
	-# The remaining bytes in the transfer.
	-# The remaining bytes in the page.
 
 If there are no more bytes to write, this function will return without initiating another page write. Otherwise, another page write is started and the function returns.
 \sa write_buffer(), isr_eeprom()
*/
void libmicavr::EEPManager::write_next_page()
{
	//Disable EEPROM interrupt (should be disabled unless command is in progress)
	NVMCTRL.INTCTRL = 0;
	//Determine number of bytes remaining in the page after the offset
	uint8_t page_bytes_remaining = EEPROM_PAGE_SIZE - (write_eeprom_position % EEPROM_PAGE_SIZE);
	//Determine number of bytes to write
	uint8_t write_len = libmodule::utility::tmin<uint8_t>(page_bytes_remaining, write_buffer_ptr->pm_len - write_buffer_position);
	//If no more bytes to write, return without starting another write request
	if(write_len == 0) {
		write_buffer_ptr = nullptr;
		return;
	}
	//Read data to be written into page buffer
	write_buffer_ptr->read(reinterpret_cast<void *>(EEPROM_START + write_eeprom_position), write_len, write_buffer_position);
	write_eeprom_position += write_len;
	write_buffer_position += write_len;
	//Make an erase-write operation for the current page (for EEPROM will only erase bytes that were written in the page buffer)
	CCP = CCP_SPM_gc;
	NVMCTRL.CTRLA = NVMCTRL_CMD_PAGEERASEWRITE_gc;
	//Enable the EEPROM ready interrupt
	NVMCTRL.INTCTRL = NVMCTRL_EEREADY_bm;

	//Note to self: Potential for a glitch here if the system goes to any sleep mode other than idle. The current write will finish, but further writes may not.
	//Actually there might not be any problem. It'll just delay until it wakes up again.
}

uint8_t libmicavr::EEPManager::write_eeprom_position = 0;
uint8_t libmicavr::EEPManager::write_buffer_position = 0;
libmodule::utility::Buffer const *libmicavr::EEPManager::write_buffer_ptr = nullptr;
