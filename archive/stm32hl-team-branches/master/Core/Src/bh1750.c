#include "bh1750.h"
#include "stdio.h"

static uint8_t BH1750_SendCommand(BH1750_HandleTypeDef *hbh1750, uint8_t command);

uint8_t BH1750_Init(BH1750_HandleTypeDef *hbh1750, I2C_HandleTypeDef *hi2c, uint8_t addr)
{
    if (hbh1750 == NULL || hi2c == NULL) {
        return 1;
    }

    hbh1750->hi2c = hi2c;
    hbh1750->addr = addr;
    hbh1750->mode = BH1750_CONTINUOUS_HIGH_RES_MODE;

    HAL_Delay(10);

    if (BH1750_SendCommand(hbh1750, BH1750_POWER_ON) != 0) {
        return 2;
    }

    HAL_Delay(10);

    return 0;
}

uint8_t BH1750_StartMeasurement(BH1750_HandleTypeDef *hbh1750)
{
    if (hbh1750 == NULL) {
        return 1;
    }

    return BH1750_SendCommand(hbh1750, hbh1750->mode);
}

uint8_t BH1750_ReadLightLevel(BH1750_HandleTypeDef *hbh1750, float *lightlevel)
{
    uint8_t data[2];
    uint16_t level;

    if (hbh1750 == NULL || lightlevel == NULL) {
        return 1;
    }

    if (HAL_I2C_Master_Receive(hbh1750->hi2c, hbh1750->addr, data, 2, 1000) != HAL_OK) {
        return 2;
    }

    level = ((uint16_t)data[0] << 8) | data[1];
    *lightlevel = (float)level / 1.2f;

    return 0;
}

static uint8_t BH1750_SendCommand(BH1750_HandleTypeDef *hbh1750, uint8_t command)
{
    if (HAL_I2C_Master_Transmit(hbh1750->hi2c, hbh1750->addr, &command, 1, 1000) != HAL_OK) {
        return 1;
    }
    return 0;
}
