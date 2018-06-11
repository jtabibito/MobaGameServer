#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uv.h"
#include "timelist.h"

struct TimeController {
	uv_timer_t timer;
	void(*on_timer)(void* udata);
	void* udata;
	int repeat;
};

static struct TimeController*
alloc_Timer(void(*on_timer)(void* udata),
	void* udata,
	int repeat) {
	struct TimeController* t = (struct TimeController*)malloc(sizeof(struct TimeController));
	memset(t, 0, sizeof(struct TimeController));

	t->on_timer = on_timer;
	t->udata = udata;
	t->repeat = repeat;
	uv_timer_init(uv_default_loop(), &t->timer);
	return t;
}

static void
uv_timer_callback(uv_timer_t* handle) {
	struct TimeController* t = handle->data;
	if (t->repeat < 0) {
		t->on_timer(t->udata);
	}else {
		if (t->repeat == 0) {
			uv_timer_stop(&t->timer);
			free(t);
		}
		t->repeat--;
		t->on_timer(t->udata);
	}
	printf("...uv_timer_callback\n");
}

struct TimeController*
schedule(void(*on_timer)(void* udata),
		void* udata,
		float after_sec,
		int repeat) {
	struct TimeController* t = alloc_Timer(on_timer, udata, repeat);

	t->timer.data = t;
	uv_timer_start(&t->timer, uv_timer_callback, (uint64_t)after_sec, (uint64_t)repeat);

	return t;
}

void cancel_timer(struct TimeController* t) {
	if (t->repeat == 0) {
		return;
	}
	uv_timer_stop(&t->timer);
	free(t);
}