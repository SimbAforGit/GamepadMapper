#ifndef  __INJECT_H__
#define  __INJECT_H__

#include <pthread.h>
#include <android/log.h>
#define  D(x...)  __android_log_print(ANDROID_LOG_INFO, "INJECT_TAG", x)

//SCREEN TOUCH
#define TOUCH_ACTION_DOWN      0
#define TOUCH_ACTION_UP        1
#define TOUCH_ACTION_MOVE      2
#define ACTION_POINTER_DOWN    5
#define ACTION_POINTER_UP      6

#define ACTION_LS 1
#define ACTION_RS 2
#define ACTION_A 3
#define ACTION_B 4
#define ACTION_X 5
#define ACTION_Y 6
#define ACTION_BTUP 7
#define ACTION_BTDOWN 8
#define ACTION_LEFT 9
#define ACTION_RIGHT 10
#define ACTION_L1 11
#define ACTION_L2 12
#define ACTION_R1 13
#define ACTION_R2 14
#define ACTION_L3 15
#define ACTION_R3 16 

//mouse
#define MOUSE_MOVE_EVENT       1

typedef struct {
	int x;
	int y;
	int pointId;
	int pressure;
	int touchMajor;
	int touchMinor;
	int toolMajor;
	int toolMinor;
	int orientation;
} Pointer_Coords;

typedef struct {
	int action;
	int pointCount;
	int actionIndex;
	Pointer_Coords point_coords[10];
} Touch_data;

typedef struct {
	int pointId;
	int x;
	int y;
} Pointer_info;

typedef struct {
	int type;
	int x;
	int y;
} Mouse_data;
#endif
