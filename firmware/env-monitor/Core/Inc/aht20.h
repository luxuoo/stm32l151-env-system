#ifndef __AHT20_H
#define __AHT20_H

#include "stm32l1xx_hal.h"
#include <stdint.h>

#define AHT20_ADDR          (0x38 << 1)

typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint8_t addr;
} AHT20_HandleTypeDef;

void AHT20_Init(AHT20_HandleTypeDef *dev, I2C_HandleTypeDef *hi2c);
uint8_t AHT20_Read(AHT20_HandleTypeDef *dev, float *temp, float *humi);

#endif
