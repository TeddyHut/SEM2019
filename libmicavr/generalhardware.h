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
	void isr_eeprom();

	class EEPManager {
		friend void isr_eeprom();
	public:
		//Returns true if there is a write operation in progress
		static bool busy();
		//Writes the buffer to EEPROM. Will return before finishing, and uses a pointer to the buffer (no copy is made).
		//Ensure the buffer is kept intact.
		static void write_buffer(libmodule::utility::Buffer const &buffer, uint8_t const eeprom_offset);
		static void read_buffer(libmodule::utility::Buffer &buffer, uint8_t const eeprom_offset, uint8_t const len);
	private:
		static void write_next_page();
		static uint8_t write_eeprom_position;
		static uint8_t write_buffer_position;
		static libmodule::utility::Buffer const *write_buffer_ptr;
	};

//--- SPI/TWI Functionality ---
/*
	class TWISlaveSPI : public libmodule::twi::TWISlave {
	public:
		bool communicating() const override;
		bool attention() const override;
		Result result() const override;
		void reset() override;
		TransactionInfo lastTransaction() override;
		void set_callbacks(Callbacks *const callbacks) override;
		void set_address(uint8_t const addr) override;
		void set_recvBuffer(uint8_t buf[], uint8_t const len) override;
		void set_sendBuffer(uint8_t const buf[], uint8_t const len) override;

		TWISlave0();
	private:
		static TWISlave0 *instance;
		//Could make this member function volatile instead of the below vars
		void handle_isr();
		//If with pm_recvbuf and pm_sendbuf have been set, TWI0 slave mode is enabled
		void enableCheck();

		enum class State {
			Idle,
			Transaction,
		} pm_state = State::Idle;

		Callbacks *pm_callbacks = nullptr;
		//volatile because accessed from both ISR and main application
		volatile Result pm_result = Result::Wait;
		volatile TransactionInfo pm_previoustransaction;
		TransactionInfo pm_currenttransaction;

		struct {
			uint8_t *buf = nullptr;
			uint8_t len = 0;
		} volatile pm_recvbuf;
		struct {
			uint8_t const *buf = nullptr;
			uint8_t len = 0;
		} volatile pm_sendbuf;
		uint8_t pm_bufpos = 0;
	};
	};
	*/
}
