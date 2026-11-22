#include <avr/sleep.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "adc.h"
#include "7seg.h"
#include "led.h"

#define INTERRUPTS_PER_SECOND 61

typedef enum {STATE_OFF, STATE_BOOT, STATE_ON_7SEG, STATE_TRANS_7SEG, STATE_ON_LED, STATE_TRANS_LED} state;

//Event variables accessed by main() and ISR so we need to have it global, but static so only visible in the module.
//Volatile because we need it to be read from and written back to memory every time.
static volatile uint8_t btn0_event = 0;
static volatile uint8_t btn1_event = 0;

//Timer interrupt variables
static volatile uint8_t timer_interrupt_counter = 0; //Counts how many timer interrupts have happened
static volatile uint8_t seconds_passed = 0; //Increments every 61 interrupts (when timer_interrupt_counter reaches INTERRUPTS_PER_SECOND)
static volatile uint8_t one_second_flag = 0;
// ^ Goes to 1 if a second has passed. Used because we need to wake up every second to read the sensor,
// but we also need the timer to run up to 15s at some points in the program. With it we can also manipulate when we want the sleep cycle to be skipped.

ISR(INT0_vect) {
	btn0_event = 1;
}

ISR(INT1_vect) {
	btn1_event = 1;
}

ISR(TIMER0_OVF_vect) {
	timer_interrupt_counter++;

	if(timer_interrupt_counter >= INTERRUPTS_PER_SECOND) {
		timer_interrupt_counter = 0;
		seconds_passed++;
		one_second_flag = 1;
	}
}

static void init(void) {
	//Initialization button0
	DDRD &= ~(1 << PD2); //Input
	PORTD |= (1 << PD2); //Pull-up resistor

	EICRA |= (1 << ISC01); //Set interrupt on falling edge of INT0
	EICRA &= ~(1 << ISC00);

	EIMSK |= (1 << INT0); //Unmask interrupt for INT0

	//Initialization button1
	DDRD &= ~(1 << PD3); //Input
	PORTD |= (1 << PD3); //Pull-up resistor

	EICRA |= (1 << ISC10); //Set interrupt on either edge of INT1
	EICRA &= ~(1 << ISC11);

	EIMSK |= (1 << INT1); //Unmask interrupt for INT1

	//Initialization timer with prescaler 1024
	TCCR0B &= ~(1 << CS01);
	TCCR0B |= (1 << CS02) | (1 << CS00);

	TIMSK0 |= (1 << TOIE0); // Unmask timer interrupt

	sleep_enable(); //Enable sleep
	sei(); //Enable interrupts
}

static state off_f(void) {
	sb_led_setMask(0x00); //Clear the LEDs
	sb_7seg_showString("  "); //Clear the 7seg

	if(btn0_event) {
		btn0_event = 0;

		seconds_passed = 0;
		timer_interrupt_counter = 0;
		one_second_flag = 1; //We want to go to BOOT immediately, i.e. we want to skip the sleep cycle

		return STATE_BOOT;
	}

	one_second_flag = 0; //We set these two to 0 because otherwise we would not be sleeping in OFF because the while() condition for the sleep loop would always be true.
	btn1_event = 0;

	return STATE_OFF;
}

static state boot_f(void) {

	sb_7seg_showString("hi");

	if(seconds_passed) {
		seconds_passed = 0; //We reset the timer before entering the next state, because there we need to start counting time from 0
		timer_interrupt_counter = 0;

		btn1_event = 0; //Once we go into ON_7SEG we don't want to process random btn1 presses that occured during OFF or BOOT
		return STATE_ON_7SEG; //We go into ON_7SEG with one_second_flag == 1 so we are going in immediately
	}

	//Allows the CPU to go to sleep during the 1s wait-time in BOOT
	btn1_event = 0;
	btn0_event = 0;
	one_second_flag = 0;

	return STATE_BOOT;
}

static state on_7seg_f(void) {

	//Check which event happened - button press or timeout
	if(btn1_event) {
		btn1_event = 0;

		seconds_passed = 0;
		timer_interrupt_counter = 0;
		one_second_flag = 1; //We immediately go to TRANS_7SEG, *not* going to sleep for 1s in between

		return STATE_TRANS_7SEG;
	}

	if(seconds_passed >= 15) {
		btn0_event = 0;
		one_second_flag = 1;

		return STATE_OFF;
	}

	int16_t photo_value = sb_adc_read(PHOTO);

	if(photo_value == -1) {			//Print Er (error) if sb_adc_read returns -1
		sb_7seg_showString("Er");
	}

	sb_7seg_showNumber(100 * (uint32_t)(photo_value) / (1 << 10) ); //We can safely cast to uint because we already checked the only negative option. We need uint32_t because we're multiplying by 100 so uint16_t is not enough

	//Allows the CPU to go to sleep while waiting for the next update
	one_second_flag = 0;
	btn0_event = 0;

	return STATE_ON_7SEG;
}

static state trans_7seg_f(void) {

	if(btn1_event) {
		btn1_event = 0;

		seconds_passed = 0;
		timer_interrupt_counter = 0;
		one_second_flag = 1;

		sb_7seg_showString("  ");

		return STATE_ON_LED;
	}

	if(seconds_passed) {
		btn0_event = 0;
		return STATE_OFF; //We go immediately to OFF, because by design, if seconds_passed, then also one_second_flag == 1, so we skip the sleep cycle
	}

	one_second_flag = 0;

	return STATE_TRANS_7SEG;
}

static state on_led_f(void) {

	if(btn1_event) {
		btn1_event = 0;

		seconds_passed = 0;
		timer_interrupt_counter = 0;
		one_second_flag = 1;

		return STATE_TRANS_LED;
	}

	if(seconds_passed >= 15) {
		btn0_event = 0;
		one_second_flag = 1;

		return STATE_OFF;
	}

	int16_t photo_value = sb_adc_read(PHOTO);

	if(photo_value == -1) {			//Print Er (error) if sb_adc_read returns -1
		sb_7seg_showString("Er");
	}

	int8_t showLevel_return = sb_led_showLevel( (uint8_t)(photo_value / 10), 102); //sb_led_showLevel() takes in two uint8_t values so we must cast photo_value to uint8_t but because it's too big we scale by 1/10

	if(showLevel_return < 0) {
		sb_7seg_showString("Er");
	}

	btn0_event = 0;
	one_second_flag = 0;

	return STATE_ON_LED;
}

static state trans_led_f(void) {

	if (btn1_event) {
		btn1_event = 0;

		seconds_passed = 0;
		timer_interrupt_counter = 0;
		one_second_flag = 1;

		sb_led_setMask(0x00);

		return STATE_ON_7SEG;
	}

	if(seconds_passed) {
		btn0_event = 0;

		return STATE_OFF;
	}

	one_second_flag = 0;

	return STATE_TRANS_LED;
}

void main(void) {

	init();

	state current_state = STATE_OFF;

	while(1) {

		cli();
		while((!btn0_event) && (!btn1_event) && (!one_second_flag)) {
			sei();
			sleep_cpu();
			cli();
		}
		sei();

		switch (current_state) {
			case STATE_OFF:
				current_state = off_f();
				break;

			case STATE_BOOT:
				current_state = boot_f();
				break;

			case STATE_ON_7SEG:
				current_state = on_7seg_f();
				break;

			case STATE_TRANS_7SEG:
				current_state = trans_7seg_f();
				break;

			case STATE_ON_LED:
				current_state = on_led_f();
				break;

			case STATE_TRANS_LED:
				current_state = trans_led_f();
				break;

			default:
				//Something crazy
				sb_led_setMask(0xff);
				break;
		}
	}
}
