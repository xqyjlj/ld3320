/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-08-11     xqyjlj       the first version
 */
#include <rtthread.h>
#include "drv_spi.h"
#include "ld3320.h"

#define LD3320_RST GET_PIN(H, 11)
#define LD3320_IRQ GET_PIN(H, 12)

static void ld3320_thread(void * parameter)
{
    static ld3320_t _ld3320;
    _ld3320 = ld3320_create("spi2a", LD3320_PIN_NONE, LD3320_RST, LD3320_IRQ, LD3320_MODE_MP3);
    ld3320_set_mp3_file_path(_ld3320, "/test.mp3");
    ld3320_mp3_start(_ld3320);
    while (1)
    {
        ld3320_run(_ld3320, LD3320_MODE_MP3);
        rt_thread_mdelay(50);
    }
}

static int create_ld3320_thread(void)
{
    rt_thread_t thread = RT_NULL;
    thread = rt_thread_create("ld3320", ld3320_thread, RT_NULL, 500, 15, 100);
    if (thread != RT_NULL)
    {
        rt_thread_startup(thread);
    }
    return RT_EOK;
}

INIT_APP_EXPORT(create_ld3320_thread);
