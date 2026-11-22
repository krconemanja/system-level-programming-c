#include "led.h"
#include "7seg.h"
#include <avr/interrupt.h>

static void wait(uint32_t cycles) {
	volatile uint32_t i = 0; //volatile so the wait function is not optimized

	while (i < cycles) {
		i++;
	}
}

static void clear(void) {
	sb_led_setMask(0x00);
	sb_7seg_showString("  ");
	wait(500000);
}

void main(void){
	sei();

	while (1) {

		sb_7seg_showString("On"); //Mark the start of the test
		wait(500000);

		//Test sb_led_on & sb_led_off
		for (int8_t i = -2; i < 10; i++) {
			sb_7seg_showNumber( sb_led_on(i) );
			wait(200000);
			sb_7seg_showNumber( sb_led_off(i) );
		}

		clear();

		//Test sb_led_toggle
		for (int8_t i = -2; i < 10; i++) {
			sb_7seg_showNumber( sb_led_toggle(i) );
			wait(200000);
			sb_7seg_showNumber( sb_led_toggle(i) );
		}

		clear();

		//Test sb_led_showLevel
		sb_7seg_showNumber( sb_led_showLevel(20, 5) ); //level > max - should return -1
		wait(200000);
		sb_7seg_showNumber( sb_led_showLevel(50, 0) ); //max == 0 - should return -2
		wait(200000);
		for (uint16_t i = 0; i < 255; i += 16) {
			sb_7seg_showNumber( sb_led_showLevel( (uint8_t)i, 255) );
			wait(200000);
		}


		clear();

		//Test sb_led_setMask
		for (uint8_t i = 0; i < 0xff; i++) {
			sb_led_setMask(i);
			wait(10000);
		}

		clear();
	}
}
