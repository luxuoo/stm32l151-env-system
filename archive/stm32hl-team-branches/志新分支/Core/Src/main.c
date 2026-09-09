/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
#include "main.h"
#include "i2c.h"
#include "gpio.h"

/* USER CODE BEGIN Includes */
#include "oled.h"
#include "bh1750.h"
#include "aht20.h"
/* USER CODE END Includes */

/* USER CODE BEGIN PV */
BH1750_HandleTypeDef bh1750;
AHT20_HandleTypeDef  aht20;

float lux_val    = 0.0f;
float temp_val   = 0.0f;
float humi_val   = 0.0f;

uint8_t bh1750_ok = 0;
uint8_t aht20_ok  = 0;
uint8_t alarm_flash = 0;
/* USER CODE END PV */

void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void I2C_BusReset(GPIO_TypeDef *scl_port, uint32_t scl_pin,
                         GPIO_TypeDef *sda_port, uint32_t sda_pin);
static void BootAnimation(void);
static void DisplayMain(void);
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

static void I2C_BusReset(GPIO_TypeDef *scl_port, uint32_t scl_pin,
                         GPIO_TypeDef *sda_port, uint32_t sda_pin)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Mode  = GPIO_MODE_OUTPUT_OD;
    gpio.Pull  = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    gpio.Pin = scl_pin;
    HAL_GPIO_Init(scl_port, &gpio);
    gpio.Pin = sda_pin;
    HAL_GPIO_Init(sda_port, &gpio);

    HAL_GPIO_WritePin(scl_port, scl_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(sda_port, sda_pin, GPIO_PIN_SET);

    for (int i = 0; i < 9; i++) {
        HAL_GPIO_WritePin(scl_port, scl_pin, GPIO_PIN_RESET);
        for (volatile int d = 0; d < 50; d++);
        HAL_GPIO_WritePin(scl_port, scl_pin, GPIO_PIN_SET);
        for (volatile int d = 0; d < 50; d++);
    }

    HAL_GPIO_WritePin(sda_port, sda_pin, GPIO_PIN_RESET);
    for (volatile int d = 0; d < 50; d++);
    HAL_GPIO_WritePin(scl_port, scl_pin, GPIO_PIN_SET);
    for (volatile int d = 0; d < 50; d++);
    HAL_GPIO_WritePin(sda_port, sda_pin, GPIO_PIN_SET);
}

static void BootAnimation(void)
{
    /* 1. 屏幕点亮,从中间向上下展开清屏 */
    OLED_Clear();
    OLED_FillRect(0, 31, 128, 2);
    OLED_Display();
    HAL_Delay(60);
    for (uint8_t i = 1; i <= 4; i++) {
        OLED_Clear();
        OLED_FillRect(0, 31 - i * 8, 128, 2 + i * 16);
        OLED_Display();
        HAL_Delay(30);
    }
    OLED_Clear();
    OLED_Display();

    /* 2. 标题逐字打出 */
    const char *title = "SENSOR MONITOR";
    uint8_t tx = 14;
    for (uint8_t i = 0; title[i]; i++) {
        OLED_DrawChar(tx + i * 8, 16, title[i], 8);
        OLED_Display();
        HAL_Delay(40);
    }

    /* 3. 底部版本号淡入(闪烁2次) */
    for (uint8_t b = 0; b < 3; b++) {
        OLED_Printf(36, 48, 8, "v1.0");
        OLED_Display();
        HAL_Delay(120);
        OLED_Clear();
        /* 保留标题 */
        OLED_DrawString(14, 16, title, 8);
        OLED_Display();
        HAL_Delay(80);
    }

    /* 4. 收尾: 标题下划线滑入 */
    OLED_DrawString(14, 16, title, 8);
    for (uint8_t lx = 0; lx <= 100; lx += 4) {
        OLED_DrawHLine(14, 26, lx);
        OLED_Display();
        HAL_Delay(10);
    }
    HAL_Delay(250);

    OLED_Clear();
    OLED_Display();
}

static void DisplayMain(void)
{
    OLED_Clear();

    /* ===== 上半: 温度 / 湿度 双卡片 ===== */
    OLED_DrawRect(0, 0, 63, 20);   /* 温度卡片 */
    OLED_DrawRect(65, 0, 63, 20);  /* 湿度卡片 */

    /* 温度 (手动拆浮点) */
    OLED_Printf(7, 2, 8, "TEMP");
    if (aht20_ok) {
        int t_i = (int)temp_val;
        int t_d = (int)(temp_val * 10) % 10;
        if (t_d < 0) t_d = -t_d;
        OLED_Printf(4, 11, 8, "%d.%d C", t_i, t_d);
    } else {
        OLED_Printf(4, 11, 8, "--.- C");
    }

    /* 湿度 */
    OLED_Printf(76, 2, 8, "HUMI");
    if (aht20_ok) {
        int h_i = (int)humi_val;
        int h_d = (int)(humi_val * 10) % 10;
        OLED_Printf(69, 11, 8, "%d.%d%%", h_i, h_d);
    } else {
        OLED_Printf(69, 11, 8, "--.-%%");
    }

    /* ===== 中间: 光照 ===== */
    OLED_Printf(0, 24, 8, "Light:");
    if (bh1750_ok)
        OLED_Printf(48, 24, 8, "%d lx", (int)lux_val);
    else
        OLED_Printf(48, 24, 8, "----");

    /* 光照条 */
    uint8_t lux_pct = (uint8_t)(lux_val > 2000 ? 100 : lux_val / 2000.0f * 100);
    OLED_FillRect(0, 34, (uint8_t)(lux_pct * 126 / 100), 3);

    /* ===== 分隔线 ===== */
    OLED_DrawHLine(0, 42, 128);

    /* ===== 底部: 传感器状态 + 报警 ===== */
    OLED_Printf(0, 46, 8, "BH:");
    OLED_Printf(18, 46, 8, bh1750_ok ? "OK" : "--");
    OLED_Printf(48, 46, 8, "AHT:");
    OLED_Printf(72, 46, 8, aht20_ok ? "OK" : "--");

    /* 高光照报警 */
    if (lux_val >= 1000.0f && bh1750_ok) {
        alarm_flash = !alarm_flash;
        if (alarm_flash) {
            OLED_Printf(96, 46, 8, "HIGH");
            OLED_InvertPageRect(94, 44, 34, 12);
        }
    }

    OLED_Display();
}
/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */

  /* 等待传感器上电 */
  HAL_Delay(500);

  /* 初始化OLED */
  OLED_Init();
  OLED_Clear();
  OLED_Display();

  /* 开屏动画 */
  BootAnimation();

  /* 初始化传感器(都在I2C1上) */
  BH1750_Init(&bh1750, &hi2c1, BH1750_ADDR);
  AHT20_Init(&aht20, &hi2c1);

  /* 首次读取 */
  bh1750_ok = (BH1750_ReadLight(&bh1750, &lux_val) == 0);
  aht20_ok  = (AHT20_Read(&aht20, &temp_val, &humi_val) == 0);

  /* USER CODE END 2 */

  /* USER CODE BEGIN WHILE */
  uint32_t last_sensor_tick = 0;
  uint32_t last_display_tick = 0;

  while (1)
  {
    uint32_t now = HAL_GetTick();

    /* 传感器读取: 1500ms */
    if (now - last_sensor_tick >= 1500) {
        last_sensor_tick = now;

        if (BH1750_ReadLight(&bh1750, &lux_val) != 0) {
            bh1750_ok = 0;
            HAL_I2C_DeInit(&hi2c1);
            I2C_BusReset(GPIOB, GPIO_PIN_6, GPIOB, GPIO_PIN_7);
            MX_I2C1_Init();
            BH1750_Init(&bh1750, &hi2c1, BH1750_ADDR);
        } else {
            bh1750_ok = 1;
        }

        if (AHT20_Read(&aht20, &temp_val, &humi_val) != 0) {
            aht20_ok = 0;
            HAL_I2C_DeInit(&hi2c1);
            I2C_BusReset(GPIOB, GPIO_PIN_6, GPIOB, GPIO_PIN_7);
            MX_I2C1_Init();
            AHT20_Init(&aht20, &hi2c1);
        } else {
            aht20_ok = 1;
        }
    }

    /* 显示刷新: 200ms */
    if (now - last_display_tick >= 200) {
        last_display_tick = now;
        DisplayMain();
    }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_OFF;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif
