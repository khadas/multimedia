/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: tsync sysnode wrapper
 * Author: song.zhao@amlogic.com
 */

#ifndef _AML_TSYNC_H_
#define _AML_TSYNC_H_

#include <stdbool.h>
#include <stdint.h>

void tsync_enable(int session, bool enable);
uint32_t  tsync_get_pcr(int session);
//uint32_t  tsync_get_vpts(int session);
int tsync_send_video_start(int session, uint32_t vpts);
int tsync_send_video_pause(int session, bool pause);
int tsync_send_video_disc(int session, uint32_t vpts);

#endif