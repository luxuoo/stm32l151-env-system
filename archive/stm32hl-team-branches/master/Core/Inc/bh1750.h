#ifndef __BH1750_H
#define __BH1750_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "i2c.h"

#define BH1750_I2C_ADDR (0x23 << 1)
#define BH1750_I2C_ADDR_ALT (0x5C << 1)

#define BH1750_POWER_DOWN 0x00
#define BH1750_POWER_ON 0x01
#define BH1750_RESET 0x07

#define BH1750_CONTINUOUS_HIGH_RES_MODE 0x10
#define BH1750_CONTINUOUS_HIGH_RES_MODE2 0x11
#define BH1750_CONTINUOUS_LOW_RES_MODE 0x13

#define BH1750_ONE_TIME_HIGH_RES_MODE 0x20
#define BH1750_ONE_TIME_HIGH_RES_MODE2 0x21
#define BH1750_ONE_TIME_LOW_RES_MODE 0x23

typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint8_t addr;
    uint8_t mode;
} BH1750_HandleTypeDef;

uint8_t BH1750_Init(BH1750_HandleTypeDef *hbh1750, I2C_HandleTypeDef *hi2c, uint8_t addr);
uint8_t BH1750_StartMeasurement(BH1750_HandleTypeDef *hbh1750);
uint8_t BH1750_ReadLightLevel(BH1750_HandleTypeDef *hbh1750, float *lightlevel);

#ifdef __cplusplus
}
#endif

#endif
