/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: tsync warrper. Single instance ONLY. Session will be ignored
 * Author: song.zhao@amlogic.com
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "tsync.h"

#define TSYNC_ENABLE "/sys/class/tsync/enable"
#define TSYNC_PCRSCR "/sys/class/tsync/pts_pcrscr"
#define TSYNC_EVENT  "/sys/class/tsync/event"

static int config_sys_node(const char* path, const char* value)
{
    int fd;
    fd = open(path, O_RDWR);
    if (fd < 0) {
        printf("fail to open %s\n", path);
        return -1;
    }
    if (write(fd, value, strlen(value)) != strlen(value)) {
        printf("fail to write %s to %s\n", value, path);
        close(fd);
        return -1;
    }
    close(fd);

    return 0;
}

static int get_sysfs_uint32(const char *path, uint32_t *value)
{
    int fd;
    char valstr[64];
    uint32_t val = 0;

    fd = open(path, O_RDONLY);
    if (fd >= 0) {
        memset(valstr, 0, 64);
        read(fd, valstr, 64 - 1);
        valstr[strlen(valstr)] = '\0';
        close(fd);
    } else {
        printf("unable to open file %s\n", path);
        return -1;
    }
    if (sscanf(valstr, "0x%x", &val) < 1) {
        printf("unable to get pts from: %s", valstr);
        return -1;
    }
    *value = val;
    return 0;
}

void tsync_enable(int session, bool enable)
{
    const char *val;

    if (enable)
        val = "1";
    else
        val = "0";
    config_sys_node(TSYNC_ENABLE, val);
}

uint32_t tsync_get_pcr(int session)
{
    uint32_t pcr = 0;

    get_sysfs_uint32(TSYNC_PCRSCR, &pcr);
    return pcr;
}

//uint32_t tsync_get_vpts(int session);

int tsync_send_video_start(int session, uint32_t vpts)
{
    char val[50];

    snprintf(val, sizeof(val), "VIDEO_START:0x%x", vpts);
    return config_sys_node(TSYNC_EVENT, val);
}

int tsync_send_video_pause(int session, bool pause)
{
    const char *val;

    if (pause)
        val = "VIDEO_PAUSE:0x1";
    else
        val = "VIDEO_PAUSE:0x0";
    return config_sys_node(TSYNC_EVENT, val);
}

int tsync_send_video_disc(int session, uint32_t vpts)
{
    char val[50];

    snprintf(val, sizeof(val), "VIDEO_TSTAMP_DISCONTINUITY:0x%x", vpts);
    return config_sys_node(TSYNC_EVENT, val);
}
