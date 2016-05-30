#ifndef SIMU_EVENT_H_
#define SIMU_EVENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/input.h>

#include "inject_jni.h"

#define ACTION_DOWN 0x00000000
#define ACTION_UP   0x00000001 
#define ACTION_MOVE 0x00000002

int transform_joystick_event_l_adv(struct input_event* pevent, int jst_x,
		int jst_y, float sensity);
int transform_joystick_event_r_adv(struct input_event* pevent, int jst_x,
		int jst_y, float sensity);
int get_rel_hor_position();
int get_rel_ver_position();
int get_rer_hor_position();
int get_rer_ver_position();
#endif
