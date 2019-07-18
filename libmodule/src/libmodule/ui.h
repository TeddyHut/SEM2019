/*
 * ui.h
 *
 * Created: 14/04/2019 5:47:13 PM
 *  Author: teddy
 */ 

#pragma once

/* Brainstorming...
Need UI objects
- List -> selecting an item in the list takes to a new UI screen
- Screen -> Has a parent screen and can spawn child screens
	- Context may be a better name... maybe not
- Number input with
	- Dynamic step change depending on decimal points
	- Changeable rapid fire levels
	- Ability to know when reached top/bottom
	- Option to wrap
Separate display and logic -> have input intergrated into logic via "Dpad"
Not bother with tick/timer templates for time-sake (maybe later though)
*/


//on child complete
//ui update
//managmenet update
//template <common>
//spawn

#ifdef LIBMODULE_INCLUDE_UI

#include "userio.h"
#include "ltd_2601g_11.h"

namespace libmodule {
	namespace ui {
		struct Dpad {
			userio::RapidInput2L1k up;
			userio::RapidInput2L1k down;
			userio::RapidInput2L1k left;
			userio::RapidInput2L1k right;
			userio::RapidInput2L1k centre;
			void set_rapidInputLevel(uint8_t const index, userio::RapidInput2L1k::Level const value);
		};

		template <typename common_t>
		class Screen {
		public:
			Screen(common_t *const ui_common = nullptr);
			//Consider keeping track of parent to notify if destructed
			//Will destruct the child if it has one
			virtual ~Screen();
		protected:
		public:
			void ui_management_update();
		protected:
			//---Parent functionality---
			//Spawn a new child object. Memory deallocation will be handled automatically.
			void ui_spawn(Screen *const child);
			//Main UI functionality. Called when there is no child object.
			virtual void ui_update();
			//Called before child is deleted.
			virtual void ui_on_childComplete();
			Screen *ui_child = nullptr;

			//---Child functionality---
			void ui_finish();
		public:
			common_t *ui_common;
		protected:
			bool ui_finished = false;
		};

		namespace segdpad {
			struct Common {
				userio::IC_LTD_2601G_11 &segs;
				Dpad dpad;
				userio::BlinkerTimer1k dp_right_blinker{segs.get_output_dp_right()};
			};

			namespace pattern {
				extern userio::Blinker::Pattern rubberband;
				extern userio::Blinker::Pattern edit;
			}

			/* List of items. When clicked, items can spawn a child.
			 * wrap: if true, the list will wrap around.
			 * Navigation
			 *	up: move up item
			 *	down: move down item
			 *	left: ui_finish
			 *	right/centre: select (click) item
			 */
			class List : public Screen<Common> {
			public:
				struct Item {
					using Screen_t = Screen<Common>;
					char name[4];
					//Called when the list item is clicked. If nullptr is not returned, the list will spawn the returned screen.
					virtual Screen *on_click() = 0;
					//Called when the screen returned in on_click finishes, if any was returned.
					virtual void on_finish(Screen *const screen);
					//Called when the screen is highlighted/is the current item. Will be called every cycle.
					//firstcycle will be true if the item has just come into view.
					virtual void on_highlight(bool const firstcycle);
				};
				//Can be used to call member function of an object of type T when Item events happen.
				template <typename T>
				struct Item_MemFnCallback : public Item {
					Screen *on_click() override;
					void on_finish(Screen *const screen) override;
					
					using callback_click_t = Screen *(T::*)();
					using callback_finish_t = void (T::*)(Screen *const);
					
					T *callback_ptr;
					callback_click_t callback_click;
					callback_finish_t callback_finish;
					Item_MemFnCallback(T *const callback_ptr, callback_click_t const callback_click, callback_finish_t const callback_finish = nullptr);
				};
				
				utility::Vector<Item *> m_items;
				List(bool const wrap = true);
			protected:
				void ui_update() override;
				void ui_on_childComplete() override;

				uint8_t pm_currentitem = 0;
				bool pm_wrap;
				bool run_init = true;
			};

			/* List of items. When clicked, items toggle on or off. This is shown on the left decimal point.
			 * wrap: if true, the list will wrap around.
			 * Navigation:
			 *	up: move up item
			 *	down: move down item
			 *	left: ui_finish
			 *	right: reset all items to default
			 *	centre: toggle item
			 */
			class ToggleList : public Screen<Common> {
			public:
				struct Item {
					char name[2];
					bool default_value : 1;
					bool value : 1;
				};

				utility::Vector<Item> m_items;
				ToggleList(bool const wrap = true);
			protected:
				void ui_update() override;
				
				uint8_t pm_currentitem = 0;
				bool pm_wrap : 1;
				bool pm_runinit : 1;
			};

			/* Number input. Upon finishing, result will be stored in m_value.
			 * config: The configuration for the number input.
			 * default_value: The value to start from when entering the number input.
			 * Navigation:
			 *	up: add step
			 *	down: subtract step
			 *	left: ui_finish, m_confirmed set to false
			 *	right: reset m_value to default
			 *	centre: ui_finish, m_confirmed set to true
			 */
			class NumberInputDecimal : public Screen<Common> {
			public:
				struct Config {
					//The minimum value
					uint16_t min = 0;
					//The maximum value
					uint16_t max = 1000;
					//The difference for a single input
					uint16_t step = 1;
					//The position of the decimal point
					uint8_t sig10 = 0;
					bool wrap : 1;
					bool dynamic_step : 1;
				};
				
				NumberInputDecimal(Config &config, uint16_t const default_value);

				uint16_t m_value;
				uint16_t m_default_value;
				bool m_confirmed : 1;
			protected:
				void ui_update() override;

				bool pm_runinit : 1;
				Config pm_config;
			private:
				template <typename T>
				static constexpr T powi(T const base, T const exp);
				template <typename T>
				static constexpr T log10i(T const p);
				template <typename T>
				static constexpr uint8_t extract_digit10i(T const p, uint8_t const exp);
			};
		}
	}
}

template <typename common_t>
libmodule::ui::Screen<common_t>::Screen(common_t *const ui_common /*= nullptr*/) : ui_common(ui_common) {}

template <typename common_t>
libmodule::ui::Screen<common_t>::~Screen()
{
	if(ui_child != nullptr) delete ui_child;
}

template <typename common_t>
void libmodule::ui::Screen<common_t>::ui_management_update()
{
	if(ui_child == nullptr) ui_update();
	else ui_child->ui_management_update();
	
	if(ui_child->ui_finished) {
		ui_on_childComplete();
		delete ui_child;
		ui_child = nullptr;
	}
}

template <typename common_t>
void libmodule::ui::Screen<common_t>::ui_spawn(Screen *const child)
{
	child->ui_common = ui_common;
	ui_child = child;
}

template <typename common_t>
void libmodule::ui::Screen<common_t>::ui_update() {}

template <typename common_t>
void libmodule::ui::Screen<common_t>::ui_on_childComplete() {}

template <typename common_t>
void libmodule::ui::Screen<common_t>::ui_finish()
{
	ui_finished = true;
}

template <typename T>
libmodule::ui::Screen<libmodule::ui::segdpad::Common> * libmodule::ui::segdpad::List::Item_MemFnCallback<T>::on_click()
{
	if(callback_ptr != nullptr && callback_click != nullptr) return (callback_ptr->*callback_click)();
	return nullptr;
}

template <typename T>
void libmodule::ui::segdpad::List::Item_MemFnCallback<T>::on_finish(Screen *const screen)
{
	if(callback_ptr != nullptr && callback_finish != nullptr) (callback_ptr->*callback_finish)(screen);
}

template <typename T>
libmodule::ui::segdpad::List::Item_MemFnCallback<T>::Item_MemFnCallback(T *const callback_ptr, callback_click_t const callback_click, callback_finish_t const callback_finish /*= nullptr*/)
: callback_ptr(callback_ptr), callback_click(callback_click), callback_finish(callback_finish) {}


template <typename T>
constexpr T libmodule::ui::segdpad::NumberInputDecimal::powi(T const base, T const exp)
{
	T res = 1;
	for(T i = 0; i < exp; i++) res *= base;
	return res;
}

template <typename T>
constexpr T libmodule::ui::segdpad::NumberInputDecimal::log10i(T const p)
{
	//Cannot take log(0)
	if(p == 0) return 0;
	T i = 0;
	for(; p / powi<T>(10, i) > 0; i++);
	return i - 1;
}

template <typename T>
constexpr uint8_t libmodule::ui::segdpad::NumberInputDecimal::extract_digit10i(T const p, uint8_t const exp)
{
	return p % powi<T>(10, exp + 1) / powi<T>(10, exp);
}

#endif
