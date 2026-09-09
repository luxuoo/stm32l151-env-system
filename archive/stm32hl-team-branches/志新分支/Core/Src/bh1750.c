#include "bh1750.h"

#define BH1750_TIMEOUT  50  /* ms */

void BH1750_Init(BH1750_HandleTypeDef *dev, I2C_HandleTypeDef *hi2c, uint8_t addr)
{
    dev->hi2c = hi2c;
    dev->addr = addr;

    uint8_t cmd;

    cmd = BH1750_POWER_ON;
    HAL_I2C_Master_Transmit(dev->hi2c, dev->addr, &cmd, 1, BH1750_TIMEOUT);
    HAL_Delay(10);

    cmd = BH1750_RESET;
    HAL_I2C_Master_Transmit(dev->hi2c, dev->addr, &cmd, 1, BH1750_TIMEOUT);
    HAL_Delay(10);

    cmd = BH1750_CONT_H_RES;
    HAL_I2C_Master_Transmit(dev->hi2c, dev->addr, &cmd, 1, BH1750_TIMEOUT);

    /* 第一次测量需要最长 180ms */
    HAL_Delay(180);
}

uint8_t BH1750_ReadLight(BH1750_HandleTypeDef *dev, float *lux)
{
    uint8_t buf[2] = {0};

    HAL_StatusTypeDef st = HAL_I2C_Master_Receive(dev->hi2c, dev->addr, buf, 2, BH1750_TIMEOUT);
    if (st != HAL_OK)
        return 1;

    uint16_t raw = ((uint16_t)buf[0] << 8) | buf[1];
    *lux = (float)raw / 1.2f;
    return 0;
}
