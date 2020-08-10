/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-08-03     xqyjlj       the first version
 */
#ifndef MY_PACKAGES_LD3320_H_
#define MY_PACKAGES_LD3320_H_

#include <rtdevice.h>
#include "ld3320_base.h"
#define LD3320_PIN_NONE                 -1

#define LD3320_MODE_ASR                 0x08
#define LD3320_MODE_MP3                 0x80

#define LD3320_CLK_IN                   22               /* LD3320的芯片输入频率 */

#define LD3320_PLL_11                   (uint8_t)((LD3320_CLK_IN/2.0)-1)

#define LD3320_PLL_MP3_19               0x0f
#define LD3320_PLL_MP3_1B               0x18
#define LD3320_PLL_MP3_1D               (uint8_t)(((90.0*((LD3320_PLL_11)+1))/(LD3320_CLK_IN))-1)

#define LD3320_PLL_ASR_19               (uint8_t)(LD3320_CLK_IN*32.0/(LD3320_PLL_11+1) - 0.51)
#define LD3320_PLL_ASR_1B               0x48
#define LD3320_PLL_ASR_1D               0x1f

#define LD3320_SPEECH_END_POINT         0x22/* 调整语音端点结束时间（吐字间隔时间），参数（0x00~0xC3,单位10MS） */
#define LD3320_SPEECH_START_TIME        0x0f/* 调整语音端点起始时间，参数（0x00~0x30,单位10MS） */
#define LD3320_SPEECH_END_TIME          0x3c/* 调整语音端点结束时间（吐字间隔时间），参数（0x00~0xC3,单位10MS） */
#define LD3320_VOICE_MAX_LENGTH         0x3c/* 最长语音段时间，参数（0x00~0xC3,单位100MS） */
#define LD3320_NOISE_TIME               0x02/* 上电噪声略过，参数（0x00~0xff,单位20MS） */
#define LD3320_MIC_VOL                  0x40/* 调整ADC增益，参数（0x00~0xFF,建议10-60） */

#define LD3320_MAX_COMMAND_LEN          20


/* ld3320端口 */
struct ld3320_port
{
    int wr_pin;
    int irq_pin;
    int rst_pin;
};
typedef struct ld3320_port ld3320_port_t;

typedef void (*asr_over_callback_t)(uint8_t num);/* ld3320 asr识别完毕后的回调函数 */
/* ld3320设备 */
struct ld3320_
{
    ld3320_port_t port;
    struct rt_spi_device *dev;
    void (*asr_over_callback_t)(uint8_t num); /* 回调函数 */
};
typedef struct ld3320_ *ld3320_t;
/* ld3320表头 */
struct ld3320_head
{
    struct ld3320_      obj;
    rt_list_t           node;
};
typedef struct ld3320_head *ld3320_head_t;
/* ld3320命令节点 */
struct ld3320_command
{
    char name[LD3320_MAX_COMMAND_LEN];
    uint8_t num;
    rt_list_t list;
};
typedef struct ld3320_command *ld3320_command_t;


ld3320_head_t ld3320_create(char *spi_dev_name, int wr_pin, int rst_pin, int irq_pin, uint8_t mode);
uint8_t ld3320_asr_start(ld3320_head_t ops);
void ld3320_set_asr_over_callback(ld3320_head_t ops, asr_over_callback_t callback);
void ld3320_run(ld3320_head_t ld3320, uint8_t mode);

void ld3320_addcommand_tonode(ld3320_head_t ops, char *pass, int num);
void ld3320_addcommand_fromnode(ld3320_head_t ops);

#endif /* MY_PACKAGES_LD3320_H_ */
