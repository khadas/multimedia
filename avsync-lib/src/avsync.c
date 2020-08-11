/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "aml_avsync.h"
#include "queue.h"
#include "pattern.h"
#include "tsync.h"
#include "aml_avsync_log.h"

enum sync_state {
    AV_SYNC_STAT_INIT = 0,
    AV_SYNC_STAT_RUNNING = 1,
    AV_SYNC_STAT_SYNC_SETUP = 2,
    AV_SYNC_STAT_SYNC_LOST = 3,
};

struct  av_sync_session {
    /* session id attached */
    int session_id;
    /* playback time, will stop increasing during pause */
    pts90K stream_time;
    pts90K vpts;

    /* phase adjustment of stream time for rate control */
    pts90K phase;
    bool phase_set;

    /* pts of last rendered frame */
    pts90K last_pts;
    struct vframe *last_frame;

    /* monotonic system time, keep increasing during pause */
    struct timespec system_time;
    bool  first_frame_toggled;
    /* Whether in pause state */
    bool  paused;
    enum sync_mode	mode;
    enum sync_state	state;
    void *pattern_detector;
    void *frame_q;
    /* start threshold */
    int start_thres;

    /* display property */
    int delay;
    pts90K vsync_interval;

    /* state  lock */
    pthread_mutex_t lock;
    /* pattern */
    int last_holding_peroid;
    bool tsync_started;
};

#define MAX_FRAME_NUM 32
#define DEFAULT_START_THRESHOLD 2
#define TIME_UNIT90K    (90000)
#define AV_DISCONTINUE_THREDHOLD_MIN (TIME_UNIT90K * 3)

static bool frame_expire(struct av_sync_session* avsync,
        uint32_t systime,
        struct vframe * frame,
        struct vframe * next_frame,
        int toggle_cnt);
static void pattern_detect(struct av_sync_session* avsync,
        int cur_period,
        int last_period);

void* av_sync_create(int session_id,
        enum sync_mode mode,
        int start_thres,
        int delay, pts90K vsync_interval)
{
    struct av_sync_session *avsync = NULL;

    if (start_thres > 5) {
        log_error("start_thres too big: %d", start_thres);
        return NULL;
    }
    if (delay != 1 && delay != 2) {
        log_error("invalid delay: %d\n", delay);
        return NULL;
    }
    if (vsync_interval < 750 || vsync_interval > 3750) {
        log_error("invalid vsync interval: %d", vsync_interval);
        return NULL;
    }

    avsync = (struct av_sync_session *)calloc(1, sizeof(*avsync));
    if (!avsync) {
        log_error("OOM");
        return NULL;
    }
    avsync->pattern_detector = create_pattern_detector();
    if (!avsync->pattern_detector) {
        log_error("pd create fail");
        free(avsync);
        return NULL;
    }
    avsync->state = AV_SYNC_STAT_INIT;
    avsync->first_frame_toggled = false;
    avsync->paused = false;
    avsync->phase_set = false;
    avsync->session_id = session_id;
    avsync->mode = mode;
    avsync->last_frame = NULL;
    avsync->tsync_started = false;
    if (!start_thres)
        avsync->start_thres = DEFAULT_START_THRESHOLD;
    else
        avsync->start_thres = start_thres;
    avsync->delay = delay;
    avsync->vsync_interval = vsync_interval;

    avsync->frame_q = create_q(MAX_FRAME_NUM);
    if (!avsync->frame_q) {
        log_error("create queue fail");
        destroy_pattern_detector(avsync->pattern_detector);
        free(avsync);
        return NULL;
    }
    //TODO: connect kernel session

    /* just in case sysnode is wrongly set */
    tsync_send_video_pause(avsync->session_id, false);

    pthread_mutex_init(&avsync->lock, NULL);
    log_info("mode: %d start_thres: %d delay: %d interval: %d done\n",
            mode, start_thres, delay, vsync_interval);
    return avsync;
}

static int internal_stop(struct av_sync_session *avsync)
{
    int ret = 0;
    struct vframe *frame;

    pthread_mutex_lock(&avsync->lock);
    if (avsync->state == AV_SYNC_STAT_INIT)
        goto exit;

    while (!dqueue_item(avsync->frame_q, (void **)&frame)) {
        frame->free(frame);
    }

    avsync->state = AV_SYNC_STAT_INIT;
exit:
    pthread_mutex_unlock(&avsync->lock);
    return ret;
}

/* destroy and detach from kernel session */
void av_sync_destroy(void *sync)
{
    struct av_sync_session *avsync = (struct av_sync_session *)sync;

    if (!avsync)
        return;

    log_info("begin");
    if (avsync->state != AV_SYNC_STAT_INIT)
        internal_stop(avsync);

    /* all frames are freed */
    //TODO: disconnect kernel session
    tsync_set_pts_inc_mode(avsync->session_id, false);
    if (avsync->mode == AV_SYNC_MODE_VMASTER)
        tsync_enable(avsync->session_id, false);
    pthread_mutex_destroy(&avsync->lock);
    destroy_q(avsync->frame_q);
    destroy_pattern_detector(avsync->pattern_detector);
    free(avsync);
    log_info("done");
}

int av_sync_pause(void *sync, bool pause)
{
    struct av_sync_session *avsync = (struct av_sync_session *)sync;

    if (!avsync)
        return -1;

    if (avsync->mode == AV_SYNC_MODE_VMASTER) {
        tsync_send_video_pause(avsync->session_id, pause);
        avsync->paused = pause;
        log_info("paused:%d\n", pause);
    } else {
        log_info("ignore paused:%d in mode %d", avsync->mode);
    }

    return 0;
}

int av_sync_push_frame(void *sync , struct vframe *frame)
{
    int ret;
    struct av_sync_session *avsync = (struct av_sync_session *)sync;

    if (!avsync)
        return -1;

    frame->hold_period = 0;
    ret = queue_item(avsync->frame_q, frame);
    if (avsync->state == AV_SYNC_STAT_INIT &&
        queue_size(avsync->frame_q) >= avsync->start_thres) {
        avsync->state = AV_SYNC_STAT_RUNNING;
        log_info("state: init --> running");
    }

    if (ret)
        log_error("%s queue fail:%d", ret);
    return ret;

}

struct vframe *av_sync_pop_frame(void *sync)
{
    struct vframe *frame = NULL;
    struct av_sync_session *avsync = (struct av_sync_session *)sync;
    int toggle_cnt = 0;
    uint32_t systime;

    pthread_mutex_lock(&avsync->lock);
    if (avsync->state == AV_SYNC_STAT_INIT) {
        log_trace("in state INIT");
        goto exit;
    }

    if (!avsync->tsync_started) {
        if (peek_item(avsync->frame_q, (void **)&frame, 0) || !frame) {
            log_info("empty q");
            goto exit;
        }

        if (tsync_enable(avsync->session_id, true))
            log_error("enable tsync fail");
        if (avsync->mode == AV_SYNC_MODE_VMASTER) {
            if (tsync_set_mode(avsync->session_id, AV_SYNC_MODE_VMASTER))
                log_error("set vmaster mode fail");
            if (tsync_set_pcr(avsync->session_id, frame->pts))
                log_error("set pcr fail");
            log_info("update pcr to: %u", frame->pts);
            if (tsync_set_pts_inc_mode(avsync->session_id, true))
                log_error("set inc mode fail");
        } else if (avsync->mode == AV_SYNC_MODE_AMASTER) {
            if (tsync_set_pts_inc_mode(avsync->session_id, false))
                log_error("set inc mode fail");
            if (tsync_set_mode(avsync->session_id, AV_SYNC_MODE_AMASTER))
                log_error("set amaster mode fail");
        } else {
            //PCR master mode should be set alreay, but it won't hurt to set again.
            if (tsync_set_mode(avsync->session_id, AV_SYNC_MODE_PCR_MASTER))
                log_error("set pcrmaster mode fail");
        }

        tsync_set_video_peek_mode(avsync->session_id);
        tsync_disable_video_stop_event(avsync->session_id, true);
        /* video start ASAP */
        tsync_set_video_sync_thres(avsync->session_id, false);
        /* video start event */
        if (tsync_send_video_start(avsync->session_id, frame->pts))
            log_error("send video start fail");
        else
            log_info("video start %u", frame->pts);
        avsync->tsync_started = true;
    }

    systime = tsync_get_pcr(avsync->session_id);
    while (!peek_item(avsync->frame_q, (void **)&frame, 0)) {
        struct vframe *next_frame = NULL;

        peek_item(avsync->frame_q, (void **)&next_frame, 1);
        if (next_frame)
            log_debug("cur_f %u next_f %u", frame->pts, next_frame->pts);
        if (frame_expire(avsync, systime, frame, next_frame, toggle_cnt)) {
            log_debug("cur_f %u expire", frame->pts);
            toggle_cnt++;

            pattern_detect(avsync,
                    (avsync->last_frame?avsync->last_frame->hold_period:0),
                    avsync->last_holding_peroid);
            if (avsync->last_frame)
                avsync->last_holding_peroid = avsync->last_frame->hold_period;

            dqueue_item(avsync->frame_q, (void **)&frame);
            if (avsync->last_frame) {
                /* free frame that are not for display */
                if (toggle_cnt > 1)
                    avsync->last_frame->free(avsync->last_frame);
            } else {
                avsync->first_frame_toggled = true;
                log_info("first frame %u", frame->pts);
            }
            avsync->last_frame = frame;
        } else
            break;
    }

exit:
    pthread_mutex_unlock(&avsync->lock);
    if (avsync->last_frame)
        log_debug("pop %u", avsync->last_frame->pts);
    else
        log_debug("pop (nil)");
    if (avsync->last_frame)
        avsync->last_frame->hold_period++;
    return avsync->last_frame;
}

void av_sync_update_vsync_interval(void *sync, pts90K vsync_interval)
{
    struct av_sync_session *avsync = (struct av_sync_session *)sync;

    pthread_mutex_lock(&avsync->lock);
    avsync->vsync_interval = vsync_interval;
    if (avsync->state >= AV_SYNC_STAT_RUNNING) {
        reset_pattern(avsync->pattern_detector);
        avsync->phase_set = false;
    }
    pthread_mutex_unlock(&avsync->lock);
}

static inline uint32_t abs_diff(uint32_t a, uint32_t b)
{
    return a > b ? a - b : b - a;
}

static bool frame_expire(struct av_sync_session* avsync,
        uint32_t systime,
        struct vframe * frame,
        struct vframe * next_frame,
        int toggle_cnt)
{
    uint32_t fpts = frame->pts;
    bool expire = false;
    uint32_t pts_correction = avsync->delay * avsync->vsync_interval;

    if (!fpts) {
        if (avsync->last_frame) {
            /* try to accumulate duration as PTS */
            fpts = avsync->vpts + avsync->last_frame->duration;
        } else {
            fpts = avsync->vpts;
        }
    }
    systime += pts_correction;

    /* phase adjustment */
    if (avsync->phase_set)
        systime += avsync->phase;

    log_trace("systime:%u phase:%u correct:%u", systime,
            avsync->phase_set?avsync->phase:0, pts_correction);
    if (abs_diff(systime, fpts) > AV_DISCONTINUE_THREDHOLD_MIN &&
            avsync->first_frame_toggled) {
        /* ignore discontinity under pause */
        if (avsync->paused && avsync->mode != AV_SYNC_MODE_PCR_MASTER)
            return false;

        log_warn("sync lost systime:%x fpts:%x", systime, fpts);
        avsync->state = AV_SYNC_STAT_SYNC_LOST;
        avsync->phase_set = false;
        if ((int)(systime - fpts) > 0) {
            if (frame->pts && avsync->mode == AV_SYNC_MODE_VMASTER)
                tsync_send_video_disc(avsync->session_id, frame->pts);
            /*catch up PCR */
            return true;
        } else if (avsync->mode == AV_SYNC_MODE_PCR_MASTER) {
            if (frame->pts)
                tsync_send_video_disc(avsync->session_id, frame->pts);
            else {
                tsync_send_video_disc(avsync->session_id, fpts);
                return true;
            }
        }
    }

    expire = (int)(systime - fpts) >= 0;

    /* scatter the frame in different vsync whenever possible */
    if (expire && next_frame && next_frame->pts && toggle_cnt) {
        /* multi frame expired in current vsync but no frame in next vsync */
        if (systime + avsync->vsync_interval < next_frame->pts) {
            expire = false;
            frame->hold_period++;
            log_debug("unset expire systime:%d inter:%d next_pts:%d toggle_cnt:%d",
                    systime, avsync->vsync_interval, next_frame->pts, toggle_cnt);
        }
    } else if (!expire && next_frame && next_frame->pts && !toggle_cnt
               && avsync->first_frame_toggled) {
        /* next vsync will have at least 2 frame expired */
        if (systime + avsync->vsync_interval > next_frame->pts) {
            expire = true;
            log_debug("set expire systime:%d inter:%d next_pts:%d",
                    systime, avsync->vsync_interval, next_frame->pts);
        }
    }

    correct_pattern(avsync->pattern_detector, frame, next_frame,
            (avsync->last_frame?avsync->last_frame->hold_period:0),
            avsync->last_holding_peroid, systime,
            avsync->vsync_interval, &expire);

    if (expire) {
        avsync->vpts = fpts;
        /* phase adjustment */
        if (!avsync->phase_set) {
            uint32_t phase_thres = avsync->vsync_interval / 4;
            //systime = tsync_get_pcr(avsync->session_id);
            if ( systime > fpts && (systime - fpts) < phase_thres) {
                /* too aligned to current VSYNC, separate them to 1/4 VSYNC */
                avsync->phase += phase_thres - (systime - fpts);
                avsync->phase_set = true;
                log_info("adjust phase to %d", avsync->phase);
            }
            if (!avsync->phase_set && systime > fpts &&
                systime < (fpts + avsync->vsync_interval) &&
                (systime - fpts) > avsync->vsync_interval - phase_thres) {
                /* too aligned to previous VSYNC, separate them to 1/4 VSYNC */
                avsync->phase += phase_thres + fpts + avsync->vsync_interval - systime;
                avsync->phase_set = true;
                log_info("adjust phase to %d", avsync->phase);
            }
        }

        if (avsync->state != AV_SYNC_STAT_SYNC_SETUP)
            log_info("sync setup");
        avsync->state = AV_SYNC_STAT_SYNC_SETUP;
    }

    return expire;
}

static void pattern_detect(struct av_sync_session* avsync, int cur_period, int last_period)
{
    log_trace("cur_period: %d last_period: %d", cur_period, last_period);
    detect_pattern(avsync->pattern_detector, AV_SYNC_FRAME_P32, cur_period, last_period);
    detect_pattern(avsync->pattern_detector, AV_SYNC_FRAME_P22, cur_period, last_period);
    detect_pattern(avsync->pattern_detector, AV_SYNC_FRAME_P41, cur_period, last_period);
}
