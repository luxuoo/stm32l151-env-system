#ifndef __AHT20_H
#define __AHT20_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "i2c.h"

#define AHT20_I2C_ADDR (0x38 << 1)

#define AHT20_CMD_INIT 0xBE
#define AHT20_CMD_INIT_PARAM1 0x08
#define AHT20_CMD_INIT_PARAM2 0x00

#define AHT20_CMD_TRIGGER_MEASURE 0xAC
#define AHT20_CMD_MEASURE_PARAM1 0x33
#define AHT20_CMD_MEASURE_PARAM2 0x00

#define AHT20_CMD_SOFT_RESET 0xBA

typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint8_t addr;
    uint8_t initialized;
} AHT20_HandleTypeDef;

uint8_t AHT20_Init(AHT20_HandleTypeDef *haht20, I2C_HandleTypeDef *hi2c);
uint8_t AHT20_ReadTempHumid(AHT20_HandleTypeDef *haht20, float *temperature, float *humidity);
uint8_t AHT20_SoftReset(AHT20_HandleTypeDef *haht20);

#ifdef __cplusplus
}
#endif

#endif
