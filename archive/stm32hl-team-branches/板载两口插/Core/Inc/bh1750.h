#ifndef __BH1750_H
#define __BH1750_H

#include "stm32l1xx_hal.h"
#include <stdint.h>

#define BH1750_ADDR         (0x23 << 1)
#define BH1750_ADDR_ALT     (0x5C << 1)

#define BH1750_POWER_ON     0x01
#define BH1750_RESET        0x07
#define BH1750_CONT_H_RES   0x10

typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint8_t addr;
} BH1750_HandleTypeDef;

void BH1750_Init(BH1750_HandleTypeDef *dev, I2C_HandleTypeDef *hi2c, uint8_t addr);
uint8_t BH1750_ReadLight(BH1750_HandleTypeDef *dev, float *lux);

#endif
