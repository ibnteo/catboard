/*
* Project: CatBoard (http://ibnteo.klava.org/tag/catboard)
* Version: 0.33 beta
* Date: 2013-01-12
* Author: Vladimir Romanovich <ibnteo@gmail.com>
* 
* Based on: http://geekhack.org/index.php?topic=15542.0
* 
* Board: AVR-USB162 (http://microsin.ru/content/view/685/44/)
*/

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <util/delay.h>
#include "usb_keyboard.h"

#define STR_MANUFACTURER	L"ibnTeo"
#define STR_PRODUCT		L"Catboard"

#define LED_CONFIG	(DDRD	|= (1<<4))
#define LED_OFF		(PORTD	&= ~(1<<4))
#define LED_ON		(PORTD	|= (1<<4))

#define _PINC		(uint8_t *const)&PINC
#define _PORTC		(uint8_t *const)&PORTC
#define _PIND		(uint8_t *const)&PIND
#define _PORTD		(uint8_t *const)&PORTD
#define _PORTB		(uint8_t *const)&PORTB
#define _PINB		(uint8_t *const)&PINB

#define ROWS	5
#define COLS	12
#define KEYS	COLS*ROWS

#define NULL			0
#define NA				0
#define KEY_LAYER1		0xF1
#define KEY_LAYER2		0xF2
#define KEY_MULT1		0xFB
#define KEY_MULT2		0xFC
#define KEY_ALT_TAB		0xFD
#define KEY_FN_LOCK		0xFE
#define KEY_FN			0xFF
#define FN_KEY_ID		7*5
#define KEY_MOD			0x80

#define KEY_PRESSED_FN		1
#define KEY_PRESSED_MODS	2
#define KEY_PRESSED_ALT		3

/*#define KEY_MEDIA_VOLUME_INC	0xE9
#define KEY_MEDIA_VOLUME_DEC	0xEA
#define KEY_MEDIA_MUTE			0xE2
#define KEY_MEDIA_PLAY_PAUSE	0xCD
#define KEY_MEDIA_NEXT_TRACK	0xB5
#define KEY_MEDIA_PREV_TRACK	0xB6
#define KEY_MEDIA_STOP			0xB7
#define KEY_MEDIA_EJECT			0xB8*/

// 0x00-0x7F - normal keys
// 0x80-0xF0 - mod_keys + 0x80 (TODO: not KEY_RIGHT_GUI)
// 0xF1-0xFF - catboard keys

const uint8_t layer1[KEYS] = {
	//ROW0				ROW1				ROW2			ROW3			ROW4
	KEY_ESC,			KEY_LAYER1,			KEY_BACKSPACE,	KEY_TAB,		KEY_TILDE,		// COL0
	KEY_ALT_TAB,		KEY_Z,				KEY_A,			KEY_Q,			KEY_1,			// COL1
	KEY_ENTER,			KEY_X,				KEY_S,			KEY_W,			KEY_2,			// COL2
	KEY_ALT|KEY_MOD,	KEY_C,				KEY_D,			KEY_E,			KEY_3,			// COL3
	KEY_CTRL|KEY_MOD,	KEY_V,				KEY_F,			KEY_R,			KEY_4,			// COL4
	KEY_SHIFT|KEY_MOD,	KEY_B,				KEY_G,			KEY_T,			KEY_5,			// COL5
	KEY_SPACE,			KEY_N,				KEY_H,			KEY_Y,			KEY_6,			// COL6 
	KEY_FN,				KEY_M,				KEY_J,			KEY_U,			KEY_7,			// COL7
	KEY_RIGHT_ALT|KEY_MOD,KEY_COMMA,		KEY_K,			KEY_I,			KEY_8,			// COL8
	KEY_BACKSLASH,		KEY_PERIOD,			KEY_L,			KEY_O,			KEY_9,			// COL9
	KEY_RIGHT_BRACE,	KEY_SLASH,			KEY_SEMICOLON,	KEY_P,			KEY_0,			// COL10
	KEY_EQUAL,			KEY_LAYER2,			KEY_QUOTE,		KEY_LEFT_BRACE,	KEY_MINUS		// COL11
};

// 0 - shorcuts my layout; 1 - shorcuts qwerty layout
#define KEY_SHORTCUTS_LAYER1	1

// Nonstandart hardware layout
#define KEY_LAYOUT_ALT_SHIFT	1
#define KEY_LAYOUT_CTRL_SHIFT	2
#define KEY_LAYOUT_GUI_SPACE	3
#define KEY_LAYOUT				0

#include "jcuken.h"
//#include "dvorak.h"

const uint8_t layer_fn[KEYS] = {
	//ROW0				ROW1				ROW2			ROW3			ROW4
	KEY_ESC,			NULL,				KEY_BACKSPACE,	KEY_TAB,		KEY_PRINTSCREEN,// COL0
	KEY_NUM_LOCK,		KEYPAD_0,			KEYPAD_ASTERIX,	KEYPAD_SLASH,	KEY_F1,			// COL1
	KEY_ENTER,			KEYPAD_1,			KEYPAD_4,		KEYPAD_7,		KEY_F2,			// COL2
	KEY_ALT|KEY_MOD,	KEYPAD_2,			KEYPAD_5,		KEYPAD_8,		KEY_F3,			// COL3
	KEY_CTRL|KEY_MOD,	KEYPAD_3,			KEYPAD_6,		KEYPAD_9,		KEY_F4,			// COL4
	KEY_SHIFT|KEY_MOD,	KEYPAD_PERIOD,		KEYPAD_PLUS,	KEYPAD_MINUS,	KEY_F5,			// COL5
	KEY_SPACE,			KEY_BACKSPACE,		KEY_ENTER,		KEY_ALT_TAB,	KEY_F6,			// COL6 
	KEY_FN,				KEY_DELETE,			KEY_LEFT,		KEY_HOME,		KEY_F7,			// COL7
	KEY_FN_LOCK,		KEY_INSERT,			KEY_DOWN,		KEY_UP,			KEY_F8,			// COL8
	NULL,				NULL,				KEY_RIGHT,		KEY_END,		KEY_F9,			// COL9
	NULL,				NULL,				KEY_PAGE_DOWN,	KEY_PAGE_UP,	KEY_F10,		// COL10
	KEY_F12,			NULL,				NULL,			KEY_ESC,		KEY_F11			// COL11
};

uint8_t *layout = layer2;

uint8_t *const	row_port[ROWS]	= { _PORTC,	_PORTC,	_PORTC,	_PORTC,	_PORTC};
const uint8_t	row_bit[ROWS]	= { (1<<7),	(1<<6),	(1<<5),	(1<<4),	(1<<2)};
uint8_t *const	col_pin[COLS]	= {_PIND, _PIND, _PIND, _PIND, _PIND, _PIND, _PINB,	_PINB,	_PINB, _PINB, _PINB, _PINB};
const uint8_t	col_bit[COLS]	= { (1<<6),	(1<<5),	(1<<3),	(1<<2),	(1<<1),	(1<<0),	(1<<7),	(1<<6),	(1<<5),	(1<<4),	(1<<1),	(1<<0)};

int8_t pressed[KEYS];
uint8_t queue[7] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
uint8_t mod_keys = 0;
uint8_t *prev_layer = 0;

uint8_t last_key = 0;
uint16_t press_time = 0;
uint16_t press_time2 = 0;
uint16_t release_time = 0;
uint16_t repeat_time = 0;

void init(void);
void send(void);
void poll(void);
void key_press(uint8_t key_id);
void key_release(uint8_t key_id);
uint8_t get_code(uint8_t key_id);

int main(void) {
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	init();
	for(;;) {
		poll();
	}
}

void init(void) {
	// set for 16 MHz clock
	CLKPR = 0x80; CLKPR = 0;

	DDRB	= 0x00; DDRC	= 0b11110100;	DDRD	= 0x00;
	PORTB	= 0xFF; PORTC	= 0b11110100; 	PORTD	= 0xFF;

	LED_CONFIG;
	LED_ON;

	for(uint8_t i=0; i<KEYS; i++) {
		pressed[i] = 0;
	}

	usb_init();
	LED_OFF;
	while(!usb_configured());
	LED_ON;
	_delay_ms(1000);
	LED_OFF;
}

void poll() {
	uint8_t row, col, key_id;
	for(row=0; row<ROWS; row++) {
		*row_port[row] &= ~row_bit[row];
		_delay_us(1);
		for(col=0; col<COLS; col++) {
			key_id = col*ROWS+row;
			if(!(*col_pin[col] & col_bit[col])) {
				if(!pressed[key_id]) {
					key_press(key_id);
				}
			} else if(pressed[key_id]) {
				key_release(key_id);
			}
		}
		*row_port[row] |= row_bit[row];
	}
	_delay_ms(5);
	// Autorepeat
	if (repeat_time) {
		if (repeat_time<(release_time>>2)) {
			repeat_time++;
		} else {
			repeat_time = 1;

			keyboard_modifier_keys = mod_keys;
			keyboard_keys[0] = get_code(last_key);
			if (! usb_keyboard_send()) {
				keyboard_keys[0] = 0;
				usb_keyboard_send();
			}
		}
	} else if (press_time2) {
		if (press_time2<(press_time+(pressed[FN_KEY_ID] ? 5 : 30))) {
			press_time2++;
		} else {
			repeat_time = 1;
		}
	} else if (release_time) {
		if (release_time<(press_time+50)) {
			release_time++;
		} else {
			last_key = 0;
			release_time = 0;
			press_time = 0;
			press_time2 = 0;
			release_time = 0;
		}
	} else if (press_time) {
		if (press_time<50) {
			press_time++;
		} else {
			press_time = 0;
		}
	}	
}

void key_press(uint8_t key_id) {
	uint8_t i;
	//pressed[key_id] = (pressed[FN_KEY_ID] ? 1 : -1);
	uint8_t mods_pressed = (mod_keys & (KEY_CTRL|KEY_RIGHT_CTRL|KEY_ALT|KEY_RIGHT_ALT));
	pressed[key_id] = (pressed[FN_KEY_ID] ? KEY_PRESSED_FN : (mods_pressed ? KEY_PRESSED_MODS : -1));
	if(pressed[FN_KEY_ID] && layer_fn[key_id]>=0x80) {
		if(layer_fn[key_id]>0xF0) { // Fn+ Catboard keys
			if(layer_fn[key_id]==KEY_ALT_TAB) { // Fn + ALT_TAB press
				usb_keyboard_press(KEY_TAB, KEY_ALT);
			} else if (layer_fn[key_id]==KEY_FN_LOCK) { // Fn + FnLock
				if (prev_layer) { // FnLock Off
					layout = prev_layer;
					prev_layer = 0;
					LED_OFF;
				} else { // FnLock On
					prev_layer = layout;
					layout = layer_fn;
					LED_ON;
				}
			}
		} else { // Fn+Mod_keys
			// TODO: not work KEY_RIGHT_GUI
			//mod_keys |= ((layer_fn[key_id]!=0x80) ? (layer_fn[key_id] & 0x7F) : layer_fn[key_id]);
			/*if (layer_fn[key_id]==(KEY_CTRL+KEY_MOD) && (mod_keys & KEY_CTRL)) { // Fn+KEY_CTRL press
				pressed[key_id] = 2;
				mod_keys |= KEY_SHIFT;
			}*/
			mod_keys |= (layer_fn[key_id] & 0x7F);
			send();
		}
	} else if(((! pressed[FN_KEY_ID]) || layout==layer_fn) && layout[key_id]>0xF0) { // Catboard keys
		if (layout[key_id]==KEY_LAYER1 && layout!=layer1) { // KEY_LAYOUT1
			layout = layer1;
			change_layout();
		} else if (layout[key_id]==KEY_LAYER2 && layout!=layer2) { // KEY_LAYOUT2
			layout = layer2;
			change_layout();
		} else if (layout[key_id]==KEY_ALT_TAB) { // ALT_TAB press
			if (!(mod_keys & (KEY_ALT|KEY_RIGHT_ALT))) {
				mod_keys |= (KEY_ALT);
			} else {
				pressed[key_id] = KEY_PRESSED_ALT;
			}
			keyboard_modifier_keys = mod_keys;
			keyboard_keys[0] = KEY_TAB;
			usb_keyboard_send();
			_delay_ms(50);
			send();
		/*} else if (layout[key_id]==KEY_MULT1) { // ups :(
			usb_keyboard_press(KEY_MEDIA_VOLUME_INC, mod_keys);
		} else if (layout[key_id]==KEY_MULT2) {
			usb_keyboard_press(KEY_MEDIA_VOLUME_DEC, mod_keys);*/
		} else if (layer_fn[key_id]==KEY_FN_LOCK) { // Fn + FnLock
			if (prev_layer) { // FnLock Off
				layout = prev_layer;
				prev_layer = 0;
				LED_OFF;
			} else { // FnLock On
				prev_layer = layout;
				layout = layer_fn;
				LED_ON;
			}
		}
	} else if(((! pressed[FN_KEY_ID]) || layout==layer_fn) && layout[key_id]>=0x80) { // Mod keys
		//mod_keys |= ((layer_fn[key_id]!=0x80) ? (layer_fn[key_id] & 0x7F) : layer_fn[key_id]);
		mod_keys |= (layout[key_id] & 0x7F);
		send();
	} else {
		for(i=5; i>0; i--) queue[i] = queue[i-1];
		queue[0] = key_id;
		send();
		// Autorepeat
		if (last_key==key_id) {
			press_time2 = 1;
			repeat_time = 0;
		} else {
			last_key = key_id;
			press_time = 1;
			press_time2 = 0;
			release_time = 0;
			repeat_time = 0;
		}
	}
}

void key_release(uint8_t key_id) {
	uint8_t i;
	int8_t pressed_key_id = pressed[key_id];
	pressed[key_id] = 0;
	if(pressed_key_id==1 && layer_fn[key_id]>=0x80) { // Fn+Mod_keys release
		//mod_keys &= ~((layer_fn[key_id]!=0x80) ? (layer_fn[key_id] & 0x7F) : layer_fn[key_id]);
		mod_keys &= ~(layer_fn[key_id] & 0x7F);
		send();
	} else if((pressed_key_id!=KEY_PRESSED_FN || layout==layer_fn) && layout[key_id]>0xF0) { // Catboard keys release
		if (layout[key_id]==KEY_ALT_TAB && pressed_key_id!=KEY_PRESSED_ALT) { // ALT_TAB release
			mod_keys &= ~(KEY_ALT);
			send();
		}
	/*} else if(pressed_key_id==2 && layer_fn[key_id]==(KEY_CTRL+KEY_MOD)) { // Fn+KEY_CTRL release
		//mod_keys &= ~((layer_fn[key_id]!=0x80) ? (layer_fn[key_id] & 0x7F) : layer_fn[key_id]);
		mod_keys &= ~(KEY_SHIFT);
		send();*/
	} else if((pressed_key_id!=KEY_PRESSED_FN || layout==layer_fn) && layout[key_id]>=0x80) { // Mod_keys release
		//mod_keys &= ~((layout[key_id]!=0x80) ? (layout[key_id] & 0x7F) : layout[key_id]);
		mod_keys &= ~(layout[key_id] & 0x7F);
		send();
	} else {
		for(i=0; i<6; i++) {
			if(queue[i]==key_id) {
				break;
			}
		}
		for(; i<6; i++) {
			queue[i] = queue[i+1];
		}
		send();
		// Autorepeat
		if (last_key==key_id) {
			if (release_time) {
				press_time2 = 0;
				release_time = 1;
				repeat_time = 0;
			} else {
				press_time2 = 0;
				release_time = 1;
				repeat_time = 0;
			}
		} else {
			press_time = 0;
			press_time2 = 0;
			release_time = 0;
			repeat_time = 0;
		}
	}
}

void change_layout(void) {
	if (KEY_LAYOUT==KEY_LAYOUT_ALT_SHIFT) {
		keyboard_modifier_keys = KEY_ALT;
		keyboard_keys[0] = 0;
		usb_keyboard_send();
		_delay_ms(50);
		usb_keyboard_press(0, KEY_ALT|KEY_SHIFT);
	} else if (KEY_LAYOUT==KEY_LAYOUT_CTRL_SHIFT) {
		keyboard_modifier_keys = KEY_CTRL;
		keyboard_keys[0] = 0;
		usb_keyboard_send();
		_delay_ms(50);
		usb_keyboard_press(0, KEY_CTRL|KEY_SHIFT);
	} else if (KEY_LAYOUT==KEY_LAYOUT_GUI_SPACE) {
		keyboard_modifier_keys = KEY_GUI;
		keyboard_keys[0] = 0;
		usb_keyboard_send();
		_delay_ms(50);
		usb_keyboard_press(KEY_SPACE, KEY_GUI);
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
