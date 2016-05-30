#include <sys/poll.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <fcntl.h>
#include <dirent.h>
#include <utils/Log.h>
#include "inject_jni.h"

#define INPUT_DEVICE_DIR "/dev/input/"
static int touchfd = 0;
static long long last_event_time_ms = 0;

void set_touch_event_time(struct timeval *tv, long long ttt_ms) {
	if (ttt_ms <= last_event_time_ms) {
		ttt_ms = last_event_time_ms + 1;
	}
	last_event_time_ms = ttt_ms;
	tv->tv_sec = (last_event_time_ms / 1000);
	tv->tv_usec = (last_event_time_ms % 1000) * 1000;
}

void injectTouchEvent(Touch_data *ptouch_event, long long ttt_ms) {
	static unsigned short seq = 0;
	int i = 0;
	struct input_event touch_event;
	int action = ptouch_event->action;
	int points = ptouch_event->pointCount;
	int actionIndex = ptouch_event->actionIndex;

	memset(&touch_event, 0, sizeof(touch_event));

	if (touchfd <= 0) {
		D("touch device is not open!!!");
		return;
	}

	D("action: %d,index: %d,count: %d", action, actionIndex, points);
	for (i = 0; i < points; i++) {
		D("cursor: %d,x: %d,y: %d,id: %d", i, ptouch_event->point_coords[i].x,
				ptouch_event->point_coords[i].y,
				ptouch_event->point_coords[i].pointId);
	}

	switch (action & 0xFF) {
	case TOUCH_ACTION_DOWN: {
		D("TOUCH_ACTION_DOWN");
		set_touch_event_time(&touch_event.time, ttt_ms);
		touch_event.type = EV_ABS;
		touch_event.code = ABS_MT_SLOT;
		touch_event.value = ptouch_event->point_coords[0].pointId;
		write(touchfd, &touch_event, sizeof(touch_event));

		touch_event.type = EV_ABS;
		touch_event.code = ABS_MT_TRACKING_ID;
		touch_event.value = seq++;
		write(touchfd, &touch_event, sizeof(touch_event));

		touch_event.type = EV_ABS;
		touch_event.code = ABS_MT_POSITION_X;
		touch_event.value = ptouch_event->point_coords[0].x;
		write(touchfd, &touch_event, sizeof(touch_event));

		touch_event.type = EV_ABS;
		touch_event.code = ABS_MT_POSITION_Y;
		touch_event.value = ptouch_event->point_coords[0].y;
		write(touchfd, &touch_event, sizeof(touch_event));

		touch_event.type = 0;
		touch_event.code = 0;
		touch_event.value = 0;
		write(touchfd, &touch_event, sizeof(touch_event));

		break;
	}
	case ACTION_POINTER_DOWN: {
		D("ACTION_POINTER_DOWN");
		for (i = 0; i < points; i++) {
			set_touch_event_time(&touch_event.time, ttt_ms);
			touch_event.type = EV_ABS;
			touch_event.code = ABS_MT_SLOT;
			touch_event.value = ptouch_event->point_coords[i].pointId;
			write(touchfd, &touch_event, sizeof(touch_event));

			if (actionIndex == i) {
				touch_event.type = EV_ABS;
				touch_event.code = ABS_MT_TRACKING_ID;
				touch_event.value = seq++;
				write(touchfd, &touch_event, sizeof(touch_event));

				touch_event.type = EV_ABS;
				touch_event.code = ABS_MT_POSITION_X;
				touch_event.value = ptouch_event->point_coords[i].x;
				write(touchfd, &touch_event, sizeof(touch_event));

				touch_event.type = EV_ABS;
				touch_event.code = ABS_MT_POSITION_Y;
				touch_event.value = ptouch_event->point_coords[i].y;
				write(touchfd, &touch_event, sizeof(touch_event));

				touch_event.type = 0;
				touch_event.code = 0;
				touch_event.value = 0;
				write(touchfd, &touch_event, sizeof(touch_event));
				continue;
			}

			touch_event.type = EV_ABS;
			touch_event.code = ABS_MT_POSITION_X;
			touch_event.value = ptouch_event->point_coords[i].x;
			write(touchfd, &touch_event, sizeof(touch_event));

			touch_event.type = EV_ABS;
			touch_event.code = ABS_MT_POSITION_Y;
			touch_event.value = ptouch_event->point_coords[i].y;
			write(touchfd, &touch_event, sizeof(touch_event));

			touch_event.type = 0;
			touch_event.code = 0;
			touch_event.value = 0;
			write(touchfd, &touch_event, sizeof(touch_event));
		}
		break;
	}
	case TOUCH_ACTION_MOVE: {
		D("TOUCH_ACTION_MOVE");
		for (i = 0; i < points; i++) {
			set_touch_event_time(&touch_event.time, ttt_ms);
			touch_event.type = EV_ABS;
			touch_event.code = ABS_MT_SLOT;
			touch_event.value = ptouch_event->point_coords[i].pointId;
			write(touchfd, &touch_event, sizeof(touch_event));

			touch_event.type = EV_ABS;
			touch_event.code = ABS_MT_POSITION_X;
			touch_event.value = ptouch_event->point_coords[i].x;
			write(touchfd, &touch_event, sizeof(touch_event));

			touch_event.type = EV_ABS;
			touch_event.code = ABS_MT_POSITION_Y;
			touch_event.value = ptouch_event->point_coords[i].y;
			write(touchfd, &touch_event, sizeof(touch_event));

			touch_event.type = 0;
			touch_event.code = 0;
			touch_event.value = 0;
			write(touchfd, &touch_event, sizeof(touch_event));
		}
		break;
	}
	case ACTION_POINTER_UP:
		D("ACTION_POINTER_UP");
		{
			for (i = 0; i < points; i++) {
				set_touch_event_time(&touch_event.time, ttt_ms);
				touch_event.type = EV_ABS;
				touch_event.code = ABS_MT_SLOT;
				touch_event.value = ptouch_event->point_coords[i].pointId;
				write(touchfd, &touch_event, sizeof(touch_event));

				if (actionIndex == i) {
					touch_event.type = EV_ABS;
					touch_event.code = ABS_MT_TRACKING_ID;
					touch_event.value = -1;
					write(touchfd, &touch_event, sizeof(touch_event));

					touch_event.type = EV_ABS;
					touch_event.code = ABS_MT_POSITION_X;
					touch_event.value = ptouch_event->point_coords[i].x;
					write(touchfd, &touch_event, sizeof(touch_event));

					touch_event.type = EV_ABS;
					touch_event.code = ABS_MT_POSITION_Y;
					touch_event.value = ptouch_event->point_coords[i].y;
					write(touchfd, &touch_event, sizeof(touch_event));

					touch_event.type = 0;
					touch_event.code = 0;
					touch_event.value = 0;
					write(touchfd, &touch_event, sizeof(touch_event));
					continue;
				}

				touch_event.type = EV_ABS;
				touch_event.code = ABS_MT_POSITION_X;
				touch_event.value = ptouch_event->point_coords[i].x;
				write(touchfd, &touch_event, sizeof(touch_event));

				touch_event.type = EV_ABS;
				touch_event.code = ABS_MT_POSITION_Y;
				touch_event.value = ptouch_event->point_coords[i].y;
				write(touchfd, &touch_event, sizeof(touch_event));

				touch_event.type = 0;
				touch_event.code = 0;
				touch_event.value = 0;
				write(touchfd, &touch_event, sizeof(touch_event));
			}
			break;
		}
	case TOUCH_ACTION_UP:
		D("TOUCH_ACTION_UP");
		{
			set_touch_event_time(&touch_event.time, ttt_ms);
			touch_event.type = EV_ABS;
			touch_event.code = ABS_MT_SLOT;
			touch_event.value = ptouch_event->point_coords[0].pointId;
			write(touchfd, &touch_event, sizeof(touch_event));

			touch_event.type = EV_ABS;
			touch_event.code = ABS_MT_TRACKING_ID;
			touch_event.value = -1;
			write(touchfd, &touch_event, sizeof(touch_event));

			touch_event.type = EV_ABS;
			touch_event.code = ABS_MT_POSITION_X;
			touch_event.value = ptouch_event->point_coords[0].x;
			write(touchfd, &touch_event, sizeof(touch_event));

			touch_event.type = EV_ABS;
			touch_event.code = ABS_MT_POSITION_Y;
			touch_event.value = ptouch_event->point_coords[0].y;
			write(touchfd, &touch_event, sizeof(touch_event));

			touch_event.type = 0;
			touch_event.code = 0;
			touch_event.value = 0;
			write(touchfd, &touch_event, sizeof(touch_event));
			break;
		}
	default:
		D("Unknown action:%d", action);
		break;
	}
	touch_event.type = 0;
	touch_event.code = 0;
	touch_event.value = 0;
	write(touchfd, &touch_event, sizeof(touch_event));
}

void injectTouchClose() {
	if (touchfd > 0) {
		close(touchfd);
		touchfd = 0;
	}
}

static char vtouchpath[4096];
int scanDirLocked(const char *dirname) {
	char devname[4096];
	char *filename = NULL;
	DIR *dir = NULL;
	struct dirent *de = NULL;
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

		D("begin check dev = %s\r\n", devname);
		if (0 == checkIfVtouchDevice(devname)) {
			D("find vtouch device, path = %s\r\n", devname);
			strcpy(vtouchpath, devname);
			break;
		}
	}
	closedir(dir);
	return 0;
}

int checkIfVtouchDevice(const char *devicePath) {
	char buffer[80];
	D("Opening device: %s", devicePath);
	int fd = open(devicePath, O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		D("could not open %s, %s\n", devicePath, strerror(errno));
		return -1;
	}
	// Get device name.
	if (ioctl(fd, EVIOCGNAME(sizeof(buffer) - 1), &buffer) < 1) {
		D("could not get device name for %s, %s\n", devicePath,
				strerror(errno));
	} else {
		buffer[sizeof(buffer) - 1] = '\0';
	}
	close(fd);
	if (0 == strcmp("vtouch", buffer)) {
		return 0;
	}
	return -1;

}

void injectTouchOpen() {
	scanDirLocked(INPUT_DEVICE_DIR);
	if (touchfd <= 0) {
		touchfd = open(vtouchpath, O_RDWR);
		if (touchfd <= 0) {
			D("injectTouchOpen:%s failed!!!", vtouchpath);
			return;
		}
		D("injectTouchOpen:%s OK!!!", vtouchpath);
	}
}
