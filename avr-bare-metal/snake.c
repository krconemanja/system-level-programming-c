#include "led.h"
#include "adc.h"
#include "button.h"

//Define Preprocessor shortcuts
#define MINDELAY 10000
#define SPEEDFACTOR 50
#define ADC_MAX_VALUE (1<<10)

static int wait(uint8_t mode) {
	// Wait function that determines the waiting time based on the light level.
	// param: mode is the current mode of the snake
	// Returns the mode for the next iteration.

	// sb_adc_read returns a signed 16 bit integer value
	int16_t photo_value = sb_adc_read(PHOTO);

	// Error handling
	if(photo_value == -1) {
		return 0;
	}

	// Number of cycles for the chosen values is not exceeding 10.000 + 50 * 2^10 < 2^16
	// Therefore, a 16 bit unsigned integer is enough.
	uint16_t delay = SPEEDFACTOR * ((1<<10) - (uint16_t) photo_value) + MINDELAY;

	// Volatile is not necessarily needed here, because there are commands in the active waiting loop that are not optimized by the compiler.
	uint16_t i = 0;

	BUTTONSTATE old_btn_state = sb_button_getState(BUTTON0);
	uint8_t press_count = 0;

	while(i < delay) {

		// Count button presses
		BUTTONSTATE btn_state = sb_button_getState(BUTTON0);
		if(old_btn_state == RELEASED && btn_state == PRESSED) {
			press_count++;
		}

		old_btn_state = btn_state;

		i++;
	}

	if(press_count % 2 == 1) {
		if(mode == 1) {
			return 0;
		}
		return 1;
	}

	return mode;
}

static void drawSnake(uint8_t head, uint8_t length, uint8_t mode) {

	// Create bit mask for LEDs
	uint8_t mask = 0x0;
	for(uint8_t i = 0; i < length; i++) {
		mask |= ((1 << (head + i) % 8));
	}

	// Invert if mode is 1
	if(mode == 1) {
		mask = ~mask;
	}

	sb_led_setMask(mask);
}

static uint8_t getLength() {

	int16_t poti_value = sb_adc_read(POTI);

	if(poti_value == -1) {
		return 0;
	}

	// We need five intervals of nearly equal size. To achieve that, we divide the measured value by the bucket size which is 1/5 of the maximum measure value.
	// Since int division is rounding to bottom, we add 1 to the result.
	return 5 - (5 * poti_value / ADC_MAX_VALUE);
}

void main(void) {

	// Define Variables
	uint8_t head = 0;
	uint8_t mode = 0;
	uint8_t length;

	while(1) {

		//Calculate Length
		length = getLength();

		//Draw Snake
		drawSnake(head, length, mode);

		// Wait and determine mode
		mode = wait(mode);

		// Move head to next position
		head++;
	}

}
