/*
 * libmodule.h
 *
 * Created: 17/01/2019 4:59:30 AM
 *  Author: teddy
 */ 

#pragma once

#include "libmodule/metadata.h"
#include "libmodule/utility.h"
#include "libmodule/userio.h"
#include "libmodule/timer.h"
#include "libmodule/74hc595.h"
#include "libmodule/mux.h"
#include "libmodule/ltd_2601g_11.h"
#include "libmodule/twislave.h"
#include "libmodule/module.h"
#include "libmodule/ui.h"

namespace libmodule {
	//Aliases to make things less intimidating

	//utility
	using utility::InStates;
	using DigitalInputStates = utility::InStates<bool>;

	//module
	using MotorMode = module::MotorController::MotorMode;
	using MotorOvercurrentState = module::MotorController::OvercurrentState;
	using MotorMoverMode = module::MotorMover::Mode;
	using ClientMode = module::Client::Mode;

	//timer
	//using Timer1k;
	//using Stopwatch1k;
	
	//userio
	using BlinkPattern = userio::Blinker::Pattern;
	using BlinkMode = userio::Blinker::Mode;
	using userio::BlinkerTimer1k;
	using userio::BlinkerTimer1k;
}
