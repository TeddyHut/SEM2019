/*
 * metadata.h
 *
 * Created: 16/01/2019 11:50:56 PM
 *  Author: teddy
 */ 

#pragma once

#include <inttypes.h>
#include <stdlib.h>
/*
0x00-0x01: Header
0x02: Signature
0x03: ID
0x04-0x0B: Name
0x0C: Status
0x0D: Settings
0x0E: InstanceCount
0x0F: SampleCount
0x10-0x13: RPS
0x14-0x17: TPS
0x18: SamplePos
0x19...: SampleBuffer
*/

namespace libmodule {
	namespace module {
		namespace metadata {
			namespace com {
				constexpr size_t NameLength = 8;
				extern uint8_t Header[2];//; //Sort of "SEMA"
				namespace offset {
					enum e {
						Header = 0,
						Signature = Header + sizeof com::Header,
						ID,
						Name,
						Status = Name + NameLength,
						Settings,
						_size,
					};
				}
				namespace sig {
					namespace status {
						enum e {
							Active = 0,
							Operational = 1,
						};
					}
					namespace settings {
						enum e {
							Power = 0,
							LED = 1,
						};
					}
				}
			}
			namespace horn {
				namespace sig {
					namespace settings {
						enum e {
							HornState = 2,
						};
					}
				}
			}
			namespace speedmonitor {
				using rps_t = uint32_t;
				using cps_t = uint32_t;
				namespace offset {
					namespace manager {
						enum e {
							InstanceCount = com::offset::_size,
							SampleCount,
							_size,
						};
					}
					namespace instance {
						enum e {
							Constant_RPS = 0,
							Constant_TPS = Constant_RPS + sizeof(rps_t),
							SamplePos = Constant_TPS + sizeof(cps_t),
							SampleBuffer,
						};
					}
				}
				namespace sig {
					namespace status {
						enum e {
							SampleSize = 4,
						};
					}
				}
				namespace mask {
					namespace status {
						enum e {
							SampleSize = 0b1111 << sig::status::SampleSize,
						};
					}
				}
			}
			namespace motorcontroller {
				namespace offset {
					enum e {
						Voltage_MaxCurrent = com::offset::_size,
						PWM_MaxCurrent = Voltage_MaxCurrent + sizeof(uint16_t),
						MeasuredCurrent = PWM_MaxCurrent + sizeof(uint16_t),
						MeasuredVoltage = MeasuredCurrent + sizeof(uint16_t),
						PWMFrequency = MeasuredVoltage + sizeof(uint16_t),
						PWMDutyCycle = PWMFrequency + sizeof(uint16_t),
						ControlVoltage,
						_size,
					};
				}
				namespace sig {
					namespace status {
						enum e {
							OvercurrentState = 2,
						};
					}
					namespace settings {
						enum e {
							MotorMode = 2,
							OvercurrentReset = 4,
						};
					}
				}
				namespace mask {
					namespace settings {
						enum e {
							MotorMode = 0b11 << sig::settings::MotorMode,
						};
					}
					namespace status {
						enum e {
							OvercurrentState = 0b11 << sig::status::OvercurrentState,
						};
					}
				}
			}
			namespace motormover {
				namespace offset {
					enum e {
						Position_Engaged = com::offset::_size,
						Position_Disengaged = Position_Engaged + sizeof(uint16_t),
						ContinuousPosition = Position_Disengaged + sizeof(uint16_t),
						_size,
					};
				}
				namespace sig {
					namespace settings {
						enum e {
							Mode = 2,
							Engaged,
							Powered,
						};
					}
					namespace status {
						enum e {
							Engaged = 2,
						};
					}
				}
			}
		}
	}
}
