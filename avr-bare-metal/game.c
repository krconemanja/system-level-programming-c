#include <avr/interrupt.h>
#include <avr/io.h>
#include "7seg.h"
#include "led.h"
#include "timer.h"

#define ANIMATION_DELAY 100
#define TIME_BTW_LEVELS 1000

typedef enum {ZERO_TO_SEVEN, SEVEN_TO_ZERO} direction_t;

// Button press interrupt changes this variable, play() reads it. Needs to be volatile to ensure it's read from and written back to memory every time.
static volatile uint8_t btn_event = 0;

// Interrupt service routine for button0
ISR(INT0_vect) {
	btn_event++;
}

static void init(void) {
	// Button 0
	DDRD &= ~(1 << PD2); //Input
	PORTD |= (1 << PD2); //Pull-up resistor

	EICRA |= (1 << ISC01); //Set interrupt on falling edge of INT0
	EICRA &= ~(1 << ISC00);
}

static void play(uint16_t wait_time) {
	uint8_t cursor = 0;
	uint8_t current_state = 0x00;
	direction_t cursor_direction = ZERO_TO_SEVEN;

	while(current_state != 0xff) {

		//Toggle cursor led
		current_state ^= (1 << cursor); //Set bitmask using XOR
		sb_led_setMask(current_state);

		//Wait, looking for button press
		EIMSK |= (1 << INT0); //Unmask INT0
		btn_event = 0;
		sb_timer_delay(wait_time);
		EIMSK &= ~(1 << INT0); //Mask INT0

		if (btn_event % 2 == 0) {	//Button was pressed odd number of times or wasn't pressed => keep bitmask
			//Toggle cursor led
			current_state ^= (1 << cursor);
			sb_led_setMask(current_state);
		}

		// Cursor movement
		if (cursor_direction == ZERO_TO_SEVEN) {
			if (cursor < 7) {
				cursor++;
			} else {
				cursor_direction = SEVEN_TO_ZERO;
				cursor--; //Avoiding double wait time at LED 7
			}

		} else {
			if (cursor > 0) {
				cursor--;
			} else {
				cursor_direction = ZERO_TO_SEVEN;
				cursor++; //Avoiding double wait time at LED 0
			}
		}
	}
}

static void show_win(void) {
	uint8_t win_mask = 0xff;

	//Switch off all LEDs one by one, from LED7 to LED0
	for (uint8_t i = 0; i < 8; i++) {
		win_mask &= ~(1 << (7 - i));
		sb_led_setMask(win_mask);
		sb_timer_delay(ANIMATION_DELAY);
	}

	//Cursor moves from LED7 to LED0 and back
	for (uint8_t i = 0; i < 8; i++) {
		win_mask ^= (1 << (7 - i));
		sb_led_setMask(win_mask);
		sb_timer_delay(ANIMATION_DELAY);
		win_mask ^= (1 << (7 - i));
		sb_led_setMask(win_mask);
	}
	for (uint8_t i = 1; i < 8; i++) {
		win_mask ^= (1 << i);
		sb_led_setMask(win_mask);
		sb_timer_delay(ANIMATION_DELAY);
		win_mask ^= (1 << i);
		sb_led_setMask(win_mask);
	}

	//Turning on from edges to center
	for (uint8_t i = 0; i < 4; i++) {
		win_mask |= (1 << i) | (1 << (7-i));
		sb_led_setMask(win_mask);
		sb_timer_delay(ANIMATION_DELAY);
	}

	//Turning off from center to edges
	for (uint8_t i = 0; i < 4; i++) {
		win_mask &= ~(1 << (3-i)) & ~(1 << (4+i));
		sb_led_setMask(win_mask);
		sb_timer_delay(ANIMATION_DELAY);
	}

	//Wait a bit before next level
	sb_timer_delay(TIME_BTW_LEVELS);
}

void main(void) {
	// Initialization
	init();
	sei();

	uint8_t level = 1;
	uint16_t waiting_time = 500; //Time in ms

	while(1) {
		// Start level
		sb_7seg_showNumber(level);
		play(waiting_time);

		// Show win sequence
		show_win();

		// Update level
		if(level < 99) {
			level++;
		}

		waiting_time = (500 / level) + 25;
	}
}
