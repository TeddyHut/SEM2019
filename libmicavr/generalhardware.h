/*
 * generalhardware.h
 *
 * Created: 12/04/2019 1:25:58 PM
 *  Author: teddy
 */ 

#pragma once

#include <avr/io.h>
#include <libmodule/utility.h>
#include <libmodule/twislave.h>

namespace libmicavr {
//--- Port functionality ---
	class PortOut : public libmodule::utility::Output<bool> {
	public:
		void set(bool const p) override;
		void toggle() override;
		PortOut(PORT_t &port, uint8_t const pin, bool const invert = false);
	private:
		PORT_t &pm_hwport;
		uint8_t const pm_hwpin;
	};
	class PortIn : public libmodule::utility::Input<bool> {
	public:
		bool get() const override;
		PortIn(PORT_t &port, uint8_t const pin, bool const onlevel = false, bool const pullup = true);
	private:	
		PORT_t &pm_hwport;
		uint8_t const pm_hwpin;
	};

//--- ADC functionality ---
	void isr_adc();
	
	class ADCManager {
	public:
		class Channel {
			friend ADCManager;

			ADC_MUXPOS_t ch_muxpos;
			ADC_REFSEL_t ch_refsel;
			VREF_ADC0REFSEL_t ch_vref;
			ADC_SAMPNUM_t ch_accumulation;

		protected:
			Channel(ADC_MUXPOS_t const muxpos, ADC_REFSEL_t const refsel, VREF_ADC0REFSEL_t const vref, ADC_SAMPNUM_t const accumulation);
			virtual ~Channel();

			uint16_t ch_result = 0;
			uint16_t ch_samples = 0;
		public:
			bool operator<(Channel const &p);
			bool operator>(Channel const &p);
		};

		//ADC manager will stop after reading all channels. Call to read all channels again.
		static void next_cycle();

	private:
		friend Channel;
		friend void isr_adc();

		static constexpr ADC_PRESC_t calculate_adc_prescale(float const f_adc, float const f_cpu);
		
		static void handle_isr();
		static void channel_on_new(Channel *const ptr);
		static void channel_on_delete(Channel *const ptr);

		static void start_conversion(Channel *const ptr);

		static libmodule::utility::Vector<Channel *> channel;
		static Channel *currentchannel;
		static volatile bool run_cycle;
	};

	class ADCChannel : public libmodule::utility::Input<uint16_t>, public ADCManager::Channel {
	public:
		uint16_t get() const override;
		uint16_t get_samplecount() const;
		ADCChannel(ADC_MUXPOS_t const muxpos, ADC_REFSEL_t const refsel, VREF_ADC0REFSEL_t const vref = VREF_ADC0REFSEL_2V5_gc, ADC_SAMPNUM_t const accumulation = ADC_SAMPNUM_ACC2_gc);
	};
//--- EEPROM functionality ---
	
	/**
	 \defgroup eeprom EEPROM
	 \brief Utilities to easily read from and write to EEPROM.

	 The megaAVR 0-series processor family has a peripheral called NVMCTRL that manages non-volatile memory. EEPROM has byte erase-write granularity, and uses a page buffer to write the contents. See [NVMCTRL in the datasheet](megaAVR 0-series datasheet.pdf#page=67) for more information.
	 \n The size of the page buffer is given by \c EEPROM_PAGE_SIZE defined in \c <avr/io.h>. For the ATmega3208, the page size of 64 bytes.
	 \n The size of the EEPROM is given by \c EEPROM_SIZE. For the ATmega3208, the EEPROM size of 256 bytes.
	 @{
	*/

	/**
	 \brief Called by EEPROM interrupt service routine.
	
	 Called by ISR(NVMCTRL_EE_vect) when the EEREADY interrupt occurs. Calls EEPManager::write_next_page().
	 \n This function only exists to be a \c friend to EEPManager::write_next_page().
	*/
	void isr_eeprom();

	/**
	 \brief Interrupt based EEPROM utilities.
	 
	 EEPManager allows easy reading and writing to EEPROM using libmodule::utility::Buffer objects for input and output. All members are static.
	 \n If no write operation is in progress, the write function is non-blocking and will return immediately. EEPROM page management is handled automatically using interrupts.
	 \n If a write operation is in progress, the write function will block until the current operation is complete.
	 \author Teddy.Hut
	*/
	class EEPManager {
		friend void isr_eeprom();
	public:
		///Returns \c true if there is a write operation in progress.
		static bool busy();

		//Writes the buffer to EEPROM. Will return before finishing, and uses a pointer to the buffer (no copy is made).
		//Ensure the buffer is kept intact.
		///Writes entire contents of \a buffer to EEPROM at position \a eeprom_offset.
		static void write_buffer(libmodule::utility::Buffer const &buffer, uint8_t const eeprom_offset);
		///Reads \a len bytes from EEPROM at position \a eeprom_offset to the start of \a buffer.
		static void read_buffer(libmodule::utility::Buffer &buffer, uint8_t const eeprom_offset, uint8_t const len);
	private:
		///Write next queued page segment to EEPROM.
		static void write_next_page();
		///Next EEPROM destination for a page write.
		static uint8_t write_eeprom_position;
		///Position to read from #write_buffer_ptr in the next page write.
		static uint8_t write_buffer_position;
		///Pointer to buffer holding the source data for an ongoing write operation.
		static libmodule::utility::Buffer const *write_buffer_ptr;
	};

	/**@}*/
}
