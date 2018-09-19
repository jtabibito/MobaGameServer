#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uv.h"
#include "timelist.h"

struct TimeController {
	uv_timer_t timer;
	void(*on_timer)(void* udata);
	void* udata;
	int repeat_count;
};

static struct TimeController*
alloc_Timer(void(*on_timer)(void* udata),
	void* udata,
	int repeat_count) {
	struct TimeController* t = (struct TimeController*)malloc(sizeof(struct TimeController));
	memset(t, 0, sizeof(struct TimeController));

	t->on_timer = on_timer;
	t->repeat_count = repeat_count;
	t->udata = udata;
	uv_timer_init(uv_default_loop(), &t->timer);
	return t;
}

static void
uv_timer_callback(uv_timer_t* handle) {
	struct TimeController* t = handle->data;
	if (t->repeat_count < 0) {
		t->on_timer(t->udata);
	} else {
		t->repeat_count--;
		t->on_timer(t->udata);
		if (t->repeat_count == 0) {
			uv_timer_stop(&t->timer);
			free(t);
		}
	}
}

struct TimeController*
schedule_repeat(void(*on_timer)(void* udata),
		void* udata,
		float after_sec,
		int repeat_count,
		int repeat_sec) {
	struct TimeController* t = alloc_Timer(on_timer, udata, repeat_count);

	t->timer.data = t;
	uv_timer_start(&t->timer, uv_timer_callback, (uint64_t)after_sec, (uint64_t)repeat_sec);

	return t;
}

void cancel_timer(struct TimeController* t) {
	if (t->repeat_count == 0) {
		return;
	}
	uv_timer_stop(&t->timer);
	free(t);
}

struct TimeController*
schedule_once(void(*on_timer)(void* udata),
	void* udata,
	float after_sec) {
	return schedule_repeat(on_timer, udata, after_sec, 1, 0);
}

void* 
get_timer_udata(struct TimeController* t) {
	return t->udata;
}
