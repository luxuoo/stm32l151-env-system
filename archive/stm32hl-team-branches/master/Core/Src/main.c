#include "main.h"
#include "i2c.h"
#include "gpio.h"
#include "bh1750.h"
#include "aht20.h"
#include "xmf_oled_stm32.h"
#include <stdio.h>

void SystemClock_Config(void);

BH1750_HandleTypeDef hbh1750;
AHT20_HandleTypeDef haht20;

volatile uint32_t system_tick = 0;

void HAL_SYSTICK_Callback(void)
{
    system_tick++;
}

uint32_t millis(void)
{
    return system_tick;
}

void delay(uint32_t ms)
{
    uint32_t start = system_tick;
    while ((system_tick - start) < ms);
}

int main(void)
{
    HAL_Init();

    SystemClock_Config();

    MX_GPIO_Init();
    MX_I2C1_Init();

    hbh1750.hi2c = &hi2c1;
    haht20.hi2c = &hi2c1;

    OLED_Init();
    
    OLED_ShowString(0, 0, "STM32 Sensor Demo");
    OLED_ShowString(0, 2, "Initializing...");
    
    HAL_Delay(1000);
    
    uint8_t bh1750_init = BH1750_Init(&hbh1750, &hi2c1, BH1750_I2C_ADDR);
    uint8_t aht20_init = AHT20_Init(&haht20, &hi2c1);
    
    OLED_Clear();
    
    char bh_stat[16];
    char ah_stat[16];
    snprintf(bh_stat, sizeof(bh_stat), "BH: %d", bh1750_init);
    snprintf(ah_stat, sizeof(ah_stat), "AH: %d", aht20_init);
    
    OLED_ShowString(0, 0, bh_stat);
    OLED_ShowString(0, 2, ah_stat);

    HAL_Delay(2000);
    
    OLED_Clear();
    
    uint32_t last_sensor_read = 0;
    uint8_t led_state = 0;
    
    float temperature = 0.0f;
    float humidity = 0.0f;
    float light = 0.0f;
    
    char buffer[32];

    while (1)
    {
        uint32_t now = millis();
        
        if (now - last_sensor_read >= 2000)
        {
            if (bh1750_init == 0)
            {
                BH1750_StartMeasurement(&hbh1750);
                HAL_Delay(120);
                BH1750_ReadLightLevel(&hbh1750, &light);
            }

            if (aht20_init == 0)
            {
                AHT20_ReadTempHumid(&haht20, &temperature, &humidity);
            }
            
            last_sensor_read = now;
            
            led_state = !led_state;
            HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, led_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
        }
        
        OLED_Clear();
        
        OLED_ShowString(0, 0, "Environment");
        
        snprintf(buffer, sizeof(buffer), "BH: %d AH: %d", bh1750_init, aht20_init);
        OLED_ShowString(0, 2, buffer);
        
        snprintf(buffer, sizeof(buffer), "T:%.1fC", temperature);
        OLED_ShowString(0, 4, buffer);
        
        snprintf(buffer, sizeof(buffer), "H:%.1f%%", humidity);
        OLED_ShowString(0, 6, buffer);
        
        snprintf(buffer, sizeof(buffer), "L:%dLux", (int)light);
        OLED_ShowString(0, 8, buffer);
        
        HAL_Delay(100);
    }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL4;
    RCC_OscInitStruct.PLL.PLLDIV = RCC_PLL_DIV3;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
    {
        Error_Handler();
    }
}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif
