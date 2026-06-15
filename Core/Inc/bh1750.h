#ifndef __BH1750_H
#define __BH1750_H

#include "stm32l1xx_hal.h"

/* BH1750 I2C address (7-bit: 0x23, left-shifted for HAL: 0x46) */
#define BH1750_ADDR          (0x23 << 1)

/* BH1750 opcodes */
#define BH1750_POWER_ON      0x01
#define BH1750_POWER_OFF     0x00
#define BH1750_RESET         0x07
#define BH1750_CONT_H_RES    0x10   /* Continuous H-Resolution Mode */
#define BH1750_CONT_H_RES2   0x11   /* Continuous H-Resolution Mode 2 */
#define BH1750_CONT_L_RES    0x13   /* Continuous L-Resolution Mode */
#define BH1750_ONE_H_RES     0x20   /* One-Time H-Resolution Mode */
#define BH1750_ONE_H_RES2    0x21   /* One-Time H-Resolution Mode 2 */
#define BH1750_ONE_L_RES     0x23   /* One-Time L-Resolution Mode */

void     BH1750_Init(I2C_HandleTypeDef *hi2c);
void     BH1750_WriteCmd(I2C_HandleTypeDef *hi2c, uint8_t cmd);
uint16_t BH1750_ReadRaw(I2C_HandleTypeDef *hi2c);
float    BH1750_ReadLight(I2C_HandleTypeDef *hi2c);

#endif /* __BH1750_H */
