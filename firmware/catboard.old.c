/*
 * CatBoard
 * Version: 0.1
 * Date: 2012-12-31
 * Author: Vladimir Romanovich <ibnteo@gmail.com>
 * 
 * Board: AVR-USB162 (http://microsin.ru/content/view/685/44/)
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "usb_keyboard.h"
#include "catboard.h"

#define LED_CONFIG	(DDRD |= (1<<4))
#define LED_ON		(PORTD &= ~(1<<4))
#define LED_OFF		(PORTD |= (1<<4))
#define CPU_PRESCALE(n)	(CLKPR = 0x80, CLKPR = (n))

static const uint8_t PROGMEM scan_pc[] = {2,4,5,6,7};
static const uint8_t PROGMEM read_pb[] = {0,1,4,5,6,7};
static const uint8_t PROGMEM read_pd[] = {0,1,2,3,5,6};
static const uint8_t PROGMEM layout1[] = {
	KEY_ESC, KEY_TILDE, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0, KEY_MINUS, KEY_EQUAL,
	KEY_TAB, KEY_TAB, KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I, KEY_O, KEY_P, KEY_LEFT_BRACE, KEY_RIGHT_BRACE,
	KEY_CAPS_LOCK, KEY_CAPS_LOCK, KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L, KEY_SEMICOLON, KEY_QUOTE, KEY_BACKSLASH,
	KEY_LEFT_SHIFT, KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, KEY_M, KEY_COMMA, KEY_PERIOD, KEY_SLASH, KEY_RIGHT_SHIFT,
	KEY_LEFT_ALT, KEY_LEFT_CTRL, KEY_SHIFT, KEY_SPACE, KEY_FN, KEY_RIGHT_ALT
};
static const uint8_t PROGMEM layout2[] = {
	KEY_ESC, KEY_TILDE, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0, KEY_MINUS, KEY_EQUAL,
	KEY_TAB, KEY_TAB, KEY_J, KEY_C, KEY_U, KEY_K, KEY_E, KEY_N, KEY_G, KEY_H, KEY_W, KEY_Z, KEY_LEFT_BRACE, KEY_RIGHT_BRACE,
	KEY_CAPS_LOCK, KEY_CAPS_LOCK, KEY_F, KEY_Y, KEY_V, KEY_A, KEY_P, KEY_R, KEY_O, KEY_L, KEY_D, KEY_SEMICOLON, KEY_QUOTE, KEY_BACKSLASH,
	KEY_LEFT_SHIFT, KEY_Q, KEY_SLASH, KEY_S, KEY_M, KEY_I, KEY_T, KEY_X, KEY_B, KEY_SEMICOLON, KEY_PERIOD, KEY_RIGHT_SHIFT,
	KEY_LEFT_ALT, KEY_LEFT_CTRL, KEY_SHIFT, KEY_SPACE, KEY_FN, KEY_RIGHT_ALT
};
static const uint8_t PROGMEM layout_fn[] = {
	0, 0, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,
	0, 0, 0, 0, 0, 0, 0, 0, KEY_HOME, KEY_UP, KEY_END, KEY_PAGE_UP, 0, 0,
	0, 0, 0, 0, 0, 0, KEY_CTRL, KEY_ENTER, KEY_LEFT, KEY_DOWN, KEY_RIGHT, KEY_PAGE_DOWN, 0, 0,
	0, 0, 0, 0, 0, 0, KEY_BACKSPACE, KEY_DELETE, KEY_INSERT, 0, 0, 0,
	0, 0, 0, 0, 0, 0
};


uint8_t number_keys[10]=
	{KEY_0,KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,KEY_7,KEY_8,KEY_9};

//uint16_t idle_count=0;

int main(void)
{
	uint8_t b, d, mask, i;//, reset_idle;
	uint8_t b_prev=0xFF, d_prev=0xFF;

	// set for 16 MHz clock
	CPU_PRESCALE(0);

	// Configure all port B and port D pins as inputs with pullup resistors.
	// See the "Using I/O Pins" page for details.
	// http://www.pjrc.com/teensy/pins.html
	DDRD = 0x00;
	DDRB = 0x00;
	PORTB = 0xFF;
	PORTD = 0xFF;
	LED_CONFIG;
	

	// Initialize the USB, and then wait for the host to set configuration.
	// If the Teensy is powered without a PC connected to the USB port,
	// this will wait forever.
	usb_init();
	while (!usb_configured()) /* wait */ ;

	// Wait an extra second for the PC's operating system to load drivers
	// and do whatever it does to actually be ready for input
	_delay_ms(1000);

	// Configure timer 0 to generate a timer overflow interrupt every
	// 256*1024 clock cycles, or approx 61 Hz when using 16 MHz clock
	// This demonstrates how to use interrupts to implement a simple
	// inactivity timeout.
	//TCCR0A = 0x00;
	//TCCR0B = 0x05;
	//TIMSK0 = (1<<TOIE0);

	for (;;) {
		// read all port B and port D pins
		b = PINB;
		d = PIND;
		// check if any pins are low, but were high previously
		mask = 1;
		//reset_idle = 0;
		for (i=0; i<8; i++) {
			if (((b & mask) == 0) && (b_prev & mask) != 0) {
				LED_ON;
				usb_keyboard_press(KEY_B, KEY_SHIFT);
				usb_keyboard_press(number_keys[i], 0);
				//reset_idle = 1;
			}
			if (((d & mask) == 0) && (d_prev & mask) != 0) {
				LED_OFF;
				usb_keyboard_press(KEY_D, KEY_SHIFT);
				usb_keyboard_press(number_keys[i], 0);
				//reset_idle = 1;
			}
			mask = mask << 1;
		}
		// if any keypresses were detected, reset the idle counter
		/*if (reset_idle) {
			// variables shared with interrupt routines must be
			// accessed carefully so the interrupt routine doesn't
			// try to use the variable in the middle of our access
			cli();
			//idle_count = 0;
			hsei();
		}*/
		// now the current pins will be the previous, and
		// wait a short delay so we're not highly sensitive
		// to mechanical "bounce".
		b_prev = b;
		d_prev = d;
		_delay_ms(20);
	}
}

// This interrupt routine is run approx 61 times per second.
// A very simple inactivity timeout is implemented, where we
// will send a space character.
//ISR(TIMER0_OVF_vect)
//{
	/*idle_count++;
	if (idle_count > 61 * 8) {
		idle_count = 0;
		usb_keyboard_press(KEY_SPACE, 0);
	}*/
//}
