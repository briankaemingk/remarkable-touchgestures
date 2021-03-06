#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <linux/input.h>
#include "eventreader.h"
#include "ui.h"
#include "config.h"

struct Finger {
	int raw_x;
	int raw_y;
    enum FingerStatus status;
    int track_id;
};
static int width = TOUCH_WIDTH;

static int height = TOUCH_HEIGHT;
static int scr_width = SCREEN_WIDTH;
static int scr_height = SCREEN_HEIGHT;

static struct Finger fingers[MAX_SLOTS+1];

static float adjustY(float y) {
    float pos = height - 1 - y;
    return pos  / height * scr_height;
    
}
static float adjustX(float x) {
#ifdef REMARKABLE1
    float pos = width - 1 - x;
    return pos  / width * scr_width;
#endif
    return x  / width * scr_width;
}
void process_touch(void(*process)(struct TouchEvent *)){
    struct input_event evt;

    int slot = 0;

    memset(fingers,0,sizeof(fingers));

    int touchscreen = open(TOUCHSCREEN, O_RDONLY);
    if (!touchscreen){
        fprintf(stderr, "cannot open touchscreen");
        exit(1);
    }

    //todo: async

    while(read(touchscreen,&evt, sizeof(evt))){
        if (evt.type == EV_ABS) {
            if (evt.code == ABS_MT_SLOT) {
                slot = evt.value;
                if (slot >= MAX_SLOTS){
                    slot = MAX_SLOTS; //sink
                }
                if (fingers[slot].status){
                    fingers[slot].status = Move;
                } 
                else {
                    fingers[slot].status = Down;
                }
            }
            else if (evt.code == ABS_MT_POSITION_X) {
                fingers[slot].raw_x = evt.value;
            }
            else if (evt.code == ABS_MT_POSITION_Y) {

                fingers[slot].raw_y = evt.value;
            } 
            else if (evt.code == ABS_MT_TRACKING_ID && evt.value == -1) {
                printf("slot %d tracking %d\n",slot, evt.value);
                if (fingers[slot].status) {
                    fingers[slot].status = Up;
                }
            } 
            else if (evt.code == ABS_MT_TRACKING_ID) {
                if (slot == 0) {
                    /* printf("TRACK\n"); */
                    fingers[slot].status = Down;
                }
                if (fingers[slot].status) {
                    fingers[slot].track_id = evt.value;
                }
            }
            else {
                /* printf("Uknown %d %d\n", evt.code, evt.value); */
            }
        }
        else if (evt.type == EV_SYN && evt.code == SYN_REPORT) {
            /* printf("SYN slot %d\n", slot); */
            struct Finger *f;
            for (int i=0;i< MAX_SLOTS;i++){
                f = &fingers[i];
                if (f->status) {

					struct timespec tv;
					clock_gettime(CLOCK_MONOTONIC, &tv);
                    struct TouchEvent event;
                    event.time = tv.tv_sec*1000+ tv.tv_nsec /1000000;
                    event.slot = i; //enumerating slots
                    event.x = adjustX(f->raw_x);
                    event.y = adjustY(f->raw_y);
                    event.raw_position.x = f->raw_x;
                    event.raw_position.y = f->raw_y;
                    event.status = f->status;

                    process(&event);

                    if (f->status == Down) {
                        f->status = Move;
                    } 

                    if (f->status == Up){
                        f->status = Untracked;
                    }

                }
            }

        }
    }
	close(touchscreen);
}
