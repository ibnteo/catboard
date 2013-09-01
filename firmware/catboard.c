/*
* Project: CatBoard ][
* Version: 3.1 beta
* Date: 2013-09-02
* Author: Vladimir Romanovich <ibnteo@gmail.com>
* License: GPL2
* Blog: http://ibnteo.klava.org/tag/catboard
* Site: http://catboard.klava.org/
* 
* Board: AVR-USB162MU (http://microsin.net/programming/AVR/avr-usb162mu.html) analogue Teensy 1.0
*/

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <util/delay.h>
#include "usb_keyboard.h"

#define STR_MANUFACTURER	L"ibnTeo"
#define STR_PRODUCT		L"CatBoard"

#define _PINC		(uint8_t *const)&PINC
#define _PORTC		(uint8_t *const)&PORTC
#define _PIND		(uint8_t *const)&PIND
#define _PORTD		(uint8_t *const)&PORTD
#define _PORTB		(uint8_t *const)&PORTB
#define _PINB		(uint8_t *const)&PINB

#define ROWS	5
#define COLS	12
#define KEYS	COLS*ROWS

//1=num lock, 2=caps lock, 4=scroll lock, 8=compose, 16=kana
#define LED_NUM_LOCK		1
#define LED_CAPS_LOCK		2
#define LED_SCROLL_LOCK	4
#define LED_COMPOSE		8
#define LED_KANA			16

#define NULL				0
#define NA					0
// 109-127 - catboard
#define KEY_CB_START		109
#define KEY_LAYER1			109
#define KEY_LAYER2			110
#define KEY_MACRO			111
// ...
#define KEY_MY_SHIFT		119
#define KEY_TYPO_MODE		120
#define KEY_LED				121 // (+Shift)
#define KEY_LOCK			122
#define KEY_TURBO_REPEAT	123
#define KEY_OS_MODE			124 // (+Shift)
#define KEY_ALT_TAB			125
#define KEY_FN_LOCK			126 // (+Shift)
#define KEY_FN				127
#define KEY_NULL			0

#define KEY_LCTRL	101
#define KEY_LSHIFT	102
#define KEY_LALT	103
#define KEY_LGUI	104
#define KEY_RCTRL	105
#define KEY_RSHIFT	106
#define KEY_RALT	107
#define KEY_RGUI	108
/*#define KEY_LCTRL	0x01+100
#define KEY_LSHIFT	0x02+100
#define KEY_LALT	0x04+100
#define KEY_LGUI	0x08+100
#define KEY_RCTRL	0x10+100
#define KEY_RSHIFT	0x20+100
#define KEY_RALT	0x40+100
#define KEY_RGUI	0x80+100*/


#define KEY_PRESSED_FN		1
#define KEY_PRESSED_MODS	2
#define KEY_PRESSED_ALT		3
#define KEY_PRESSED_SHIFT	4
#define KEY_PRESSED_CTRL	5
#define KEY_PRESSED_PREV	6

//#include "qwerty.h"
//#include "dvorak.h"
#include "jcuken.h"

//#include "at90usb162.h"
#include "at90usb162mu.h"

#include "my_macros.h"

// 0 - shorcuts my layout; 1 - shorcuts qwerty layout
#define KEY_SHORTCUTS_LAYER1	1

// Nonstandart hardware layout
#define KEY_LAYOUT_ALT_SHIFT	1
#define KEY_LAYOUT_CTRL_SHIFT	2
#define KEY_LAYOUT_GUI_SPACE	3

//#define KEY_LAYOUT		0
#define KEY_LAYOUT		KEY_LAYOUT_ALT_SHIFT

// OS mode: 0 - Windows, 1 - Linux, 2 - Mac
uint8_t os_mode = 1;

uint8_t caps_lock_led = 0;

// 0x00-0x7F - normal keys
// 0x80-0xF0 - mod_keys | 0x80
// 0xF1-0xFF - catboard keys

// TODO: think...
// 4-99 - Normal keys (96 keys)
// 101-108|KEY_MOD - Mod keys (8 keys)
// 109-127|KEY_MOD - CatBoard keys (19 keys)
// n|0x80 - Shift inverse keys


// Start layout
uint8_t *layout = &layer2;

int8_t pressed[KEYS];
uint8_t queue[7] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
uint8_t mod_keys = 0;
uint8_t *prev_layer = 0;

uint8_t turbo_repeat = 1;
uint8_t locked = 0;
uint8_t led = 1; // LED light

uint8_t last_key = 0xFF;
uint16_t press_time = 0;
uint16_t press_time2 = 0;
uint16_t release_time = 0;
uint16_t repeat_time = 0;

void init(void);
void send(void);
void poll(void);
void repeat_tick(void);
void key_press(uint8_t key_id);
void key_release(uint8_t key_id);
uint8_t get_code(uint8_t key_id);

/*
rus
§!@#$%^&*()–»
јџќ®†њѓѕў‘“ъ
ƒыћ÷©}°љ∆…эё
]ђ≈≠µи™~≤≥“

rus shift
±|"£€∞¬¶√'`—«
ЈЏЌ®†ЊЃЅЎ’”Ъ
ƒЫЋ÷©{•Љ∆…ЭЁ
[Ђ≈≠µИ™~<>„

lat
§¡™£¢∞§¶•ªº–≠
œ∑´®†¥¨ˆøπ“‘
åß∂ƒ©˙∆˚¬…æ«
`Ω≈ç√∫˜µ≤≥÷

lat shift
±⁄€‹›ﬁﬂ‡°·‚—±
Œ„´‰ˇÁ¨ˆØ∏”’
ÅÍÎÏ˝ÓÔÒÚÆ»
`¸˛Ç◊ı˜Â¯˘¿
*/


int main(void) {
	// Disable watchdog if enabled by bootloader/fuses
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	// Disable clock division
	clock_prescale_set(clock_div_1);

	init();
	for (;;) {
		poll();
	}
}

void init(void) {
	// Set for 16 MHz clock
	CLKPR = 0x80; CLKPR = 0;

	init_ports();

	LED_CONFIG;
	LED_RED_CONFIG;
	LED_BLUE_CONFIG;

	LED_RED_OFF;
	LED_BLUE_OFF;
	if (led) LED_ON;

	for (uint8_t i=0; i<KEYS; i++) {
		pressed[i] = 0;
	}

	usb_init();
	while(!usb_configured());
	LED_OFF;
	if (led) LED_RED_ON;
	caps_lock_led = keyboard_leds;
}

void poll() {
	uint8_t row, col, key_id;
	for (row=0; row<ROWS; row++) { // scan rows
		*row_port[row] &= ~row_bit[row];
		_delay_us(1);
		for (col=0; col<COLS; col++) { // read columns
			key_id = col*ROWS+row;
			if (! (*col_pin[col] & col_bit[col])) { // press key
				if (! pressed[key_id]) {
					key_press(key_id);
				}
			} else if (pressed[key_id]) { // release key
				key_release(key_id);
			}
		}
		*row_port[row] |= row_bit[row];
	}
	if (caps_lock_led != (keyboard_leds & LED_CAPS_LOCK)) { // change layout
		caps_lock_change_layer();
	}
	//if (keyboard_leds) LED_ON; else LED_OFF;
	repeat_tick();
	_delay_ms(5);
}

void caps_lock_change_layer() {
	caps_lock_led = (keyboard_leds & LED_CAPS_LOCK);
	if ((caps_lock_led) && (layout != layer1)) {
		if (layout==layer_fn) {
			prev_layer = layer1;
		} else {
			layout = layer1;
		}
		LED_RED_OFF;
		if (led) LED_BLUE_ON;
	} else if ((! caps_lock_led) && (layout != layer2)) {
		if (layout==layer_fn) {
			prev_layer = layer2;
		} else {
			layout = layer2;
		}
		LED_BLUE_OFF;
		if (led) LED_RED_ON;
	}
}

void repeat_tick(void) {
	if (repeat_time) { // repeat pause
		if (repeat_time<(release_time>>2)) {
			repeat_time++;
		} else { // repeat press
			repeat_time = 1;
			if (turbo_repeat) {
				keyboard_modifier_keys = mod_keys;
				keyboard_keys[0] = get_code(last_key);
				if (! usb_keyboard_send()) { // repeat release
					keyboard_keys[0] = 0;
					usb_keyboard_send();
				}
			}
		}
	} else if (press_time2) { // press2 pause
		if (press_time2<(press_time+(pressed[FN_KEY_ID] ? 5 : 30))) {
			press_time2++;
		} else {
			repeat_time = 1;
		}
	} else if (release_time) { // release pause
		if (release_time<(press_time+50)) {
			release_time++;
		} else {
			last_key = 0xFF;
			release_time = 0;
			press_time = 0;
			press_time2 = 0;
			release_time = 0;
		}
	} else if (press_time) { // press1 pause
		if (press_time<250) {
			press_time++;
		} else {
			press_time = 0;
		}
	}	
}

void key_press(uint8_t key_id) {
	uint8_t i;
	uint8_t mods_pressed = (mod_keys & (KEY_CTRL|KEY_RIGHT_CTRL|KEY_ALT|KEY_RIGHT_ALT|KEY_GUI|KEY_RIGHT_GUI));
	pressed[key_id] = (pressed[FN_KEY_ID] ? KEY_PRESSED_FN : (mods_pressed ? KEY_PRESSED_MODS : -1));
	
	uint8_t key_code = ((pressed[key_id]==KEY_PRESSED_FN) ? layer_fn[key_id] : layout[key_id]);
	if (key_code==NULL) {
		key_code = layout[key_id];
		pressed[key_id] = (mods_pressed ? KEY_PRESSED_MODS : -1);
		if (key_code==NULL && prev_layer && ! mod_keys) {
			key_code = prev_layer[key_id];
			pressed[key_id] = KEY_PRESSED_PREV;
		}
	}

	if (locked && key_code!=KEY_LOCK) return;
	
	if (key_code>=KEY_CB_START) { // Catboard keys
		if (key_code==KEY_ALT_TAB) { // AltTab press
			if (pressed[key_id]==KEY_PRESSED_FN) { // Fn + AltTab
				usb_keyboard_press(KEY_TAB, KEY_ALT);
			} else { // Alt press, Tab press and release
				if (! mod_keys) {
					mod_keys |= (KEY_ALT);
				} else {
					pressed[key_id] = KEY_PRESSED_ALT;
				}
				keyboard_modifier_keys = mod_keys;
				keyboard_keys[0] = KEY_TAB;
				usb_keyboard_send();
				_delay_ms(50);
				send();
			}
		} else if (key_code==KEY_FN_LOCK) { // FnLock
			if (pressed[key_id]==KEY_PRESSED_FN && (mod_keys & (KEY_SHIFT|KEY_RIGHT_SHIFT))) {
				if (prev_layer) { // FnLock Off
					layout = prev_layer;
					prev_layer = 0;
				} else { // FnLock On
					prev_layer = layout;
					layout = layer_fnlock;
				}
			}
		} else if (key_code==KEY_OS_MODE) { // Mac mode
			if (pressed[key_id]==KEY_PRESSED_FN && (mod_keys & (KEY_SHIFT|KEY_RIGHT_SHIFT))) {
				os_mode++;
				if (os_mode>2) os_mode = 0;
			} else { // Press Space
				usb_keyboard_press(KEY_SPACE, mod_keys);
			}
		} else if (key_code==KEY_LAYER1) { // KEY_LAYOUT1
			if (mod_keys & (KEY_SHIFT|KEY_RIGHT_SHIFT)) {
				pressed[key_id] = KEY_PRESSED_CTRL;
				mod_keys |= KEY_CTRL;
				send();
			} else {
				if (mod_keys) pressed[key_id] = KEY_PRESSED_SHIFT;
				mod_keys |= KEY_SHIFT;
				send();
			}
		} else if (key_code==KEY_LAYER2) { // KEY_LAYOUT2
			mod_keys |= KEY_RIGHT_SHIFT;
			send();
		} else if (key_code==KEY_TURBO_REPEAT) { // TURBO_REPEAT ON/OFF
			turbo_repeat = ! turbo_repeat;
		} else if (key_code==KEY_MY_SHIFT) { // My Shift
			mod_keys |= KEY_SHIFT;
			send();
		} else if (key_code==KEY_MACRO) { // TODO: My Macro
			/*uint8_t macros_pos_index = 0;
			for (i==0; i<sizeof(macros_pos); i++) {
				if (key_id==macros_pos[i]) {
					macros_pos_index = i;
					break;
				}
			}
			uint16_t *macros;
			uint16_t *macros = macros_press[macros_pos_index];
			for (i==0; i<sizeof(macros); i=i+2) {
				usb_keyboard_press(macros[i], macros[i+1]);
			}*/
		} else if (key_code==KEY_LOCK) { // Lock/Unlock keyboard
			if (locked) {
				locked = 0;
				if (led) {
					if (layout==layer1 || prev_layer==layer1) LED_BLUE_ON;
					if (layout==layer2 || prev_layer==layer2) LED_RED_ON;
				}
			} else {
				locked = 1;
				LED_OFF;
				LED_RED_OFF;
				LED_BLUE_OFF;
				usb_keyboard_press(KEY_L, KEY_GUI); // Block computer
			}
		} else if (key_code==KEY_LED && (mod_keys & (KEY_SHIFT|KEY_RIGHT_SHIFT))) { // LED On/Off
			if (led) {
				led = 0;
				LED_OFF;
				LED_RED_OFF;
				LED_BLUE_OFF;
			} else {
				led = 1;
				if (layout==layer1 || prev_layer==layer1) LED_BLUE_ON;
				if (layout==layer2 || prev_layer==layer2) LED_RED_ON;
			}
		}
	} else if (key_code>=KEY_LCTRL) { // Mod keys
		if (os_mode==2 && key_code==KEY_LCTRL) {
			mod_keys |= KEY_GUI;
		} else if ((os_mode==2 && key_code==KEY_RCTRL) || key_code==KEY_RGUI) {
			mod_keys |= KEY_RIGHT_GUI;
		} else if (key_code==KEY_LCTRL) {
			mod_keys |= KEY_CTRL;
		} else if (key_code==KEY_LSHIFT) {
			mod_keys |= KEY_SHIFT;
		} else if (key_code==KEY_LALT) {
			mod_keys |= KEY_ALT;
		} else if (key_code==KEY_LGUI) {
			mod_keys |= KEY_GUI;
		} else if (key_code==KEY_RCTRL) {
			mod_keys |= KEY_RIGHT_CTRL;
		} else if (key_code==KEY_RSHIFT) {
			mod_keys |= KEY_SHIFT;
		} else if (key_code==KEY_RALT) {
			mod_keys |= KEY_RIGHT_ALT;
		} else if (key_code==KEY_RGUI) {
			mod_keys |= KEY_RIGHT_GUI;
		}
		send();
	} else {
		/*if (mod_keys & (KEY_ALT|KEY_RIGHT_ALT)) { // TODO: typo
			uint16_t num = layer2_typo[0];
			char str[4];
			itoa(num, &str, 16);
		}*/
		if (! (last_key==key_id && release_time<10)) { // debounce
			for (i=5; i>0; i--) queue[i] = queue[i-1];
			queue[0] = key_id;
			send();
		}
	}
	// Autorepeat
	if (last_key==key_id) { // calc press2
		press_time2 = 1;
		repeat_time = 0;
	} else { // calc press1
		last_key = key_id;
		press_time = 1;
		press_time2 = 0;
		release_time = 0;
		repeat_time = 0;
	}
}

void key_release(uint8_t key_id) {
	uint8_t i;
	int8_t pressed_key_id = pressed[key_id];
	uint8_t key_code = ((pressed_key_id==KEY_PRESSED_FN) ? layer_fn[key_id] : layout[key_id]);
	if (pressed_key_id==KEY_PRESSED_PREV && prev_layer) {
		key_code = prev_layer[key_id];
	}
	pressed[key_id] = 0;
	if (locked) return;
	if (key_code>=KEY_CB_START) { // Catboard keys release
		if (key_code==KEY_ALT_TAB && pressed_key_id!=KEY_PRESSED_ALT) { // AltTab: Alt release
			mod_keys &= ~(KEY_ALT);
			send();
		} else if (key_code==KEY_LAYER1 && pressed_key_id==KEY_PRESSED_CTRL) { // Layer1: Ctrl release
			mod_keys &= ~(KEY_CTRL);
			send();
		} else if (key_code==KEY_LAYER1) { // LAYER1
			mod_keys &= ~(KEY_SHIFT);
			send();
			if (last_key==key_id && press_time && press_time<50 && pressed_key_id!=KEY_PRESSED_SHIFT) {
				if (layout!=layer1) {
					if (layout==layer_fn) {
						prev_layer = layer1;
					} else {
						layout = layer1;
					}
					change_layout();
					//LED_ON;
					LED_RED_OFF;
					if (led) LED_BLUE_ON;
				}
			}
			last_key = 0xFF;
			press_time = 0;
			press_time2 = 0;
			release_time = 0;
			repeat_time = 0;
		} else if (key_code==KEY_LAYER2) { // LAYER2
			mod_keys &= ~(KEY_RIGHT_SHIFT);
			send();
			if (last_key==key_id && press_time && press_time<50 && pressed_key_id!=KEY_PRESSED_SHIFT) {
				if (layout!=layer2) {
					if (layout==layer_fn) {
						prev_layer = layer2;
					} else {
						layout = layer2;
					}
					change_layout();
					//LED_OFF;
					LED_BLUE_OFF;
					if (led) LED_RED_ON;
				}
			}
			last_key = 0xFF;
			press_time = 0;
			press_time2 = 0;
			release_time = 0;
			repeat_time = 0;
		} else if (key_code==KEY_MY_SHIFT) { // My Shift
			mod_keys &= ~KEY_SHIFT;
			send();
			if (last_key==key_id && press_time && press_time<50 && pressed_key_id!=KEY_PRESSED_MODS && ! mod_keys) {
				usb_keyboard_press(KEY_SPACE, mod_keys);
			}
			last_key = 0xFF;
			press_time = 0;
			press_time2 = 0;
			release_time = 0;
			repeat_time = 0;
		}
	} else if (key_code>=KEY_LCTRL) { // Mod keys release
		if (os_mode==2 && key_code==KEY_LCTRL) {
			mod_keys &= ~KEY_LGUI;
		} else if ((os_mode==2 && key_code==KEY_RCTRL) || key_code==KEY_RGUI) {
			mod_keys &= ~KEY_RGUI;
		} else if (key_code==KEY_LCTRL) {
			mod_keys &= ~KEY_CTRL;
		} else if (key_code==KEY_LSHIFT) {
			mod_keys &= ~KEY_SHIFT;
		} else if (key_code==KEY_LALT) {
			mod_keys &= ~KEY_ALT;
		} else if (key_code==KEY_LGUI) {
			mod_keys &= ~KEY_GUI;
		} else if (key_code==KEY_RCTRL) {
			mod_keys &= ~KEY_RIGHT_CTRL;
		} else if (key_code==KEY_RSHIFT) {
			mod_keys &= ~KEY_SHIFT;
		} else if (key_code==KEY_RALT) {
			mod_keys &= ~KEY_RIGHT_ALT;
		} else if (key_code==KEY_RGUI) {
			mod_keys &= ~KEY_RIGHT_GUI;
		}
		send();
	} else {
		for (i=0; i<6; i++) {
			if (queue[i]==key_id) {
				break;
			}
		}
		for (; i<6; i++) {
			queue[i] = queue[i+1];
		}
		send();
		// Autorepeat
		if (last_key==key_id) { // realise time
			press_time2 = 0;
			release_time = 1;
			repeat_time = 0;
		} else { // reset
			press_time = 0;
			press_time2 = 0;
			release_time = 0;
			repeat_time = 0;
		}
	}
}

void change_layout(void) {
	if (KEY_LAYOUT==KEY_LAYOUT_GUI_SPACE || os_mode==2) { // Press Cmd+Space
		keyboard_modifier_keys = KEY_GUI;
		keyboard_keys[0] = 0;
		usb_keyboard_send();
		_delay_ms(50);
		usb_keyboard_press(KEY_SPACE, KEY_GUI);
	} else if (KEY_LAYOUT==KEY_LAYOUT_ALT_SHIFT) { // Press Alt+Shift
		keyboard_modifier_keys = KEY_ALT;
		keyboard_keys[0] = 0;
		usb_keyboard_send();
		_delay_ms(50);
		usb_keyboard_press(0, KEY_ALT|KEY_SHIFT);
	} else if (KEY_LAYOUT==KEY_LAYOUT_CTRL_SHIFT) { // Press Ctrl+Shift
		keyboard_modifier_keys = KEY_CTRL;
		keyboard_keys[0] = 0;
		usb_keyboard_send();
		_delay_ms(50);
		usb_keyboard_press(0, KEY_CTRL|KEY_SHIFT);
	}
}

void send(void) {
	uint8_t i;
	for (i=0; i<6; i++) {
		keyboard_keys[i] = get_code(queue[i]);
	}
	keyboard_modifier_keys = mod_keys;
	usb_keyboard_send();
}

uint8_t get_code(uint8_t key_id) {
	uint8_t key_code = 0;
	if (key_id<KEYS) { // not 0xFF
		if (pressed[key_id]==KEY_PRESSED_FN) { // key+Fn key
			if (layer_fn[key_id]>0 && layer_fn[key_id]<0x80) {
				key_code = layer_fn[key_id];
			}
		} else if (layout!=layer_fn && pressed[key_id]==KEY_PRESSED_MODS) { // keyboard shortcuts from layer1
			key_code = (KEY_SHORTCUTS_LAYER1 ? layer1[key_id] : layer2[key_id]);
		} else {
			key_code = layout[key_id];
		}
	}
	return key_code;
}
