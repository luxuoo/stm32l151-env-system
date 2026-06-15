#include "bh1750.h"

/**
  * @brief  Send a command byte to BH1750
  */
void BH1750_WriteCmd(I2C_HandleTypeDef *hi2c, uint8_t cmd)
{
    HAL_I2C_Master_Transmit(hi2c, BH1750_ADDR, &cmd, 1, 100);
}

/**
  * @brief  Initialize BH1750: power on → reset → continuous H-resolution mode
  */
void BH1750_Init(I2C_HandleTypeDef *hi2c)
{
    BH1750_WriteCmd(hi2c, BH1750_POWER_ON);
    BH1750_WriteCmd(hi2c, BH1750_RESET);
    BH1750_WriteCmd(hi2c, BH1750_CONT_H_RES);
    HAL_Delay(180);  /* H-Resolution mode needs ~180ms for first measurement */
}

/**
  * @brief  Read raw 16-bit value from BH1750
  * @retval Raw sensor value
  */
uint16_t BH1750_ReadRaw(I2C_HandleTypeDef *hi2c)
{
    uint8_t buf[2];

    HAL_I2C_Master_Receive(hi2c, BH1750_ADDR, buf, 2, 200);
    return ((uint16_t)buf[0] << 8) | buf[1];
}

/**
  * @brief  Read light intensity from BH1750, returns lux value
  * @retval Light intensity in lux (0~65535)
  */
float BH1750_ReadLight(I2C_HandleTypeDef *hi2c)
{
    uint16_t raw = BH1750_ReadRaw(hi2c);
    return raw / 1.2f;
}
