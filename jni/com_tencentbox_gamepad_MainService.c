#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <poll.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <linux/input.h>

#include <android/log.h>

#include "gamepad.h"
#include "inject_jni.h"
#include "nativehelper/jni.h"
#include "com_tencentbox_gamepad_MainService.h"

#if defined(DEBUG_ENABLED)
#define gs_print(...)  __android_log_print(ANDROID_LOG_INFO, tag, __VA_ARGS__);
#else
#define gs_print(...)
#endif
#define INJECT_CLASSNAME  "com/tencentbox/gamepad/MainService"

extern void injectTouchOpen();
extern void injectMouseOpen();
extern int install_sigaction_handlers();
extern int transform_joystick_event_l_adv(struct input_event* pevent, int jst_x,
		int jst_y, float sensity);
extern int transform_joystick_event_r_adv(struct input_event* pevent, int jst_x,
		int jst_y, float sensity);
extern int transform_joystick_event_to_mouse(struct input_event* pevent);
void* input_event_routine(void* parg);

const char* name_cls_key_event = "android/view/KeyEvent";
const char* name_cls_main_service = "com/tencentbox/gamepad/MainService";
jclass cls_key_event;
static jclass cls_main_service;
static jmethodID event_callback_menu_event_id;
static jmethodID event_callback_key_rs_id;
static jmethodID event_callback_key_a_id;
static JavaVM *g_jvm = NULL;
static jobject g_jobject;

static int native_rs_x = 0;
static int native_rs_y = 0;
enum {
	PRINT_DEVICE_ERRORS = 1U << 0,
	PRINT_DEVICE = 1U << 1,
	PRINT_DEVICE_NAME = 1U << 2,
	PRINT_DEVICE_INFO = 1U << 3,
	PRINT_VERSION = 1U << 4,
	PRINT_POSSIBLE_EVENTS = 1U << 5,
	PRINT_INPUT_PROPS = 1U << 6,
	PRINT_HID_DESCRIPTOR = 1U << 7,
	PRINT_ALL_INFO = (1U << 8) - 1,
	PRINT_LABELS = 1U << 16,
};
//for read the event from device node
static struct pollfd *ufds;
static char **device_names;
static int nfds;
//first we pause the inject operation
static int pauseInjection = 1;
//we designed this only for SparkFox gamepad
const char* const input_device_name = "SparkFox MS-SparkFox";
const char* const input_device_name2 = "Broadcom Bluetooth HID";
const char* const tag = "Gamepad_MainService";
//the current key map
gamepad_button_map cur_btn_map;
//for multi-touch
Pointer_info pointer_info[8];
int ids[8];
//the main thread and thread lock
pthread_t input_event_thread;
pthread_mutex_t game_name_mutex;
//the blocklist for btn
static int block_l3 = 0;
static int block_l2 = 0;
static int block_l1 = 0;
static int block_ls = 0;
static int block_r3 = 0;
static int block_r2 = 0;
static int block_r1 = 0;
static int block_rs = 0;
static int block_up = 0;
static int block_down = 0;
static int block_left = 0;
static int block_right = 0;
static int block_a = 0;
static int block_b = 0;
static int block_x = 0;
static int block_y = 0;

static float ls_sensity = 1;
static float rs_sensity = 1;
static void init_pointer_info() {
	int index = 0;
	for (index = 0; index < 8; index++) {
		pointer_info[index].pointId = -1;
		ids[index] = -1;
	}
}
static void init_figures_directly() {
	memset(&cur_btn_map, 0, sizeof(gamepad_button_map));

	cur_btn_map.jst_x = 133;
	cur_btn_map.jst_y = 524;

	cur_btn_map.jsr_x = 927;
	cur_btn_map.jsr_y = 524;

	cur_btn_map.btn_a.x = 743;
	cur_btn_map.btn_a.y = 400;

	cur_btn_map.btn_b.x = 857;
	cur_btn_map.btn_b.y = 300;

	cur_btn_map.btn_x.x = 637;
	cur_btn_map.btn_x.y = 300;

	cur_btn_map.btn_y.x = 743;
	cur_btn_map.btn_y.y = 192;

	cur_btn_map.btn_l.x = 265;
	cur_btn_map.btn_l.y = 300;

	cur_btn_map.btn_r.x = 461;
	cur_btn_map.btn_r.y = 300;

	cur_btn_map.btn_u.x = 363;
	cur_btn_map.btn_u.y = 400;

	cur_btn_map.btn_d.x = 363;
	cur_btn_map.btn_d.y = 400;

	cur_btn_map.btn_tl1.x = 100;
	cur_btn_map.btn_tl1.y = 190;

	cur_btn_map.btn_tr1.x = 970;
	cur_btn_map.btn_tr1.y = 190;

	cur_btn_map.btn_tl2.x = 100;
	cur_btn_map.btn_tl2.y = 60;

	cur_btn_map.btn_tr2.x = 970;
	cur_btn_map.btn_tr2.y = 60;

	cur_btn_map.btn_thumbl.x = 133;
	cur_btn_map.btn_thumbl.y = 524;

	cur_btn_map.btn_thumbr.x = 927;
	cur_btn_map.btn_thumbr.y = 524;
}
static void reload_figures_directly(int a_x, int a_y, int b_x, int b_y, int x_x,
		int x_y, int y_x, int y_y, int left_x, int left_y, int right_X,
		int right_y, int up_x, int up_y, int down_x, int down_y, int l1_x,
		int l1_y, int l2_x, int l2_y, int r1_x, int r1_y, int r2_x, int r2_y,
		int ls_x, int ls_y, int rs_x, int rs_y, int l3_x, int l3_y, int r3_x,
		int r3_y) {
	memset(&cur_btn_map, 0, sizeof(gamepad_button_map));

	cur_btn_map.jst_x = ls_x;
	cur_btn_map.jst_y = ls_y;

	cur_btn_map.jsr_x = rs_x;
	cur_btn_map.jsr_y = rs_y;

	cur_btn_map.btn_a.x = a_x;
	cur_btn_map.btn_a.y = a_y;

	cur_btn_map.btn_b.x = b_x;
	cur_btn_map.btn_b.y = b_y;

	cur_btn_map.btn_x.x = x_x;
	cur_btn_map.btn_x.y = x_y;

	cur_btn_map.btn_y.x = y_x;
	cur_btn_map.btn_y.y = y_y;

	cur_btn_map.btn_l.x = left_x;
	cur_btn_map.btn_l.y = left_y;

	cur_btn_map.btn_r.x = right_X;
	cur_btn_map.btn_r.y = right_y;

	cur_btn_map.btn_u.x = up_x;
	cur_btn_map.btn_u.y = up_y;

	cur_btn_map.btn_d.x = down_x;
	cur_btn_map.btn_d.y = down_y;

	cur_btn_map.btn_tl1.x = l1_x;
	cur_btn_map.btn_tl1.y = l1_y;

	cur_btn_map.btn_tr1.x = r1_x;
	cur_btn_map.btn_tr1.y = r1_y;

	cur_btn_map.btn_tl2.x = l2_x;
	cur_btn_map.btn_tl2.y = l2_y;

	cur_btn_map.btn_tr2.x = r2_x;
	cur_btn_map.btn_tr2.y = r2_y;

	cur_btn_map.btn_thumbl.x = l3_x;
	cur_btn_map.btn_thumbl.y = l3_y;

	cur_btn_map.btn_thumbr.x = r3_x;
	cur_btn_map.btn_thumbr.y = r3_y;
}
static void reload_blacklist_directly(int a, int b, int x, int y, int left,
		int right, int up, int down, int l1, int l2, int r1, int r2, int ls,
		int rs, int l3, int r3) {
	block_l3 = l3;
	block_l2 = l2;
	block_l1 = l1;
	block_ls = ls;
	block_r3 = r3;
	block_r2 = r2;
	block_r1 = r1;
	block_rs = rs;
	block_up = up;
	block_down = down;
	block_left = left;
	block_right = right;
	block_a = a;
	block_b = b;
	block_x = x;
	block_y = y;
}
static void reload_sensity_directly(int ls_sensity_in, int rs_sensity_in) {
	switch (ls_sensity_in) {
	case 1:
		ls_sensity = SENSITY_1;
		break;
	case 2:
		ls_sensity = SENSITY_2;
		break;
	case 3:
		ls_sensity = SENSITY_3;
		break;
	case 4:
		ls_sensity = SENSITY_4;
		break;
	case 5:
		ls_sensity = SENSITY_5;
		break;
	default:
		break;
	}
	switch (rs_sensity_in) {
	case 1:
		rs_sensity = SENSITY_1;
		break;
	case 2:
		rs_sensity = SENSITY_2;
		break;
	case 3:
		rs_sensity = SENSITY_3;
		break;
	case 4:
		rs_sensity = SENSITY_4;
		break;
	case 5:
		rs_sensity = SENSITY_5;
		break;
	default:
		break;
	}
}
static int open_device(const char *device, int print_flags) {
	int fd;
	struct pollfd *new_ufds;
	char **new_device_names;
	char name[80];

	fd = open(device, O_RDWR);
	if (fd < 0) {
		gs_print("could not open %s, %s\n", device, strerror(errno));
		return -1;
	}

	name[sizeof(name) - 1] = '\0';
	if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
		name[0] = '\0';
	}

	if (!strcasecmp(name, input_device_name)
			|| !strcasecmp(name, input_device_name2)) {
		new_ufds = realloc(ufds, sizeof(ufds[0]) * (nfds + 1));
		if (new_ufds == NULL) {
			fprintf(stderr, "out of memory\n");
			return -1;
		}
		ufds = new_ufds;
		new_device_names = realloc(device_names,
				sizeof(device_names[0]) * (nfds + 1));
		if (new_device_names == NULL) {
			fprintf(stderr, "out of memory\n");
			return -1;
		}
		device_names = new_device_names;

		ufds[nfds].fd = fd;
		ufds[nfds].events = POLLIN;
		device_names[nfds] = strdup(device);
		nfds++;
	}
	return 0;
}
int close_device(const char *device, int print_flags) {
	int i;
	for (i = 1; i < nfds; i++) {
		if (strcmp(device_names[i], device) == 0) {
			int count = nfds - i - 1;
			free(device_names[i]);
			memmove(device_names + i, device_names + i + 1,
					sizeof(device_names[0]) * count);
			memmove(ufds + i, ufds + i + 1, sizeof(ufds[0]) * count);
			nfds--;
			return 0;
		}
	}
	return -1;
}
static int read_notify(const char *dirname, int nfd, int print_flags) {
	int res;
	char devname[PATH_MAX];
	char *filename;
	char event_buf[512];
	int event_size;
	int event_pos = 0;
	struct inotify_event *event;

	res = read(nfd, event_buf, sizeof(event_buf));
	if (res < (int) sizeof(*event)) {
		if (errno == EINTR) {
			return 0;
		}
		fprintf(stderr, "could not get event, %s\n", strerror(errno));
		return 1;
	}

	strcpy(devname, dirname);
	filename = devname + strlen(devname);
	*filename++ = '/';

	while (res >= (int) sizeof(*event)) {
		event = (struct inotify_event *) (event_buf + event_pos);
		if (event->len) {
			strcpy(filename, event->name);
			if (event->mask & IN_CREATE) {
				open_device(devname, print_flags);
			} else {
				close_device(devname, print_flags);
			}
		}
		event_size = sizeof(*event) + event->len;
		res -= event_size;
		event_pos += event_size;
	}
	return 0;
}
static int scan_dir(const char *dirname, int print_flags) {
	char devname[PATH_MAX];
	char *filename;
	DIR *dir;
	struct dirent *de;
	dir = opendir(dirname);
	if (dir == NULL) {
		return -1;
	}

	strcpy(devname, dirname);
	filename = devname + strlen(devname);
	*filename++ = '/';
	while ((de = readdir(dir))) {
		if (de->d_name[0] == '.'
				&& (de->d_name[1] == '\0'
						|| (de->d_name[1] == '.' && de->d_name[2] == '\0'))) {
			continue;
		}
		strcpy(filename, de->d_name);
		open_device(devname, print_flags);
	}

	closedir(dir);
	return 0;
}
static int down_count = 0;
int upallPointer() {
	int i = 0;
	int upindex;
	while (down_count > 0) {
		Touch_data data;
		data.pointCount = down_count;
		int j = 0;
		for (i = 0; i < down_count; i++) {
			if (ids[i] != -1) {
				upindex = i;
				data.point_coords[j].x = pointer_info[i].x;
				data.point_coords[j].y = pointer_info[i].y;
				data.point_coords[j].pointId = pointer_info[i].pointId;
				j++;
			}
		}
		data.actionIndex = j;
		if (down_count == 1) {
			data.action = ACTION_POINTER_UP;
		} else if (down_count > 1) {
			data.action = TOUCH_ACTION_UP;
		}
		injectTouchEvent(&data, 0);
		down_count--;
		ids[upindex] = -1;
	}
	return 0;
}

#define ACTION_DOWN 0x00000000
#define ACTION_UP   0x00000001 
#define ACTION_MOVE 0x00000002

int inject_action(int action_module, int action_type, int x, int y) {
	gs_print("action_module: %d",action_module);
	Touch_data data;
	int index = 0;
	if (pauseInjection == 1) {
		if (down_count == 0) {
			return 0;
		}
	}
	switch (action_type) {
	case ACTION_DOWN:
		if (down_count == 0) {
			data.action = TOUCH_ACTION_DOWN;
		} else {
			data.action = ACTION_POINTER_DOWN;
		}
		down_count++;
		for (index = 0; index < 8; index++) {
			if (ids[index] == -1) {
				ids[index] = action_module;
				pointer_info[index].pointId = index;
				pointer_info[index].x = x;
				pointer_info[index].y = y;
				break;
			}
		}
		break;
	case ACTION_MOVE:
		data.action = TOUCH_ACTION_MOVE;
		for (index = 0; index < 8; index++) {
			if (ids[index] == action_module) {
				pointer_info[index].pointId = index;
				pointer_info[index].x = x;
				pointer_info[index].y = y;
				break;
			}
		}
		break;
	case ACTION_UP:
		if (down_count == 1) {
			data.action = TOUCH_ACTION_UP;
		} else {
			data.action = ACTION_POINTER_UP;
		}
		for (index = 0; index < 8; index++) {
			if (ids[index] == action_module) {

				pointer_info[index].pointId = index;
				pointer_info[index].x = x;
				pointer_info[index].y = y;
				break;
			}
		}
		break;
	default:
		break;
	}

	data.pointCount = down_count;
	int cursor = 0;
	for (index = 0; index < 8; index++) {
		if (ids[index] != -1) {
			if (ids[index] == action_module) {
				data.actionIndex = cursor;
			}
			data.point_coords[cursor].x = pointer_info[index].x;
			data.point_coords[cursor].y = pointer_info[index].y;
			data.point_coords[cursor].pointId = pointer_info[index].pointId;
			cursor++;
		}
	}gs_print("touch data:action: %d,index: %d,count: %d",data.action,data.actionIndex,data.pointCount);
	for (index = 0; index < down_count; index++) {
		gs_print("touch data: cursor: %d,x: %d,y: %d,id: %d",index,data.point_coords[index].x,data.point_coords[index].y,data.point_coords[index].pointId);
	}
	injectTouchEvent(&data, 0);

	if (action_type == ACTION_UP) {
		for (index = 0; index < 8; index++) {
			if (ids[index] == action_module) {
				ids[index] = -1;
				break;
			}
		}
		down_count--;
	}

	return 0;
}

void inject_event_down(int code, int x, int y) {
	if ((x == 0) || (y == 0)) {
		return 0;
	}
	inject_action(code, ACTION_DOWN, x, y);
}

void inject_event_up(int code, int x, int y) {
	if ((x == 0) || (y == 0)) {
		return 0;
	}
	inject_action(code, ACTION_UP, x, y);
}

/*the thread for read event*/
void* input_event_routine(void* parg) {
	int i;
	int res;
	int pollres;
	int print_device = 0;

	struct input_event event;

	int print_flags = 0;
	int print_flags_set = 0;
	const char *device_path = "/dev/input";

	int pre_ABS_HAT0X = 0;
	int pre_ABS_HAT0Y = 0;

	nfds = 1;
	ufds = calloc(1, sizeof(ufds[0]));
	ufds[0].fd = inotify_init();
	ufds[0].events = POLLIN;

	if (!print_flags_set) {
		print_flags |= PRINT_DEVICE_ERRORS | PRINT_DEVICE | PRINT_DEVICE_NAME;
	}
	print_device = 1;

	res = inotify_add_watch(ufds[0].fd, device_path, IN_DELETE | IN_CREATE);
	if (res < 0) {
		return 1;
	}

	res = scan_dir(device_path, print_flags);
	if (res < 0) {
		return 1;
	}

	gs_print("prepare to loop event~\n");
	while (1) {
		pollres = poll(ufds, nfds, -1);
		if (ufds[0].revents & POLLIN) {
			read_notify(device_path, ufds[0].fd, print_flags);
		}
		for (i = 1; i < nfds; i++) {
			if (ufds[i].revents) {
				if (ufds[i].revents & POLLIN) {
					res = read(ufds[i].fd, &event, sizeof(event));
					if (res < (int) sizeof(event)) {
						return 1;
					}

					switch (event.type) {
					case EV_KEY: {
						if (event.value == 0x00000001) {
							switch (event.code) {
							case BTN_A:
								if (block_a == 0)
									inject_event_down(ACTION_A,
											cur_btn_map.btn_a.x,
											cur_btn_map.btn_a.y);
								break;
							case BTN_B:
								if (block_b == 0)
									inject_event_down(ACTION_B,
											cur_btn_map.btn_b.x,
											cur_btn_map.btn_b.y);
								break;
							case BTN_X:
								if (block_x == 0)
									inject_event_down(ACTION_X,
											cur_btn_map.btn_x.x,
											cur_btn_map.btn_x.y);
								break;
							case BTN_Y:
								if (block_y == 0)
									inject_event_down(ACTION_Y,
											cur_btn_map.btn_y.x,
											cur_btn_map.btn_y.y);
								break;
							case BTN_TL:
								if (block_l1 == 0)
									inject_event_down(ACTION_L1,
											cur_btn_map.btn_tl1.x,
											cur_btn_map.btn_tl1.y);
								break;
							case BTN_TR:
								if (block_r1 == 0)
									inject_event_down(ACTION_R1,
											cur_btn_map.btn_tr1.x,
											cur_btn_map.btn_tr1.y);
								break;
							case BTN_TL2:
								if (block_l2 == 0)
									inject_event_down(ACTION_L2,
											cur_btn_map.btn_tl2.x,
											cur_btn_map.btn_tl2.y);
								break;
							case BTN_TR2:
								if (block_r2 == 0)
									inject_event_down(ACTION_R2,
											cur_btn_map.btn_tr2.x,
											cur_btn_map.btn_tr2.y);
								break;
							case BTN_THUMBL:
								if (block_l3 == 0)
									inject_event_down(ACTION_L3,
											cur_btn_map.btn_thumbl.x,
											cur_btn_map.btn_thumbl.y);
								break;
							case BTN_THUMBR:
								if (block_r3 == 0)
									inject_event_down(ACTION_R3,
											cur_btn_map.btn_thumbr.x,
											cur_btn_map.btn_thumbr.y);
								break;
							}
						} else if (event.value == 0x00000000) {
							switch (event.code) {
							case BTN_A:
								if (block_a == 0)
									inject_event_up(ACTION_A,
											cur_btn_map.btn_a.x,
											cur_btn_map.btn_a.y);
								input_key(ACTION_A);
								break;
							case BTN_B:
								if (block_b == 0)
									inject_event_up(ACTION_B,
											cur_btn_map.btn_b.x,
											cur_btn_map.btn_b.y);
								break;
							case BTN_X:
								if (block_x == 0)
									inject_event_up(ACTION_X,
											cur_btn_map.btn_x.x,
											cur_btn_map.btn_x.y);
								break;
							case BTN_Y:
								if (block_y == 0)
									inject_event_up(ACTION_Y,
											cur_btn_map.btn_y.x,
											cur_btn_map.btn_y.y);
								break;
							case BTN_TL:
								if (block_l1 == 0)
									inject_event_up(ACTION_L1,
											cur_btn_map.btn_tl1.x,
											cur_btn_map.btn_tl1.y);
								break;
							case BTN_TR:
								if (block_r1 == 0)
									inject_event_up(ACTION_R1,
											cur_btn_map.btn_tr1.x,
											cur_btn_map.btn_tr1.y);
								break;
							case BTN_TL2:
								if (block_l2 == 0)
									inject_event_up(ACTION_L2,
											cur_btn_map.btn_tl2.x,
											cur_btn_map.btn_tl2.y);
								input_key(ACTION_L2);
								break;
							case BTN_TR2:
								if (block_r2 == 0)
									inject_event_up(ACTION_R2,
											cur_btn_map.btn_tr2.x,
											cur_btn_map.btn_tr2.y);
								break;
							case BTN_THUMBL:
								if (block_l3 == 0)
									inject_event_up(ACTION_L3,
											cur_btn_map.btn_thumbl.x,
											cur_btn_map.btn_thumbl.y);
								input_key(ACTION_L3);
								break;
							case BTN_THUMBR:
								if (block_r3 == 0)
									inject_event_up(ACTION_R3,
											cur_btn_map.btn_thumbr.x,
											cur_btn_map.btn_thumbr.y);
								input_key(ACTION_R3);
								break;
							case BTN_START:
								input_key_menu();
								break;
							}
						}
					}
						break;
					case EV_ABS: {
						switch (event.code) {
						case ABS_HAT0X:
							if (event.value == 0xFFFFFFFF) {
								if (block_left == 0)
									inject_event_down(ACTION_LEFT,
											cur_btn_map.btn_l.x,
											cur_btn_map.btn_l.y);
								pre_ABS_HAT0X = 0;
							} else if (event.value == 0x00000001) {
								if (block_right == 0)
									inject_event_down(ACTION_RIGHT,
											cur_btn_map.btn_r.x,
											cur_btn_map.btn_r.y);
								pre_ABS_HAT0X = 1;
							} else if (event.value == 0x00000000) {
								if (pre_ABS_HAT0X == 0) {
									if (block_left == 0)
										inject_event_up(ACTION_LEFT,
												cur_btn_map.btn_l.x,
												cur_btn_map.btn_l.y);
								} else if (pre_ABS_HAT0X == 1) {
									if (block_right == 0)
										inject_event_up(ACTION_RIGHT,
												cur_btn_map.btn_r.x,
												cur_btn_map.btn_r.y);
								}
							}
							break;
						case ABS_HAT0Y:
							if (event.value == 0xFFFFFFFF) {
								if (block_up == 0)
									inject_event_down(ACTION_BTUP,
											cur_btn_map.btn_u.x,
											cur_btn_map.btn_u.y);
								pre_ABS_HAT0Y = 0;
							} else if (event.value == 0x00000001) {
								if (block_down == 0)
									inject_event_down(ACTION_BTDOWN,
											cur_btn_map.btn_d.x,
											cur_btn_map.btn_d.y);
								pre_ABS_HAT0Y = 1;
							} else if (event.value == 0x00000000) {
								if (pre_ABS_HAT0Y == 0) {
									if (block_up == 0)
										inject_event_up(ACTION_BTUP,
												cur_btn_map.btn_u.x,
												cur_btn_map.btn_u.y);
								} else if (pre_ABS_HAT0Y == 1) {
									if (block_down == 0)
										inject_event_up(ACTION_BTDOWN,
												cur_btn_map.btn_d.x,
												cur_btn_map.btn_d.y);
								}
							}
							break;
						case ABS_X:
						case ABS_Y:
							if (block_ls == 0)
								transform_joystick_event_l_adv(&event,
										cur_btn_map.jst_x, cur_btn_map.jst_y,
										ls_sensity);
							break;
						case ABS_Z:
						case ABS_RZ:
							if (block_rs == 0)
								transform_joystick_event_r_adv(&event,
										cur_btn_map.jsr_x, cur_btn_map.jsr_y,
										rs_sensity);
							input_key_rs(&event);
							break;
						default:
							break;
						}
					}
						break;
					}
				}
			}
		}
	}
	return 0;
}
void input_key_menu() {
	int result = 0;
	JNIEnv *l_env = NULL;
	(*g_jvm)->AttachCurrentThread(g_jvm, &l_env, NULL);
	if (event_callback_menu_event_id != NULL) {
		(*l_env)->CallVoidMethod(l_env, g_jobject,
				event_callback_menu_event_id);
	} gs_print("input_key_menu result is: %d\n", result);
	(*g_jvm)->DetachCurrentThread(g_jvm);
	return 0;
}
void input_key_rs(struct input_event* pevent) {
	int result = 0;
	switch (pevent->code) {
	case ABS_Z:
		native_rs_x = pevent->value;
		break;
	case ABS_RZ:
		native_rs_y = pevent->value;
		break;
	default:
		break;
	}
	JNIEnv *l_env = NULL;
	(*g_jvm)->AttachCurrentThread(g_jvm, &l_env, NULL);
	if (event_callback_key_rs_id != NULL) {
		(*l_env)->CallVoidMethod(l_env, g_jobject, event_callback_key_rs_id, native_rs_x,
				native_rs_y);
	} gs_print("input_key_rs result is: %d\n", result);
	(*g_jvm)->DetachCurrentThread(g_jvm);
	return 0;
}
void input_key(int code) {
	int result = 0;
	JNIEnv *l_env = NULL;
	(*g_jvm)->AttachCurrentThread(g_jvm, &l_env, NULL);
	if (event_callback_key_a_id != NULL) {
		(*l_env)->CallVoidMethod(l_env, g_jobject, event_callback_key_a_id,
				code);
	} gs_print("input_key_menu result is: %d\n", result);
	(*g_jvm)->DetachCurrentThread(g_jvm);
	return 0;
}
JNIEXPORT jboolean JNICALL Java_com_tencentbox_gamepad_MainService_nativeStartService(
		JNIEnv *env, jobject obj) {
	int result = 0;
	gs_print("native start service~\n");
	pthread_mutex_init(&game_name_mutex, NULL);
	injectTouchOpen();
	injectMouseOpen();
	init_figures_directly();
	init_pointer_info();
	result = pthread_create(&input_event_thread, NULL, &input_event_routine,
			NULL);
	assert(result == 0);
	return JNI_TRUE;
}
JNIEXPORT jboolean JNICALL Java_com_tencentbox_gamepad_MainService_nativeSetOperatingMap(
		JNIEnv *env, jobject obj, jstring string, jint a_x, jint a_y, jint b_x,
		jint b_y, jint x_x, jint x_y, jint y_x, jint y_y, jint left_x,
		jint left_y, jint right_X, jint right_y, jint up_x, jint up_y,
		jint down_x, jint down_y, jint l1_x, jint l1_y, jint l2_x, jint l2_y,
		jint r1_x, jint r1_y, jint r2_x, jint r2_y, jint ls_x, jint ls_y,
		jint rs_x, jint rs_y, jint l3_x, jint l3_y, jint r3_x, jint r3_y) {
	pthread_mutex_lock(&game_name_mutex);
	reload_figures_directly(a_x, a_y, b_x, b_y, x_x, x_y, y_x, y_y, left_x,
			left_y, right_X, right_y, up_x, up_y, down_x, down_y, l1_x, l1_y,
			l2_x, l2_y, r1_x, r1_y, r2_x, r2_y, ls_x, ls_y, rs_x, rs_y, l3_x,
			l3_y, r3_x, r3_y);
	pthread_mutex_unlock(&game_name_mutex);
	return JNI_TRUE;
}
JNIEXPORT jboolean JNICALL Java_com_tencentbox_gamepad_MainService_nativeSetBlacklist(
		JNIEnv *env, jobject obj, jstring string, jint a, jint b, jint x,
		jint y, jint left, jint right, jint up, jint down, jint l1, jint l2,
		jint r1, jint r2, jint ls, jint rs, jint l3, jint r3) {
	pthread_mutex_lock(&game_name_mutex);
	reload_blacklist_directly(a, b, x, y, left, right, up, down, l1, l2, r1, r2,
			ls, rs, l3, r3);
	pthread_mutex_unlock(&game_name_mutex);
	return JNI_TRUE;
}
JNIEXPORT jboolean JNICALL Java_com_tencentbox_gamepad_MainService_nativeSetSensity(
		JNIEnv *env, jobject obj, jstring string, jint ls_sensity,
		jint rs_sensity) {
	pthread_mutex_lock(&game_name_mutex);
	reload_sensity_directly(ls_sensity, rs_sensity);
	pthread_mutex_unlock(&game_name_mutex);
	return JNI_TRUE;
}
JNIEXPORT jboolean JNICALL Java_com_tencentbox_gamepad_MainService_nativePauseService(
		JNIEnv *env, jobject obj) {
	gs_print("native pause service~\n");
	pthread_mutex_lock(&game_name_mutex);
	pauseInjection = 1;
	pthread_mutex_unlock(&game_name_mutex);
	return JNI_TRUE;
}
JNIEXPORT jboolean JNICALL Java_com_tencentbox_gamepad_MainService_nativeResumeService(
		JNIEnv *env, jobject obj) {
	gs_print("native resume service~\n");
	pthread_mutex_lock(&game_name_mutex);
	pauseInjection = 0;
	down_count = 0;
	init_pointer_info();
	pthread_mutex_unlock(&game_name_mutex);
	return JNI_TRUE;
}
jboolean mainservice_dynamic_test(JNIEnv *env, jobject obj) {
	return JNI_TRUE;
}

static const JNINativeMethod* methods = { { "dynamic_test", "()Z",
		(void*) mainservice_dynamic_test }, };

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* jvm, void* reserved) {
	jint result = 0;
	JNIEnv* env = NULL;
	gs_print("JNI Library is loaded!\n");
	result = (*jvm)->GetEnv(jvm, (void**) (&env), JNI_VERSION_1_2);
	gs_print("Function GetEnv result is: %d\n", result);
	cls_key_event = (*env)->FindClass(env, name_cls_key_event);
	gs_print("cls_key_event is: %p\n", cls_key_event);
	cls_main_service = (*env)->FindClass(env, name_cls_main_service);
	gs_print("cls_main_service is: %p\n", cls_main_service);
	result = (*env)->RegisterNatives(env, cls_main_service, methods,
			sizeof(methods) / sizeof(methods[0]));
	gs_print("register native methods result is: %d\n", result);
	event_callback_menu_event_id = (*env)->GetStaticMethodID(env,
			cls_main_service, "native_onMenuEvent", "()V");
	event_callback_key_rs_id = (*env)->GetStaticMethodID(env, cls_main_service,
			"native_onKeyRS", "(II)V");
	event_callback_key_a_id = (*env)->GetStaticMethodID(env, cls_main_service,
			"native_onKey", "(I)V");
	g_jvm = jvm;
	g_jobject = (*env)->NewGlobalRef(env, cls_main_service);

	return JNI_VERSION_1_2;
}
JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* jvm, void* reserved) {
	jint result = 0;
	JNIEnv* env = NULL;
	gs_print("JNI Library is unloaded!\n");
	result = (*jvm)->GetEnv(jvm, (void**)(&env), JNI_VERSION_1_2);
	gs_print("Function GetEnv result is: %d\n", result);
	result = (*env)->UnregisterNatives(env, cls_main_service);

}
