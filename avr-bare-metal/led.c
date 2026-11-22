#if 1

#include "led.h"
#include <avr/io.h>

//Static for both because the arrays are not part of the interface, so they should only be accessible from within the module.

//Volatile because we need to ensure that the state of the pin is always written back to memory.
//Const because the pointers will always point to the same memory addresses.
static volatile uint8_t * const ports[8] = {&PORTD, &PORTD, &PORTD, &PORTD, &PORTB, &PORTB, &PORTC, &PORTC};

//Const because the PD6 - PC2 values are constant, i.e. they are not going to change during the program.
static uint8_t const pins[8] = {PD6, PD5, PD4, PD7, PB0, PB1, PC3, PC2};

static void init(void) {
	static uint8_t initDone = 0;
	if (initDone == 0) {
		initDone = 1;

		//Initialize DDRx for LEDs as outputs
		DDRD |= (1 << PD6);
		DDRD |= (1 << PD5);
		DDRD |= (1 << PD4);
		DDRD |= (1 << PD7);

		DDRB |= (1 << PB0);
		DDRB |= (1 << PB1);

		DDRC |= (1 << PC3);
		DDRC |= (1 << PC2);

		//Set all LEDs to 1 (off) initially
		PORTD |= (1 << PD6);
		PORTD |= (1 << PD5);
		PORTD |= (1 << PD4);
		PORTD |= (1 << PD7);

		PORTB |= (1 << PB0);
		PORTB |= (1 << PB1);

		PORTC |= (1 << PC3);
		PORTC |= (1 << PC2);
	}
}

int8_t sb_led_on(LED led) {
	if (led > 7) {
		return -1;
	}

	init();

	//Turn on LED
	* (ports[led]) &= ~(1 << pins[led]);

	return 0;
}

int8_t sb_led_off(LED led) {
	if (led > 7) {
		return -1;
	}

	init();

	//Turn off LED
	* (ports[led]) |= (1 << pins[led]);

	return 0;
}

int8_t sb_led_toggle(LED led) {
	if (led > 7) {
		return -1;
	}

	init();

	if ( (*ports[led] & (1 << pins[led]) ) == 0) {
		sb_led_off(led);
	} else {
		sb_led_on(led);
	}

	return 0;
}

void sb_led_setMask(uint8_t mask) {

	init();

	for (uint8_t i = 0; i < 8; i++) {
		if (mask & (1 << i)) {
			sb_led_on(i);
		} else {
			sb_led_off(i);
		}
	}
}

int8_t sb_led_showLevel(uint8_t level, uint8_t max) {
	if (max == 0) {
		return -2;
	}

	if (level > max) {
		return -1;
	}

	init();

	sb_led_setMask(0x00);

	uint8_t num_leds = 9 * level / max;

	for (uint8_t i = 0; i < num_leds; i++ ) {
		sb_led_on(i);
	}

	return num_leds;
}

#endif
