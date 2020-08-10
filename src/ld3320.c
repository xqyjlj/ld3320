/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-08-03     xqyjlj       the first version
 */
#include "ld3320.h"

#define DBG_TAG "ld3320"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

static rt_sem_t ld3320_sem = RT_NULL;

void rt_hw_us_delay(rt_uint32_t us);

/**
 * Name:    ld3320_soft_rst
 * Brief:   LD3320 软件复位
 * Input:
 *  @ops:   ld3320_head_t句柄
 * Output:  None
 */
static void ld3320_soft_rst(ld3320_head_t ops)
{
    RT_ASSERT(ops);
    ld3320_write_reg(ops->obj.dev, 0x17, 0x35);/* ld3320软复位 */
}
/**
 * Name:    ld3320_init_common
 * Brief:   LD3320 通用初始化设置
 * Input:
 *  @ops:    ld3320_head_t句柄
 *  @mode:      运行模式
 * Output:  None
 */
static void ld3320_init_common(ld3320_head_t ops, uint8_t mode)
{
    RT_ASSERT(ops);
    ld3320_read_reg(ops->obj.dev, 0x06);/* 读fifo状态 */
    ld3320_soft_rst(ops);/* 软件复位 */
    rt_hw_us_delay(10);
    ld3320_read_reg(ops->obj.dev, 0x06);/* 读fifo状态 */

    ld3320_write_reg(ops->obj.dev, 0x89, 0x03);/* 模拟电路控制初始化 */
    rt_hw_us_delay(10);
    ld3320_write_reg(ops->obj.dev, 0xcf, 0x43);/* 内部省电模式初始化 */

    rt_hw_us_delay(10);
    ld3320_write_reg(ops->obj.dev, 0xcb, 0x02);/* 读取ASR结果（候补4） */

    /*PLL setting*/
    ld3320_write_reg(ops->obj.dev, 0x11, LD3320_PLL_11);/* 设置时钟频率1 */
    if (mode == LD3320_MODE_MP3) {
        ld3320_write_reg(ops->obj.dev, 0x1e, 0x00);/* ADC控制初始化 */
        ld3320_write_reg(ops->obj.dev, 0x19, LD3320_PLL_MP3_19);/* 设置时钟频率2 */
        ld3320_write_reg(ops->obj.dev, 0x1b, LD3320_PLL_MP3_1B);/* 设置时钟频率3 */
        ld3320_write_reg(ops->obj.dev, 0x1d, LD3320_PLL_MP3_1D);/* 设置时钟频率4 */
    } else if (mode == LD3320_MODE_ASR) {
        ld3320_write_reg(ops->obj.dev, 0x1e, 0x00);/* ADC控制初始化 */
        ld3320_write_reg(ops->obj.dev, 0x19, LD3320_PLL_ASR_19);/* 设置时钟频率2 */
        ld3320_write_reg(ops->obj.dev, 0x1b, LD3320_PLL_ASR_1B);/* 设置时钟频率3 */
        ld3320_write_reg(ops->obj.dev, 0x1d, LD3320_PLL_ASR_1D);/* 设置时钟频率4 */
    }
    rt_hw_us_delay(1);
    ld3320_write_reg(ops->obj.dev, 0xcd, 0x04);/* 允许DSP休眠 */
    ld3320_write_reg(ops->obj.dev, 0x17, 0x4c);/* DSP休眠 */
    rt_hw_us_delay(10);
    ld3320_write_reg(ops->obj.dev, 0xb9, 0x00);/* ASR字符串长度初始化 */
    ld3320_write_reg(ops->obj.dev, 0xcf, 0x4f);/* 内部省电模式（初始化MP3和ASR） */
    ld3320_write_reg(ops->obj.dev, 0x6f, 0xff);
}
/**
 * Name:    ld3320_init_asr
 * Brief:   初始化LD3320 语音识别模式
 * Input:
 *  @ops:                ld3320_head_t句柄
 * Output:  None
 */
static void ld3320_init_asr(ld3320_head_t ops)
{
    RT_ASSERT(ops);
    ld3320_write_reg(ops->obj.dev, 0xbd, 0x00);/* 初始化ASR控制器 */
    ld3320_write_reg(ops->obj.dev, 0x17, 0x48);/* 激活DSP */
    rt_hw_us_delay(1);
    ld3320_write_reg(ops->obj.dev, 0x3c, 0x80);/* 初始化FIFO EXT */
    ld3320_write_reg(ops->obj.dev, 0x3e, 0x07);/* 初始化FIFO EXT */
    ld3320_write_reg(ops->obj.dev, 0x38, 0xff);/* 初始化FIFO EXT */
    ld3320_write_reg(ops->obj.dev, 0x3a, 0x07);/* 初始化FIFO EXT */
    ld3320_write_reg(ops->obj.dev, 0x40, 0x00);/* 初始化FIFO EXT MCU */
    ld3320_write_reg(ops->obj.dev, 0x42, 0x08);/* 初始化FIFO EXT MCU */
    ld3320_write_reg(ops->obj.dev, 0x44, 0x00);/* 初始化FIFO EXT DSP */
    ld3320_write_reg(ops->obj.dev, 0x46, 0x08);/* 初始化FIFO EXT DSP */
    rt_hw_us_delay(1);
}
/**
 * Name:    ld3320_check_asrbusy_flag_b2
 * Brief:   检查LD3320 b2寄存器的值是否等于0x21，只有等于0x21时，LD3320才是空闲的
 * Input:
 *  @ops:                ld3320_head_t句柄
 * Output:  1：如果LD3320为空闲时
 *          0：LD3320繁忙
 */
static uint8_t ld3320_check_asrbusy_flag_b2(ld3320_head_t ops)
{
    RT_ASSERT(ops);
    uint8_t j;
    uint8_t flag = 0;

    for (j = 0; j < 10; j++) {
        if (ld3320_read_reg(ops->obj.dev, 0xb2) == 0x21) {
            flag = 1;
            break;
        }
        rt_hw_us_delay(10);
    }
    return flag;
}
/**
 * Name:    ld3320_set_speechEndpoint
 * Brief:   调整语音端点结束时间（吐字间隔时间），参数（0x00~0xC3,单位10MS）
 * Input:
 *  @ops:                ld3320_head_t句柄
 *  @speech_endpoint_:      语音端点结束时间
 * Output:  None
 */
static void ld3320_set_speechEndpoint(ld3320_head_t ops, uint8_t speech_endpoint)
{
    RT_ASSERT(ops);
    ld3320_write_reg(ops->obj.dev, 0xb3, speech_endpoint);/* 语音端点检测控制 */
}
/**
 * Name:    ld3320_set_micvol
 * Brief:   调整ADC增益，参数（0x00~0xFF,建议10-60）
 * Input:
 *  @ops:      ld3320_head_t句柄
 *  @micvol:      ADC增益
 * Output:  None
 */
static void ld3320_set_micvol(ld3320_head_t ops, uint8_t micvol)
{
    RT_ASSERT(ops);
    ld3320_write_reg(ops->obj.dev, 0x35, micvol);/* 设置ADC增益 */
}
/**
 * Name:    ld3320_speechStartTime
 * Brief:   调整语音端点起始时间，参数（0x00~0x30,单位10MS）
 * Input:
 *  @ops:                   ld3320_head_t句柄
 *  @speech_start_time_:    语音端点起始时间
 * Output:  None
 */
static void ld3320_set_speechStartTime(ld3320_head_t ops,uint8_t speech_start_time)
{
    RT_ASSERT(ops);
    ld3320_write_reg(ops->obj.dev, 0xb4, speech_start_time);/* 语音端点起始时间 */
}
/**
 * Name:    ld3320_speechEndTime
 * Brief:   调整语音端点结束时间（吐字间隔时间），参数（0x00~0xC3,单位10MS）；
 * Input:
 *  @ops:               ld3320_head_t句柄
 *  speech_end_time:    语音结束时间
 * Output:  None
 */
static void ld3320_set_speechEndTime(ld3320_head_t ops, uint8_t speech_end_time)
{
    RT_ASSERT(ops);
    ld3320_write_reg(ops->obj.dev, 0xb5, speech_end_time);/* 语音结束时间 */
}
/**
 * Name:    ld3320_voiceMaxLength
 * Brief:   最长语音段时间，参数（0x00~0xC3,单位100MS）
 * Input:
 *  @ops:               ld3320_head_t句柄
 *  speech_end_time:    语音结束时间
 * Output:  None
 */
static void ld3320_set_voiceMaxLength(ld3320_head_t ops, uint8_t voice_max_length)
{
    RT_ASSERT(ops);
    ld3320_write_reg(ops->obj.dev, 0xb6, voice_max_length);/* 语音最长时间 */
}
/**
 * Name:    ld3320_noiseTime
 * Brief:   上电噪声略过，参数（0x00~0xff,单位20MS）
 * Input:
 *  @ops:               ld3320_head_t句柄
 *  speech_end_time:    噪声时间
 * Output:  None
 */
static void ld3320_set_noiseTime(ld3320_head_t ops, uint8_t noise_time)
{
    RT_ASSERT(ops);
    ld3320_write_reg(ops->obj.dev, 0xb7, noise_time);/* 噪声时间 */
}

/**
 * Name:    ld3320_asr_start
 * Brief:   启动LD3320语音识别
 * Input:
 *  @ops:    ld3320_head_t句柄
 * Output:  1：启动成功
 *          0：启动失败
 */
uint8_t ld3320_asr_start(ld3320_head_t ops)
{
    RT_ASSERT(ops);
    ld3320_write_reg(ops->obj.dev, 0x1c, 0x09);/* ADC开关控制 保留控制字 */
    ld3320_write_reg(ops->obj.dev, 0xbd, 0x20);/* 控制方式 保留控制字 */
    ld3320_write_reg(ops->obj.dev, 0x08, 0x01);/* 清除FIFO DATA */
    rt_hw_us_delay(1);

    ld3320_write_reg(ops->obj.dev, 0x08, 0x00);/* 清除FIFO后需要再写入一次0x00 */
    rt_hw_us_delay(1);

    if (ld3320_check_asrbusy_flag_b2(ops) == 0) {
        return 0;
    }

    ld3320_write_reg(ops->obj.dev, 0xb2, 0xff);/* DSP忙状态 */
    ld3320_write_reg(ops->obj.dev, 0x37, 0x06);/* 通知ASR开始识别语音 */
    rt_hw_us_delay(5);

    ld3320_write_reg(ops->obj.dev, 0x1c, 0x0b);/* 麦克风输入ADC通道 */
    ld3320_write_reg(ops->obj.dev, 0x29, 0x10);/* FIFO中断允许 */
    ld3320_write_reg(ops->obj.dev, 0xbd, 0x00);/* 启动为ASR模式 */

    return 1;
}
/**
 * Name:    ld3320_addcommand
 * Brief:   往ld3320中加入识别语句
 * Input:
 *  @ops:      ld3320_head_t句柄
 *  @*pass：      语音关键字
 *  @micvol:      关键字标号
 * Output:  None
 */
void ld3320_addcommand(ld3320_head_t ops, char *pass, int num)
{
    RT_ASSERT(ops);
    int i;
    if (ld3320_check_asrbusy_flag_b2(ops) != 1) return;
    ld3320_write_reg(ops->obj.dev, 0xc1, num); /* 字符编号 */
    ld3320_write_reg(ops->obj.dev, 0xc3, 0); /* 添加时输入00 */
    ld3320_write_reg(ops->obj.dev, 0x08, 0x04); /* 清除FIFO_EXT */

    rt_hw_us_delay(1);
    ld3320_write_reg(ops->obj.dev, 0x08, 0x00); /* 清除FIFO后还需写一个0x00 */
    rt_hw_us_delay(1);
    for (i = 0; i <= 80; i++) {
        if (pass[i] == 0) break;
        ld3320_write_reg(ops->obj.dev, 0x5, pass[i]); /* 写入FIFO_EXT */
    }
    ld3320_write_reg(ops->obj.dev, 0xb9, i); /* 写入当前添加字符串长度 */
    ld3320_write_reg(ops->obj.dev, 0xb2, 0xff); /* B2全写ff */
    ld3320_write_reg(ops->obj.dev, 0x37, 0x04); /* 添加语句 */
}
/**
 * Name:    ld3320_addcommand_tonode
 * Brief:   往ld3320链表结构中加入识别语句
 * Input:
 *  @*pass：      语音关键字
 *  @micvol:      关键字标号
 * Output:  None
 */
void ld3320_addcommand_tonode(ld3320_head_t ops, char *pass, int num)
{
    ld3320_command_t node;
    node = (ld3320_command_t) rt_malloc(sizeof(struct ld3320_command));
    if (node == RT_NULL) {
        LOG_E("fail malloc node");
    }
    rt_strncpy(node->name, pass, rt_strlen(pass) + 1);
    node->num = num;
    rt_list_insert_after(&ops->node, &node->list);
}
/**
 * Name:    ld3320_addcommand_fromnode
 * Brief:   将存储在ld3320链表结构的识别语句加入进LD3320中
 * Input:   None
 * Output:  None
 */
void ld3320_addcommand_fromnode(ld3320_head_t ops)
{
    ld3320_command_t tmp;
    rt_list_t  *node;
    node = ops->node.next;
    for (; node != &(ops->node); node = node->next) {
        tmp = rt_list_entry(node, struct ld3320_command, list);
        ld3320_addcommand(ops, tmp->name, tmp->num);
    }
}
/**
 * Name:    ld3320_init_chip
 * Brief:   ld3320 初始化芯片
 * Input:
 * @ops  ld3320_head_t句柄
 * @mode    模式
 * Output:  None
 */
static void ld3320_init_chip(ld3320_head_t ops, uint8_t mode)
{
    RT_ASSERT(ops);

    ld3320_init_common(ops, mode);/* 通用初始化 */

    if (mode == LD3320_MODE_ASR) {
        ld3320_init_asr(ops);/* 初始化ASR模式 */
        ld3320_set_micvol(ops, LD3320_MIC_VOL);
        ld3320_set_speechEndpoint(ops, LD3320_SPEECH_END_POINT);
        ld3320_set_speechStartTime(ops, LD3320_SPEECH_START_TIME);
        ld3320_set_speechEndTime(ops, LD3320_SPEECH_END_TIME);
        ld3320_set_voiceMaxLength(ops, LD3320_VOICE_MAX_LENGTH);
        ld3320_set_noiseTime(ops, LD3320_NOISE_TIME);
    }
}
/**
 * Name:    ld3320_hw_rst
 * Brief:   ld3320硬件复位
 * Input:
 * @ops  ld3320_head_t句柄
 * Output:  None
 */
static void ld3320_hw_rst(ld3320_head_t ops)
{
    RT_ASSERT(ops);

    rt_pin_write(ops->obj.port.rst_pin, PIN_LOW);
    rt_hw_us_delay(10);
    rt_pin_write(ops->obj.port.rst_pin, PIN_HIGH);
    rt_hw_us_delay(10);

}
/**
 * Name:    ld3320_asr_down
 * Brief:   ld3320中断信号来临
 * Input:   None
 * Output:  None
 */
static void ld3320_asr_down(void *asr)
{
    rt_sem_release(ld3320_sem); //释放二值信号量
}
/**
 * Name:    ld3320_create
 * Brief:   创造一个ld3320对象
 * Input:
 *  @spi_dev_name:   spi设备名称
 *  @wr_pin：        WR IO口，如不用设为LD3320_PIN_NONE即可
 *  @rst_pin：       RST IO口，如不用设为LD3320_PIN_NONE即可
 *  @irq_pin：       IRQ IO口
 *  @mode：          LD3320运行模式选择，有LD3320_MODE_ASR与LD3320_MODE_MP3两种选项可供选择
 * Output:  ld3320对象
 */
void ld3320_finsh_init(ld3320_head_t ops);
ld3320_head_t ld3320_create(char *spi_dev_name, int wr, int rst, int irq, uint8_t mode)
{
    RT_ASSERT(spi_dev_name);
    ld3320_head_t ops;
    /* 链表表头初始化 */
    ops = (ld3320_head_t) rt_malloc(sizeof(struct ld3320_head));
    if (ops == RT_NULL) {
        LOG_E("ld3320 head create fail");
        return RT_NULL;
    }
    /* 初始化链表结构 */
    rt_list_init(&ops->node);
    if (!rt_list_isempty(&ops->node)) {
        LOG_E("fail init ld3320 head");
        return RT_NULL;
    }
    /* 查找SPI设备 */
    ops->obj.dev = (struct rt_spi_device *) rt_device_find(spi_dev_name);
    if (ops->obj.dev == RT_NULL) {
        LOG_E("Can't find dev for ld3320 device on '%s' ", spi_dev_name);
        return RT_NULL;
    }
    /* 重新配置SPI模式 */
    struct rt_spi_configuration cfg;
    cfg.data_width = 8;
    cfg.mode = RT_SPI_MASTER | RT_SPI_MODE_2 | RT_SPI_MSB;
    cfg.max_hz = 1 * 1000 * 1000;
    rt_spi_configure(ops->obj.dev, &cfg);
    /* 初始化IO口 */
    ops->obj.port.wr_pin = wr;
    if (ops->obj.port.wr_pin != LD3320_PIN_NONE) {
        rt_pin_mode(ops->obj.port.wr_pin, PIN_MODE_OUTPUT);
        rt_pin_write(ops->obj.port.wr_pin, PIN_LOW);
    }

    ops->obj.port.rst_pin = rst;
    if (ops->obj.port.rst_pin != LD3320_PIN_NONE) {
        rt_pin_mode(ops->obj.port.rst_pin, PIN_MODE_OUTPUT);
        rt_pin_write(ops->obj.port.rst_pin, PIN_HIGH);
    }

    ops->obj.port.irq_pin = irq;
    if (ops->obj.port.irq_pin != LD3320_PIN_NONE) {
        /* 中断模式（下降沿触发） */
        rt_pin_mode(ops->obj.port.irq_pin, PIN_MODE_INPUT_PULLUP);
        rt_pin_attach_irq(ops->obj.port.irq_pin, PIN_IRQ_MODE_FALLING, ld3320_asr_down, RT_NULL);
        rt_pin_irq_enable(ops->obj.port.irq_pin, PIN_IRQ_ENABLE);
        ld3320_sem = rt_sem_create("ld3320", 0, RT_IPC_FLAG_FIFO);/* 二值信号量 */
        if (ld3320_sem == RT_NULL) {
            LOG_E("ld3320 sem create fail");
            return RT_NULL;
        }
    } else {
        LOG_E("please input correct pin");
        return RT_NULL;
    }

    ld3320_hw_rst(ops);/* 硬件复位 */
    ld3320_init_chip(ops, mode);/* 初始化芯片 */
#ifdef LD3320_USING_FINSH
    ld3320_finsh_init(ops);
#endif
    return ops;
}
/**
 * Name:    ld3320_asr_run
 * Brief:   LD3320 ASR模式运行函数
 * Input:
 *  @ops:   ld3320_head_t句柄
 * Output:  None
 */
static void ld3320_asr_run(ld3320_head_t ops)
{
    RT_ASSERT(ops);
    uint8_t Asr_Count = 0;
    uint8_t num = 0;
    if (ops->obj.port.irq_pin != LD3320_PIN_NONE) {
        rt_sem_take(ld3320_sem, RT_WAITING_FOREVER); /* 等待时间：一直等 */
    }
    /* 如果有语音识别中断、DSP闲、ASR正常结束 */
    if ((ld3320_read_reg(ops->obj.dev, 0x2b) & 0x10) && ld3320_read_reg(ops->obj.dev, 0xb2) == 0x21
            && ld3320_read_reg(ops->obj.dev, 0xbf) == 0x35) {
        ld3320_write_reg(ops->obj.dev, 0x29, 0); /* 关中断 */
        ld3320_write_reg(ops->obj.dev, 0x02, 0); /* 关FIFO中断 */
        Asr_Count = ld3320_read_reg(ops->obj.dev, 0xba); /* 读中断辅助信息 */
        /* 如果有识别结果 */
        if (Asr_Count > 0 && Asr_Count < 4) {
            num = ld3320_read_reg(ops->obj.dev, 0xc5);
            ops->obj.asr_over_callback_t(num);
        }
        ld3320_write_reg(ops->obj.dev, 0x2b, 0); /* 清除中断编号 */
        ld3320_write_reg(ops->obj.dev, 0x1C, 0); /* 貌似关麦克风啊~~为毛 */
        ld3320_write_reg(ops->obj.dev, 0x29, 0); /* 关中断 */
        ld3320_write_reg(ops->obj.dev, 0x02, 0); /* 关FIFO中断 */
        ld3320_write_reg(ops->obj.dev, 0x2B, 0);
        ld3320_write_reg(ops->obj.dev, 0xBA, 0);
        ld3320_write_reg(ops->obj.dev, 0xBC, 0);
        ld3320_write_reg(ops->obj.dev, 0x08, 1); /* 清除FIFO_DATA */
        ld3320_write_reg(ops->obj.dev, 0x08, 0); /* 清除FIFO_DATA后 再次写0 */
    }
    ld3320_hw_rst(ops);

    ld3320_init_chip(ops, LD3320_MODE_ASR);

    ld3320_addcommand_fromnode(ops);

    ld3320_asr_start(ops);
}
/**
 * Name:    ld3320_set_asr_over_callback
 * Brief:   设置LD3320语音识别成功后的回调函数
 * Input:
 *  @ops:           ld3320_head_t句柄
 *  @callback:      回调函数
 * Output:  None
 */
void ld3320_set_asr_over_callback(ld3320_head_t ops, asr_over_callback_t callback)
{
    RT_ASSERT(ops);
    if (ops->obj.dev != RT_NULL) {
        ops->obj.asr_over_callback_t = callback;
    } else {
        LOG_W("fail set ld3320 asr over callback");
    }
}
/**
 * @brief   LD3320心跳函数
 * @note    必须循环调用该函数，否则LD3320将不会工作。
 *          可以将其放入线程或定时器中，保证每隔一段时间调用即可
 *          LD3320_TICK_TIME是在"ld3320.h"中定义的宏。
 * */
void ld3320_run(ld3320_head_t ops, uint8_t mode)
{
    RT_ASSERT(ops);
    if (mode == LD3320_MODE_ASR) {
        ld3320_asr_run(ops);
    }
}
