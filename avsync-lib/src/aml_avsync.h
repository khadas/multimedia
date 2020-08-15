/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 * User space AV sync module.
 *
 * Author: song.zhao@amlogic.com
 */
#ifndef AML_AVSYNC_H__
#define AML_AVSYNC_H__

#include <stdint.h>
#include <time.h>

enum sync_mode {
    AV_SYNC_MODE_VMASTER = 0,
    AV_SYNC_MODE_AMASTER = 1,
    AV_SYNC_MODE_PCR_MASTER = 2,
};

typedef uint32_t pts90K;
struct vframe;
typedef void (*free_frame)(struct vframe * frame);

struct vframe {
    /* private user data */
    void *private;
    pts90K pts;
    /* duration of this frame.  0 for invalid value */
    pts90K duration;
    /* free function, will be called when multi frames are
     * toggled in a single VSYNC, on frames not for display.
     * For the last toggled frame, free won't be called. Caller
     * of av_sync_pop_frame() are responsible for free poped frame.
     * For example, if frame 1/2/3 are toggled in a single VSYCN,
     * free() of 1/2 will be called, but free() of 3 won't.
     */
    free_frame free;

    //For internal usage under this line
    /*holding period */
    int hold_period;
};

/* create and attach to kernel session. The returned avsync module will
 * associated with @session_id.
 * Params:
 *   @session_id: unique AV sync session ID to bind audio and video
 *               usually get from kernel driver.
 *   @mode: AV sync mode of enum sync_mode
 *   @start_thres: The start threshold of AV sync module. Set it to 0 for
 *               a default value. For low latency mode, set it to 1. Bigger
 *               value will increase the delay of the first frame shown.
 *               AV sync will start when frame number reached threshold.
 *   @delay: AV sync delay number. The delay of display pipeline.
 *           2 for video planes
 *           1 for osd planes
 *   @vsync_interval: Interval of VSYNC, in uint of 90K.
 * Return:
 *   null for failure, or handle for avsync module.
 */
void* av_sync_create(int session_id,
                     enum sync_mode mode,
                     int start_thres,
                     int delay,
                     pts90K vsync_interval);

void av_sync_destroy(void *sync);

/* Pause/Resume AV sync module.
 * It will return last frame in @av_sync_pop_frame() in pause state
 * Params:
 *   @sync: AV sync module handle
 *   @pause: pause for true, or resume.
 * Return:
 *   0 for OK, or error code
 */
int av_sync_pause(void *sync, bool pause);

/* Push a new frame to AV sync module
 * Params:
 *   @sync: AV sync module handle
 * Return:
 *   0 for OK, or error code
 */
int av_sync_push_frame(void *sync , struct vframe *frame);

/* Pop video frame for next VSYNC. This API should be VSYNC triggerd.
 * Params:
 *   @sync: AV sync module handle
 * Return:
 *   Old frame if current frame is hold
 *   New frame if it is time for a frame toggle.
 *   null if there is no frame to pop out (underrun).
 * */
struct vframe *av_sync_pop_frame(void *sync);

/* notify a change in display refresh rate
 * All AV phase/rate logic will be reset
 * Params:
 *   @sync: AV sync module handle
 *   @vsync_interval: Interval of VSYNC, in uint of 90K.
 */
void av_sync_update_vsync_interval(void *sync, pts90K vsync_interval);

/* set playback speed
 * Currently only work for VMASTER mode
 * Params:
 *   @speed: 1.0 is normal speed. 2.0 is 2x faster. 0.1 is 10x slower
 *           Minimium speed is 0.001
 *           Max speed is 100
 * Return:
 *   0 for OK, or error code
 */
int av_sync_set_speed(void *sync, float speed);

#endif
