#include "aht20.h"
#include "stdio.h"

static uint8_t AHT20_SendCommand(AHT20_HandleTypeDef *haht20, uint8_t cmd, uint8_t param1, uint8_t param2);

uint8_t AHT20_Init(AHT20_HandleTypeDef *haht20, I2C_HandleTypeDef *hi2c)
{
    if (haht20 == NULL || hi2c == NULL) {
        return 1;
    }

    haht20->hi2c = hi2c;
    haht20->addr = AHT20_I2C_ADDR;
    haht20->initialized = 0;

    HAL_Delay(40);

    if (AHT20_SendCommand(haht20, AHT20_CMD_INIT, AHT20_CMD_INIT_PARAM1, AHT20_CMD_INIT_PARAM2) != 0) {
        return 2;
    }

    HAL_Delay(10);

    haht20->initialized = 1;
    return 0;
}

uint8_t AHT20_ReadTempHumid(AHT20_HandleTypeDef *haht20, float *temperature, float *humidity)
{
    uint8_t cmd[3];
    uint8_t data[6];
    uint32_t sr, st;

    if (haht20 == NULL || temperature == NULL || humidity == NULL) {
        return 1;
    }

    cmd[0] = AHT20_CMD_TRIGGER_MEASURE;
    cmd[1] = AHT20_CMD_MEASURE_PARAM1;
    cmd[2] = AHT20_CMD_MEASURE_PARAM2;

    if (HAL_I2C_Master_Transmit(haht20->hi2c, haht20->addr, cmd, 3, 1000) != HAL_OK) {
        return 2;
    }

    HAL_Delay(80);

    if (HAL_I2C_Master_Receive(haht20->hi2c, haht20->addr, data, 6, 1000) != HAL_OK) {
        return 3;
    }

    if ((data[0] & 0x80) != 0) {
        return 4;
    }

    sr = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | ((uint32_t)data[3] >> 4);
    st = (((uint32_t)data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | ((uint32_t)data[5]);

    *humidity = ((float)sr / 1048576.0f) * 100.0f;

    float temp_raw = ((float)st / 1048576.0f) * 200.0f - 50.0f;
    *temperature = temp_raw;

    return 0;
}

uint8_t AHT20_SoftReset(AHT20_HandleTypeDef *haht20)
{
    if (haht20 == NULL) {
        return 1;
    }

    uint8_t cmd = AHT20_CMD_SOFT_RESET;

    if (HAL_I2C_Master_Transmit(haht20->hi2c, haht20->addr, &cmd, 1, 1000) != HAL_OK) {
        return 2;
    }

    HAL_Delay(20);

    return 0;
}

static uint8_t AHT20_SendCommand(AHT20_HandleTypeDef *haht20, uint8_t cmd, uint8_t param1, uint8_t param2)
{
    uint8_t data[3] = {cmd, param1, param2};

    if (HAL_I2C_Master_Transmit(haht20->hi2c, haht20->addr, data, 3, 1000) != HAL_OK) {
        return 1;
    }

    return 0;
}
