#include "simu_event.h"
#include <android/log.h>

static const char* tag = "simu_event";
extern int inject_action(int action_module, int action_type, int x, int y);

#define MIN_DIF_TIME 200000
#define gs_print(...)  __android_log_print(ANDROID_LOG_INFO, tag, __VA_ARGS__);

static int l_rel_x = 0x80;
static int l_rel_y = 0x80;
static int l_jst_x = 0;
static int l_jst_y = 0;

static int r_rel_x = 0;
static int r_rel_y = 0;
static int r_jst_x = 0;
static int r_jst_y = 0;

static int timer_status_adv_l = 0;
static int timer_status_adv_r = 0;

static int prev_abs_x;
static int prev_abs_y;
static int prer_abs_x;
static int prer_abs_y;

static int mouse_x = 0;
static int mouse_y = 0;

// transform the L switch event to event move
int transform_joystick_event_l_adv(struct input_event* pevent, int jst_x,
		int jst_y, float sensity) {
	if ((jst_x == 0) || (jst_y == 0)) {
		return 0;
	} else {
		l_jst_x = jst_x;
		l_jst_y = jst_y;
	}

	switch (pevent->code) {
	case ABS_X:
		l_rel_x = pevent->value;
		break;
	case ABS_Y:
		l_rel_y = pevent->value;
		break;
	default:
		break;
	}

	if (timer_status_adv_l == 0) {
		timer_status_adv_l = 1;
		inject_action(1, ACTION_DOWN, l_jst_x, l_jst_y);
		if ((l_rel_x != 0x80) || (l_rel_y != 0x80)) {
			prev_abs_x = l_jst_x + get_rel_hor_position() * sensity;
			prev_abs_y = l_jst_y + get_rel_ver_position() * sensity;
			if (prev_abs_x > 1920)
				prev_abs_x = 1920;
			if (prev_abs_x < 0)
				prev_abs_x = 0;
			if (prev_abs_y > 1080)
				prev_abs_y = 1080;
			if (prev_abs_y < 0)
				prev_abs_y = 0;
			inject_action(1, ACTION_MOVE, prev_abs_x, prev_abs_y);
		}
	} else if (timer_status_adv_l == 1) {
		if ((l_rel_x == 0x80) && (l_rel_y == 0x80)) {
			inject_action(1, ACTION_UP, prev_abs_x, prev_abs_y);
			prev_abs_x = l_jst_x;
			prev_abs_y = l_jst_y;
			timer_status_adv_l = 0;
		} else {
			prev_abs_x = l_jst_x + get_rel_hor_position() * sensity;
			prev_abs_y = l_jst_y + get_rel_ver_position() * sensity;
			if (prev_abs_x > 1920)
				prev_abs_x = 1920;
			if (prev_abs_x < 0)
				prev_abs_x = 0;
			if (prev_abs_y > 1080)
				prev_abs_y = 1080;
			if (prev_abs_y < 0)
				prev_abs_y = 0;
			inject_action(1, ACTION_MOVE, prev_abs_x, prev_abs_y);
		}
	}
	return 0;
}
// transform the R switch event to event move
int transform_joystick_event_r_adv(struct input_event* pevent, int jst_x,
		int jst_y, float sensity) {
	if ((jst_x == 0) || (jst_y == 0)) {
		return 0;
	} else {
		r_jst_x = jst_x;
		r_jst_y = jst_y;
	}

	switch (pevent->code) {
	case ABS_Z:
		r_rel_x = pevent->value;
		break;
	case ABS_RZ:
		r_rel_y = pevent->value;
		break;
	default:
		break;
	}

	if (timer_status_adv_r == 0) {
		timer_status_adv_r = 1;
		inject_action(2, ACTION_DOWN, r_jst_x, r_jst_y);
		if ((r_rel_x != 0x80) && (r_rel_y != 0x80)) {
			prer_abs_x = r_jst_x + get_rer_hor_position() * sensity;
			prer_abs_y = r_jst_y + get_rer_ver_position() * sensity;
			if (prer_abs_x > 1920)
				prer_abs_x = 1920;
			if (prer_abs_x < 0)
				prer_abs_x = 0;
			if (prer_abs_y > 1080)
				prer_abs_y = 1080;
			if (prer_abs_y < 0)
				prer_abs_y = 0;
			inject_action(2, ACTION_MOVE, prer_abs_x, prer_abs_y);
		}
	} else if (timer_status_adv_r == 1) {
		if ((r_rel_x == 0x80) && (r_rel_y == 0x80)) {
			inject_action(2, ACTION_UP, prer_abs_x, prer_abs_y);
			prer_abs_x = r_jst_x;
			prer_abs_y = r_jst_y;
			timer_status_adv_r = 0;
		} else {
			prer_abs_x = r_jst_x + get_rer_hor_position() * sensity;
			prer_abs_y = r_jst_y + get_rer_ver_position() * sensity;
			if (prer_abs_x > 1920)
				prer_abs_x = 1920;
			if (prer_abs_x < 0)
				prer_abs_x = 0;
			if (prer_abs_y > 1080)
				prer_abs_y = 1080;
			if (prer_abs_y < 0)
				prer_abs_y = 0;
			inject_action(2, ACTION_MOVE, prer_abs_x, prer_abs_y);
		}
	}
	return 0;
}

int transform_joystick_event_to_mouse(struct input_event* pevent) {
	gs_print("transform_joystick_event_to_mouse");
	Mouse_data data;
	data.type = MOUSE_MOVE_EVENT;
	switch (pevent->code) {
	case ABS_Z:
		if (pevent->value - 0x80 > 0) {
			data.x = 0x00000001;
		} else if (pevent->value - 0x80 < 0) {
			data.x = 0xffffffff;
		} else {
			data.x = 0x00000000;
		}
		break;
	case ABS_RZ:
		if (pevent->value - 0x80 > 0) {
			data.y = 0x00000001;
		} else if (pevent->value - 0x80 < 0) {
			data.y = 0xffffffff;
		} else {
			data.y = 0x00000000;
		}
		break;
	default:
		break;
	}
	injectMouseEvent(&data);
	return 0;
}

int get_rel_hor_position() {
	return l_rel_x - 0x80;
}

int get_rel_ver_position() {
	return l_rel_y - 0x80;
}

int get_rer_hor_position() {
	return r_rel_x - 0x80;
}

int get_rer_ver_position() {
	return r_rel_y - 0x80;
}

