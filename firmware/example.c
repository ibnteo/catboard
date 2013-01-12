/* Mouse example with debug channel, for Teensy USB Development Board
 * http://www.pjrc.com/teensy/usb_mouse.html
 * Copyright (c) 2009 PJRC.COM, LLC
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "usb_mouse.h"

#define LED_CONFIG	(DDRD |= (1<<6))
#define LED_ON		(PORTD &= ~(1<<6))
#define LED_OFF		(PORTD |= (1<<6))
#define CPU_PRESCALE(n)	(CLKPR = 0x80, CLKPR = (n))

int8_t circle[];

int main(void)
{
	int8_t x, y, *p;
	uint8_t i;

	// set for 16 MHz clock
	CPU_PRESCALE(0);
	LED_CONFIG;
	LED_OFF;

	// Initialize the USB, and then wait for the host to set configuration.
	// If the Teensy is powered without a PC connected to the USB port,
	// this will wait forever.
	usb_init();
	while (!usb_configured()) /* wait */ ;

	// Wait an extra second for the PC's operating system to load drivers
	// and do whatever it does to actually be ready for input
	_delay_ms(1000);

	while (1) {
		_delay_ms(1000);
		LED_ON;  // turn the LED on while moving the mouse
		p = circle;
		for (i=0; i<36; i++) {
			x = pgm_read_byte(p++);
			y = pgm_read_byte(p++);
			usb_mouse_move(x, y, 0);
			_delay_ms(20);
		}
		LED_OFF;
		_delay_ms(9000);
		// This sequence creates a right click
		//usb_mouse_buttons(0, 0, 1);
		//_delay_ms(10);
		//usb_mouse_buttons(0, 0, 0);
	}
}


int8_t PROGMEM circle[] = {
16, -1,
15, -4,
14, -7,
13, -9,
11, -11,
9, -13,
7, -14,
4, -15,
1, -16,
-1, -16,
-4, -15,
-7, -14,
-9, -13,
-11, -11,
-13, -9,
-14, -7,
-15, -4,
-16, -1,
-16, 1,
-15, 4,
-14, 7,
-13, 9,
-11, 11,
-9, 13,
-7, 14,
-4, 15,
-1, 16,
1, 16,
4, 15,
7, 14,
9, 13,
11, 11,
13, 9,
14, 7,
15, 4,
16, 1
};
