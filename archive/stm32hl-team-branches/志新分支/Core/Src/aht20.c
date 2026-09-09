#include "aht20.h"

#define AHT20_TIMEOUT  100 /* ms */

static uint8_t AHT20_CRC8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++)
            crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
    }
    return crc;
}

/* 读 1 字节状态(发 0x71 失败时,直接 receive 也能拿到当前状态) */
static uint8_t AHT20_ReadStatus(AHT20_HandleTypeDef *dev)
{
    uint8_t status = 0;
    HAL_I2C_Master_Receive(dev->hi2c, dev->addr, &status, 1, AHT20_TIMEOUT);
    return status;
}

void AHT20_Init(AHT20_HandleTypeDef *dev, I2C_HandleTypeDef *hi2c)
{
    dev->hi2c = hi2c;
    dev->addr = AHT20_ADDR;

    /* 上电稳定时间 ≥ 40ms */
    HAL_Delay(50);

    /* 软复位 0xBA,让传感器从任何状态恢复 */
    uint8_t soft_reset = 0xBA;
    HAL_I2C_Master_Transmit(dev->hi2c, dev->addr, &soft_reset, 1, AHT20_TIMEOUT);
    HAL_Delay(25);

    /* 检查校准位 */
    uint8_t status = AHT20_ReadStatus(dev);

    /* bit3 = 1 表示已校准。未校准则发初始化命令 */
    if ((status & 0x08) == 0)
    {
        uint8_t cmd[3] = {0xBE, 0x08, 0x00};
        HAL_I2C_Master_Transmit(dev->hi2c, dev->addr, cmd, 3, AHT20_TIMEOUT);
        HAL_Delay(15);
    }
}

uint8_t AHT20_Read(AHT20_HandleTypeDef *dev, float *temp, float *humi)
{
    uint8_t cmd[3] = {0xAC, 0x33, 0x00};
    uint8_t data[7] = {0};

    /* 触发测量 */
    if (HAL_I2C_Master_Transmit(dev->hi2c, dev->addr, cmd, 3, AHT20_TIMEOUT) != HAL_OK)
        return 1;

    /* 测量典型 80ms,先等够 */
    HAL_Delay(80);

    /* 轮询忙位,最多再等 100ms */
    uint8_t status = 0;
    uint8_t tries = 10;
    while (tries--)
    {
        if (HAL_I2C_Master_Receive(dev->hi2c, dev->addr, &status, 1, AHT20_TIMEOUT) == HAL_OK)
        {
            if ((status & 0x80) == 0) break; /* 不忙了 */
        }
        HAL_Delay(10);
    }
    if (status & 0x80) return 3;

    /* 读完整 7 字节数据 */
    if (HAL_I2C_Master_Receive(dev->hi2c, dev->addr, data, 7, AHT20_TIMEOUT) != HAL_OK)
        return 2;

    /* CRC 校验失败也允许出数据(有些假货 AHT20 模块 CRC 不对),返回 4 但仍解析 */
    uint8_t crc_ok = (AHT20_CRC8(data, 6) == data[6]);

    uint32_t raw_humi = ((uint32_t)data[1] << 12) |
                       ((uint32_t)data[2] << 4)  |
                       ((uint32_t)data[3] >> 4);
    uint32_t raw_temp = (((uint32_t)data[3] & 0x0F) << 16) |
                       ((uint32_t)data[4] << 8) |
                       ((uint32_t)data[5]);

    *humi = ((float)raw_humi / 1048576.0f) * 100.0f;
    *temp = ((float)raw_temp / 1048576.0f) * 200.0f - 50.0f;

    return crc_ok ? 0 : 4;
}
