/*
 * twi.h
 *
 * Created: 8/01/2019 9:51:17 PM
 *  Author: teddy
 */ 

#pragma once

#include <libmodule/utility.h>
#include <libmodule/twislave.h>

namespace hw {
	//Declare for friend statement
	void isr_twi_slave0();

	class TWISlave0 : public libmodule::twi::TWISlave {
		friend void isr_twi_slave0();
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

		//For these pos could be combined into one
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
	namespace inst {
		extern TWISlave0 twiSlave0;
	}
}
