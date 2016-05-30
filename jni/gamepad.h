#ifndef __GAMEPAD_H
#define __GAMEPAD_H
#define DEF_LEN_NAME 256
#define DOWN 0x00000001
#define UP   0x00000000

#define SENSITY_1 0.5
#define SENSITY_2 1
#define SENSITY_3 1.5
#define SENSITY_4 2
#define SENSITY_5 3

typedef struct button_t {
	int x;
	int y;
} button;

typedef struct gamepad_button_map_t {
	button btn_a, btn_b, btn_x, btn_y;
	button btn_l, btn_r, btn_u, btn_d;
	button btn_tl1, btn_tr1, btn_tl2, btn_tr2;
	button btn_thumbl, btn_thumbr;
	int jst_x, jst_y;
	int jsr_x, jsr_y;
} gamepad_button_map;

typedef int (*gamepad_callback_button)(int type, int code, int value,
		gamepad_button_map* gb_map);
typedef int (*gamdpad_callback_joystick)(int type, int code, int value);

#endif
