#include "tim.h"
#include "main.h"

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim6;
TIM_HandleTypeDef htim7;

/* =========================================================
 *  TIM2 CH4 PWM — PA3 (LED, low-active)
 *  32 MHz / (PSC+1=32) / (ARR+1=1000) = 1 kHz
 * ========================================================= */
void MX_TIM2_Init(void)
{
    TIM_OC_InitTypeDef sConfigOC = {0};

    htim2.Instance               = TIM2;
    htim2.Init.Prescaler         = 31;        /* 32 MHz / 32 = 1 MHz */
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period            = 999;       /* 1 MHz / 1000 = 1 kHz */
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim2);

    sConfigOC.OCMode     = TIM_OCMODE_PWM1;
    sConfigOC.Pulse      = 999;  /* Start with LED off (low-active) */
    sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;  /* Low-active */
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_4);
}

/* =========================================================
 *  TIM6 Basic — 10 ms dimming tick
 *  32 MHz / (PSC+1=3200) / (ARR+1=100) = 100 Hz → 10 ms
 * ========================================================= */
void MX_TIM6_Init(void)
{
    htim6.Instance               = TIM6;
    htim6.Init.Prescaler         = 3199;
    htim6.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim6.Init.Period            = 99;
    htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_Base_Init(&htim6);
}

/* =========================================================
 *  TIM7 Basic — ~1 second sensor read tick
 *  32 MHz / (PSC+1=32000) / (ARR+1=1000) = 1 Hz → 1 s
 * ========================================================= */
void MX_TIM7_Init(void)
{
    htim7.Instance               = TIM7;
    htim7.Init.Prescaler         = 31999;
    htim7.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim7.Init.Period            = 999;
    htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_Base_Init(&htim7);
}

/* =========================================================
 *  HAL TIM MSP Init (called by HAL_TIM_xxx_Init)
 *  Configures GPIO for TIM2 CH4 and NVIC for TIM6/7
 * ========================================================= */
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (htim->Instance == TIM2) {
        __HAL_RCC_TIM2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* PA3 → TIM2_CH4 (AF1) */
        GPIO_InitStruct.Pin       = GPIO_PIN_3;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6) {
        __HAL_RCC_TIM6_CLK_ENABLE();
        HAL_NVIC_SetPriority(TIM6_IRQn, 3, 0);
        HAL_NVIC_EnableIRQ(TIM6_IRQn);
    }
    else if (htim->Instance == TIM7) {
        __HAL_RCC_TIM7_CLK_ENABLE();
        HAL_NVIC_SetPriority(TIM7_IRQn, 3, 1);
        HAL_NVIC_EnableIRQ(TIM7_IRQn);
    }
}
