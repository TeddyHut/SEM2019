/*
 * metadata.h
 *
 * Created: 16/01/2019 11:50:56 PM
 *  Author: teddy
 */ 

#pragma once

#include <inttypes.h>
#include <stdlib.h>

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
		}
	}
}
