#include <sys/poll.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <fcntl.h>
#include "inject_jni.h"

static int mousefd = 0;
#define MOUSE_DEVICE "/dev/input/event4"

int injectMouseEvent(Mouse_data *pmouse_event) {
	if (mousefd <= 0) {
		return -1;
	}

	struct input_event mouse_event;
	int event_type = pmouse_event->type;
	memset(&mouse_event, 0, sizeof(mouse_event));

	switch (event_type) {
	case MOUSE_MOVE_EVENT:
		gettimeofday(&mouse_event.time, NULL);
		mouse_event.type = EV_REL;
		mouse_event.code = REL_X;
		mouse_event.value = pmouse_event->x;
		write(mousefd, &mouse_event, sizeof(mouse_event));
		mouse_event.type = EV_REL;
		mouse_event.code = REL_Y;
		mouse_event.value = pmouse_event->y;
		write(mousefd, &mouse_event, sizeof(mouse_event));
		mouse_event.type = EV_SYN;
		mouse_event.code = SYN_REPORT;
		mouse_event.value = 0;
		write(mousefd, &mouse_event, sizeof(mouse_event));
		break;
	default:
		break;
	}

	return 0;
}

void injectMouseOpen() {
	if (mousefd <= 0) {

		mousefd = open(MOUSE_DEVICE, O_RDWR);
		if (mousefd <= 0) {
			D("injectMouse_Open: %s failed!!!", MOUSE_DEVICE);
			return;
		}

		D("injectMouse_Open: %s OK!", MOUSE_DEVICE);
	}
}

void injectMouseClose() {
	if (mousefd > 0) {
		close(mousefd);
		mousefd = 0;
	}
}

