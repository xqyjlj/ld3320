/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-08-08     xqyjlj       the first version
 */
#ifndef MY_PACKAGES_INC_LD3320_BASE_H_
#define MY_PACKAGES_INC_LD3320_BASE_H_
#include <stdint.h>
#include <rtdevice.h>

#ifdef PKG_USING_LD3320_DEBUG
    #ifdef PKG_USING_LD3320_RECV_REPORT
        #define LD3320_USING_RECV_REPORT
    #endif

    #ifdef PKG_USING_LD3320_FINSH
        #define LD3320_USING_FINSH
    #endif
#endif
#define LD3320_USING_FINSH

void ld3320_write_reg(struct rt_spi_device *device, uint8_t addr, uint8_t data);
uint8_t ld3320_read_reg(struct rt_spi_device *device, uint8_t addr);


#endif /* MY_PACKAGES_INC_LD3320_BASE_H_ */