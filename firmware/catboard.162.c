/*
* Project: CatBoard (http://ibnteo.klava.org/tag/catboard)
* Version: 2.0
* Date: 2013-05-22
* Author: Vladimir Romanovich <ibnteo@gmail.com>
* License: GPL2
* 
* Based on: http://geekhack.org/index.php?topic=15542.0
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

#define NULL				0
#define NA					0
#define KEY_LAYER1			0xF1
#define KEY_LAYER2			0xF2
#define KEY_MY_SHIFT		0xF3
#define KEY_TURBO_REPEAT	0xFB
#define KEY_MAC_MODE		0xFC // (+Shift)
#define KEY_ALT_TAB			0xFD
#define KEY_FN_LOCK			0xFE
#define KEY_FN				0xFF
#define FN_KEY_ID			7*5
#define KEY_MOD				0x80
#define KEY_NULL			0

#define KEY_PRESSED_FN		1
#define KEY_PRESSED_MODS	2
#define KEY_PRESSED_ALT		3
#define KEY_PRESSED_SHIFT	4
#define KEY_PRESSED_CTRL	5
#define KEY_PRESSED_PREV	6

// #include "jcuken.h"
//#include "dvorak.h"

//#include "at90usb162.h"
#include "at90usb162mu.h"

// 0 - shorcuts my layout; 1 - shorcuts qwerty layout
#define KEY_SHORTCUTS_LAYER1	1

// Nonstandart hardware layout
#define KEY_LAYOUT_ALT_SHIFT	1
#define KEY_LAYOUT_CTRL_SHIFT	2
#define KEY_LAYOUT_GUI_SPACE	3

//#define KEY_LAYOUT		0
#define KEY_LAYOUT		KEY_LAYOUT_ALT_SHIFT

// Mac mode off
uint8_t mac_mode = 0;

// 0x00-0x7F - normal keys
// 0x80-0xF0 - mod_keys | 0x80
// 0xF1-0xFF - catboard keys


#if defined LAYOUT
#else
	uint8_t *layer2 = layer1;
#endif

// Start layout
uint8_t *layout = &layer1;


int8_t pressed[KEYS];
int8_t pressed_sw2 = 0;
uint8_t queue[7] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
uint8_t mod_keys = 0;
uint8_t *prev_layer = 0;

uint8_t turbo_repeat = 1;

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
void key_press_sw2(void);
void key_release_sw2(void);
uint8_t get_code(uint8_t key_id);

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
	LED_ON;

	for (uint8_t i=0; i<KEYS; i++) {
		pressed[i] = 0;
	}

	usb_init();
	//LED_OFF;
	while(!usb_configured());
	//LED_ON;
	//_delay_ms(1000);
	LED_OFF;
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
	if (! (*col_pin_sw2 & col_bit_sw2)) { // press SW2
		if (! pressed_sw2) {
			key_press_sw2();
		}
	} else if (pressed_sw2) { // release SW2
		key_release_sw2();
	}
	repeat_tick();
	_delay_ms(5);
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

void key_press_sw2(void) {
	pressed_sw2 = 1;
	usb_keyboard_press(KEY_C, KEY_SHIFT);
	usb_keyboard_press(KEY_A, 0);
	usb_keyboard_press(KEY_T, 0);
	usb_keyboard_press(KEY_B, KEY_SHIFT);
	usb_keyboard_press(KEY_O, 0);
	usb_keyboard_press(KEY_A, 0);
	usb_keyboard_press(KEY_R, 0);
	usb_keyboard_press(KEY_D, 0);
}
void key_release_sw2(void) {
	pressed_sw2 = 0;
	for (uint8_t i=0; i<8; i++) {
		usb_keyboard_press(KEY_BACKSPACE, 0);
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
	
	if (key_code>0xF0) { // Catboard keys
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
			if (prev_layer) { // FnLock Off
				layout = prev_layer;
				prev_layer = 0;
			} else { // FnLock On
				prev_layer = layout;
				layout = layer_fnlock;
			}
			if (prev_layer || mac_mode) {
				LED_ON;
			} else {
				LED_OFF;
			}
		} else if (key_code==KEY_MAC_MODE) { // Mac mode
			if (pressed[key_id]==KEY_PRESSED_FN && (mod_keys & (KEY_SHIFT|KEY_RIGHT_SHIFT))) {
				mac_mode = ! mac_mode;
				if (mac_mode || prev_layer) {
					LED_ON;
				} else {
					LED_OFF;
				}
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
		}
	} else if (key_code>=0x80) { // Mod keys
		if (mac_mode && key_code==(KEY_CTRL|KEY_MOD)) {
			mod_keys |= KEY_GUI;
		} else if ((mac_mode && key_code==(KEY_RIGHT_CTRL|KEY_MOD)) || key_code==(KEY_RIGHT_GUI|KEY_MOD)) {
			mod_keys |= KEY_RIGHT_GUI;
		} else {
			mod_keys |= (key_code & 0x7F);
		}
		send();
	} else {
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
	if (key_code>0xF0) { // Catboard keys release
		if (key_code==KEY_ALT_TAB && pressed_key_id!=KEY_PRESSED_ALT) { // AltTab: Alt release
			mod_keys &= ~(KEY_ALT);
			send();
		} else if (key_code==KEY_LAYER1 && pressed_key_id==KEY_PRESSED_CTRL) {
			mod_keys &= ~(KEY_CTRL);
			send();
		} else if (key_code==KEY_LAYER1) {
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
					LED_ON;
				}
			}
			last_key = 0xFF;
			press_time = 0;
			press_time2 = 0;
			release_time = 0;
			repeat_time = 0;
		} else if (key_code==KEY_LAYER2) {
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
					LED_OFF;
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
	} else if (key_code>=0x80) { // Mod keys release
		if (mac_mode && key_code==(KEY_CTRL|KEY_MOD)) {
			mod_keys &= ~KEY_GUI;
		} else if ((mac_mode && key_code==(KEY_RIGHT_CTRL|KEY_MOD)) || key_code==(KEY_RIGHT_GUI|KEY_MOD)) {
			mod_keys &= ~KEY_RIGHT_GUI;
		} else {
			mod_keys &= ~(key_code & 0x7F);
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
	if (KEY_LAYOUT==KEY_LAYOUT_GUI_SPACE || mac_mode) { // Press Cmd+Space
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
