/*
 * protimer_state_mach.c
 *
 *  Created on: Nov 3, 2025
 *      Author: PRAKASH DAS
 */

#include "main.h"
#include<string.h>
#include <iostream>
#include "ssd1306.h"
//prototypes of the helper functions

static void display_clear(void);
static void display_message(const char *s, uint8_t c, uint8_t r);
static void display_time(uint32_t time);

//prototypes of the state functions
static event_status_t protimer_state_handler_IDEL(protimer_t *const mobj,
		event_t const *const e);
static event_status_t protimer_state_handler_TIME_SET(protimer_t *const mobj,
		event_t const *const e);
static event_status_t protimer_state_handler_COUNTDOWN(protimer_t *const mobj,
		event_t const *const e);
static event_status_t protimer_state_handler_PAUSE(protimer_t *const mobj,
		event_t const *const e);
static event_status_t protimer_state_handler_STAT(protimer_t *const mobj,
		event_t const *const e);


#define IDEL		&protimer_state_handler_IDEL
#define TIME_SET	&protimer_state_handler_TIME_SET
#define COUNTDOWN	&protimer_state_handler_COUNTDOWN
#define PAUSE		&protimer_state_handler_PAUSE
#define STAT		&protimer_state_handler_STAT

void protimer_init(protimer_t *mobj) {
	event_t ee;
	ee.sig = ENTRY;
	mobj->active_state = IDEL;
	mobj->pro_time = 0;
	(*mobj->active_state)(mobj, &ee);
}


static event_status_t protimer_state_handler_IDEL(protimer_t *const mobj,
		event_t const *const e) {
	switch (e->sig) {
	case ENTRY: {
		mobj->curr_time = 0;
		mobj->elapsed_time = 0;
		display_time(0);
		display_message("Set time", 20, 30);
		return EVENT_HANDLED;
	}
	case EXIT: {
		display_clear();
		return EVENT_HANDLED;

	}
	case INC_TIME: {
		mobj->curr_time += 60;
		mobj->active_state = TIME_SET;
		return EVENT_TRANSITION;
	}
	case START_PAUSE: {
		mobj->active_state = STAT;
		return EVENT_TRANSITION;

	}
	case TIME_TICK: {
		if (((protimer_tick_event_t*) (e))->ss == 5) {
			return EVENT_HANDLED;
		}
		return EVENT_IGNORED;

	}
	} // end of switch case
	return EVENT_IGNORED;
}

static event_status_t protimer_state_handler_TIME_SET(protimer_t *const mobj,
		event_t const *const e) {
	switch (e->sig) {
	case ENTRY: {
		display_time(mobj->curr_time);
		return EVENT_HANDLED;
	}
	case EXIT: {
		display_clear();
		return EVENT_HANDLED;
	}
	case INC_TIME: {
		mobj->curr_time += 60;
		display_time(mobj->curr_time);
		return EVENT_HANDLED;
	}
	case DEC_TIME: {
		if (mobj->curr_time >= 60) {
			mobj->curr_time -= 60;
			display_time(mobj->curr_time);
			return EVENT_HANDLED;
		}
		return EVENT_IGNORED;
	}
	case ABRT: {
		mobj->active_state = IDEL;
		return EVENT_TRANSITION;
	}
	case START_PAUSE: {
		if (mobj->curr_time >= 60) {
			mobj->active_state = COUNTDOWN;
			return EVENT_TRANSITION;
		}
		return EVENT_HANDLED;
	}
	}
	return EVENT_IGNORED;
}

static event_status_t protimer_state_handler_COUNTDOWN(protimer_t *const mobj,
		event_t const *const e) {
	switch (e->sig) {
	case EXIT: {
		mobj->pro_time += mobj->elapsed_time;
		mobj->elapsed_time = 0;
		return EVENT_HANDLED;
	}
	case TIME_TICK: {
		if (((protimer_tick_event_t*) (e))->ss == 10) {
			--mobj->curr_time;
			++mobj->elapsed_time;
			display_time(mobj->curr_time);

			if (mobj->curr_time == 0) {
				mobj->active_state = IDEL;
				return EVENT_TRANSITION;
			}
			return EVENT_HANDLED;
		}
		return EVENT_HANDLED;
	}
	case START_PAUSE: {
		mobj->active_state = PAUSE;
		return EVENT_TRANSITION;
	}
	case ABRT: {
		mobj->active_state = IDEL;
		return EVENT_TRANSITION;
	}

	}
	return EVENT_IGNORED;
}

static event_status_t protimer_state_handler_PAUSE(protimer_t *const mobj,
		event_t const *const e) {
	switch (e->sig) {
	case ENTRY: {
		display_message("Paused", 30, 30);
		return EVENT_HANDLED;
	}
	case EXIT: {
		display_clear();
		return EVENT_HANDLED;
	}
	case INC_TIME: {
		mobj->curr_time += 60;
		mobj->active_state = TIME_SET;
		return EVENT_TRANSITION;
	}
	case DEC_TIME: {
		if (mobj->curr_time >= 60) {
			mobj->curr_time -= 60;
			mobj->active_state = TIME_SET;
			return EVENT_TRANSITION;
		}
		return EVENT_IGNORED;
	}
	case START_PAUSE: {
		mobj->active_state = COUNTDOWN;
		return EVENT_TRANSITION;
	}
	case ABRT: {
		mobj->active_state = IDEL;
		return EVENT_TRANSITION;
	}
	}
	return EVENT_IGNORED;
}

static event_status_t protimer_state_handler_STAT(protimer_t *const mobj,
		event_t const *const e) {

	static uint8_t tick_count;

	switch (e->sig) {
	case ENTRY: {
		display_time(mobj->pro_time);
		display_message("Productive Time", 10, 30);
		return EVENT_HANDLED;
	}
	case EXIT: {
		display_clear();
		return EVENT_HANDLED;
	}
	case TIME_TICK: {
		if (++tick_count == 10) {
			tick_count = 0;
			mobj->active_state = IDEL;
			return EVENT_TRANSITION;
		}
		return EVENT_IGNORED;
	}
	}
	return EVENT_IGNORED;
}

//helper functions

static void display_time(uint32_t time) {
	char buf[7];

	uint16_t m = time / 60;
	uint8_t s = time % 60;
	sprintf(buf, "%03d:%02d", m, s);

	SSD1306_GotoXY(30, 0);
	SSD1306_Puts(buf, &Font_11x18, SSD1306_COLOR_WHITE);
	SSD1306_UpdateScreen();
}

static void display_message(const char *s, uint8_t c, uint8_t r) {
	if (s == NULL)
		return;  // safety check
	// Set the OLED cursor position (column = c, row = r)
	SSD1306_GotoXY(c, r);

	// Print the string with your desired font and color
	SSD1306_Puts(s, &Font_11x18, SSD1306_COLOR_WHITE);

	// Update OLED to display the text
	SSD1306_UpdateScreen();
}

static void display_clear(void) {
	SSD1306_Clear();
}
