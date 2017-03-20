/*
	 IR control for Pentax - ATtiny10 IR controller
	 Copyright (c) EHbtj<contact at ehbtj.com>.  All right reserved.
	 
	 Reference code
	 ir remote  for DSLR  by morecat_lab
	 http://morecatlab.akiba.coocan.jp/lab/index.php/2012/03/tiny10-ir-remote/
	 
	 This code is free software; you can redistribute it and/or
	 modify it under the terms of the GNU Lesser General Public
	 License as published by the Free Software Foundation; either
	 version 2.1 of the License, or (at your option) any later version.
	 
	 Version 001 10 Aug 2013
 */

#define IRLEDPIN 1
#define SHUTBUTTON  2
#define FOCUSBUTTON 0

#include <avr/io.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <avr/interrupt.h>

extern void delay_1u(uint8_t);
extern void delay_100u(uint16_t);
extern void pulse_100u(uint16_t);
extern void pentax_shutter();
extern void pentax_focus();

#define V_OCR33 116  // 33KHz carrier @ 3V, 119 @ 5V
#define V_OCR38 100  // 38KHz carrier @ 3V, 103 @ 5V
#define V_OCR40 95   // 40KHz carrier @ 3v,  98 @ 5V

#define ON_OCB0   TCCR0A = 0b00010000
#define OFF_OCB0  TCCR0A = 0b00000000

// 10 instructions (1.0uS @ 8MHz) delay for 1 loop
void delay_1u(uint8_t __count) {
	__asm__ volatile (
	"1: dec %0" "\n\t"
	"nop" "\n" "nop" "\n"
	"nop" "\n" "nop" "\n"
	"nop" "\n" "nop" "\n"
	"brne 1b"
	: "=r" (__count)
	: "0" (__count)
	);
}

// 100uS delay
void delay_100u(uint16_t __count) {
	uint16_t i;
	for (i = 0;i < __count;i++) {
		delay_1u(100);
	}
}

void pulse_100u(uint16_t t1) {
	ON_OCB0;
	delay_100u(t1);
	OFF_OCB0;
}

void delay_ms (uint16_t dly) {
	uint8_t n, d;
	do {
		n = 31;
		do {
			d = PINB;
		} while (--n);
	} while (--dly);
}

EMPTY_INTERRUPT(INT0_vect);

int main() {
	CCP = 0xD8;
	CLKMSR = 0x00;  // internal 8MHz
	CCP = 0xD8;
	CLKPSR = 0x00;  // prescale = 1
	CCP = 0xD8;
	OSCCAL = 160;   // ~= 8MHz at 5V

	DDRB = (1 << IRLEDPIN); // output
	PUEB = ((1 << FOCUSBUTTON) | (1 << SHUTBUTTON));		// Pull-up
	
	GTCCR = 0;

	// OC0B (PB1)を初期化 CTC MODE WGSM0 3:0 = 0100
	TCCR0A = 0b00010000;
	// unuse OC0A , OC0B (Toggle OC0B on compare match), CTC
	TCCR0B = 0b00001001;
	// no prescaler (1/1 clock)

	// CTCの周波数を設定する
	OCR0A = V_OCR38;		// for 38KHz carrier
	OFF_OCB0;

	EICRA = (1 << ISC01);		// falling edge of INT0 
	EIMSK = (1 << INT0);		// INT0 enable 
	sei();
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);

	while (1) {
		//sleep_mode();			// Enter Power-Down mode
												// ボタンが2つになると復帰できないのでスリープしない

		if (bit_is_clear(PINB, SHUTBUTTON)) {
			delay_ms(10);
			if (bit_is_set(PINB, SHUTBUTTON)) {
				continue;		// if chattering, go to sleep again
			}
			pentax_shutter();
			delay_ms(10);
			while (bit_is_clear(PINB, SHUTBUTTON)); // wait until button release
		}

		if (bit_is_clear(PINB, FOCUSBUTTON)) {
			delay_ms(10);
			if (bit_is_set(PINB, FOCUSBUTTON)) {
				continue;		// if chattering, go to sleep again
			}
			pentax_focus();
			delay_ms(10);
			while (bit_is_clear(PINB, FOCUSBUTTON)); // wait until button release
		}
	}
}

void pentax_shutter() {
	uint8_t i;
	OCR0A = V_OCR38;
	pulse_100u(132);
	delay_100u(32);
	for (i = 0;i < 7;i++) {
		pulse_100u(10);
		delay_100u(10);
	}
}

void pentax_focus() {
	uint8_t i;
	OCR0A = V_OCR38;
	pulse_100u(132);
	delay_100u(32);
	for (i = 0;i < 5;i++) {
		pulse_100u(10);
		delay_100u(10);
	}
	pulse_100u(10);
	delay_100u(32);
	pulse_100u(10);
}