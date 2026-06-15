#include "app.h"
#include "bh1750.h"
#include "XMF_OLED_STM32Cube.h"
#include "i2c.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

/* ---- Private variables ---- */
static char oled_buf[32];
static char uart_buf[48];
static volatile uint32_t last_btn_tick = 0;
static float  g_current_lux = 0.0f;

/* =========================================================
 *  UART DMA transmit helper
 * ========================================================= */
void UART_SendString(const char *str)
{
    uint16_t len = strlen(str);
    if (len == 0) return;
    /* Wait previous DMA to finish (max 50ms) */
    uint32_t tick = HAL_GetTick();
    while (g_uart_busy && (HAL_GetTick() - tick < 50));
    g_uart_busy = 1;
    HAL_UART_Transmit_DMA(&huart1, (uint8_t *)str, len);
}

/* =========================================================
 *  I2C2 bus recovery (bit-bang SCL to free stuck SDA)
 * ========================================================= */
void I2C2_Recover(void)
{
    /* Disable I2C2 peripheral */
    __HAL_RCC_I2C2_CLK_DISABLE();
    HAL_Delay(10);
    __HAL_RCC_I2C2_CLK_ENABLE();

    /* Toggle SCL (PB10) as GPIO to clock out stuck slave */
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin  = GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);

    for (int i = 0; i < 9; i++) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
        HAL_Delay(1);
    }

    /* Re-init I2C2 peripheral */
    gpio.Pin  = GPIO_PIN_10 | GPIO_PIN_11;
    gpio.Mode = GPIO_MODE_AF_OD;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF4_I2C2;
    HAL_GPIO_Init(GPIOB, &gpio);

    MX_I2C2_Init();
    HAL_Delay(10);
}

/* =========================================================
 *  BH1750 read with error recovery
 * ========================================================= */
void Sensor_Read(void)
{
    uint16_t raw;
    HAL_StatusTypeDef ret;
    uint8_t buf[2];

    /* Try to read 2 bytes from BH1750 */
    ret = HAL_I2C_Master_Receive(&hi2c2, BH1750_ADDR, buf, 2, 200);

    if (ret == HAL_OK) {
        raw = ((uint16_t)buf[0] << 8) | buf[1];
        g_current_lux = (float)raw / 1.2f;

        if (!g_sensor_ok) {
            /* Sensor recovered */
            g_sensor_ok = 1;
            if (g_sys_state == STATE_ERROR) {
                g_sys_state = STATE_RUNNING;
            }
        }
    } else {
        /* Communication failed */
        g_sensor_ok = 0;
        g_current_lux = 0.0f;

        if (g_sys_state == STATE_RUNNING) {
            g_sys_state = STATE_ERROR;
            /* Force LED off */
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, htim2.Init.Period);
            UART_SendString("SENSOR_DISCONNECT\r\n");
        }

        /* Attempt I2C bus recovery */
        I2C2_Recover();
    }
}

/* =========================================================
 *  Dimming algorithm (called every 10ms from TIM6 ISR flag)
 *  LED is LOW-active: CCR=0 → full bright, CCR=Period → off
 * ========================================================= */
void Dimming_Algorithm(void)
{
    uint32_t period = htim2.Init.Period;  /* 999 */
    uint16_t target_raw;

    if (!g_sensor_ok || g_sys_state != STATE_RUNNING) {
        target_raw = period;  /* LED off */
    } else if (g_current_lux >= 300.0f) {
        target_raw = period;  /* Bright enough, LED off */
    } else {
        /* Darker → brighter LED */
        /* 0 lux → CCR=0 (full bright), 300 lux → CCR=999 (off) */
        float ratio = g_current_lux / 300.0f;
        if (ratio > 1.0f) ratio = 1.0f;
        target_raw = (uint16_t)(period * (1.0f - ratio));
    }

    /* First-order lag filter: α=0.05 */
    float alpha = 0.05f;
    g_smooth_lux = g_smooth_lux + alpha * ((float)target_raw - g_smooth_lux);

    uint16_t ccr = (uint16_t)g_smooth_lux;
    if (ccr > period) ccr = period;
    g_pwm_target = ccr;

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, g_pwm_target);
}

/* =========================================================
 *  Menu screen (STATE_IDLE)
 * ========================================================= */
void Menu_Draw(void)
{
    OLED_Clear();
    OLED_ShowString(0, 0, (unsigned char *)"BH1750 Reader");

    if (g_menu_cursor == 0) {
        OLED_ShowString(0, 2, (unsigned char *)"-> SYS START");
        OLED_ShowString(0, 4, (unsigned char *)"   SYS OFF  ");
    } else {
        OLED_ShowString(0, 2, (unsigned char *)"   SYS START");
        OLED_ShowString(0, 4, (unsigned char *)"-> SYS OFF  ");
    }
}

/* =========================================================
 *  Running/Error screen (STATE_RUNNING / STATE_ERROR)
 * ========================================================= */
void Running_Draw(void)
{
    if (g_sys_state == STATE_ERROR) {
        OLED_Clear();
        OLED_ShowString(0, 0, (unsigned char *)"NO DATA     ");
        OLED_ShowString(0, 2, (unsigned char *)"LED: OFF    ");
        return;
    }

    /* Format lux with left-align: %-8.1f → integer approach */
    uint16_t lux_x10 = (uint16_t)((float)g_current_lux * 10.0f + 0.5f);
    uint16_t lux_int = lux_x10 / 10;
    uint16_t lux_dec = lux_x10 % 10;
    sprintf(oled_buf, "%d.%d       ", lux_int, lux_dec);

    OLED_ShowString(0, 0, (unsigned char *)"Light:      ");
    OLED_ShowString(0, 2, (unsigned char *)oled_buf);
    OLED_ShowString(0, 4, (unsigned char *)"Lux         ");

    /* LED status */
    if (g_current_lux < 300.0f && g_sensor_ok) {
        OLED_ShowString(0, 6, (unsigned char *)"LED: ON     ");
    } else {
        OLED_ShowString(0, 6, (unsigned char *)"LED: OFF    ");
    }
}

/* =========================================================
 *  App_Init: called after all MX peripheral inits
 * ========================================================= */
void App_Init(void)
{
    g_sys_state   = STATE_IDLE;
    g_menu_cursor = 0;
    g_need_redraw = 1;
    g_read_flag   = 0;
    g_dim_flag    = 0;
    g_uart_busy   = 0;
    g_smooth_lux  = (float)htim2.Init.Period;  /* Start with LED off */
    g_pwm_target  = htim2.Init.Period;
    g_sensor_ok   = 0;

    /* Initialize OLED */
    OLED_Init();
    OLED_Clear();
    OLED_ShowString(0, 0, (unsigned char *)"BH1750 System");
    OLED_ShowString(0, 2, (unsigned char *)"Ready...");
    HAL_Delay(800);

    /* Initial BH1750 power-on test */
    BH1750_Init(&hi2c2);
    HAL_Delay(200);

    g_need_redraw = 1;
}

/* =========================================================
 *  App_Loop: runs in main while(1)
 * ========================================================= */
void App_Loop(void)
{
    /* ---- Handle state transitions from buttons ---- */
    /* (Buttons are handled in EXTI callback, state changes are immediate) */

    /* ---- Periodic sensor read (TIM7 flag) ---- */
    if (g_read_flag && g_sys_state == STATE_RUNNING) {
        g_read_flag = 0;
        Sensor_Read();
        g_need_redraw = 1;
    }

    /* ---- Dimming algorithm (TIM6 flag) ---- */
    if (g_dim_flag) {
        g_dim_flag = 0;
        if (g_sys_state == STATE_RUNNING || g_sys_state == STATE_ERROR) {
            Dimming_Algorithm();
        }
    }

    /* ---- OLED refresh ---- */
    if (g_need_redraw) {
        g_need_redraw = 0;

        switch (g_sys_state) {
        case STATE_IDLE:
            Menu_Draw();
            break;
        case STATE_RUNNING:
        case STATE_ERROR:
            Running_Draw();
            break;
        case STATE_OFF:
            /* Screen off, nothing to draw */
            break;
        }
    }
}

/* =========================================================
 *  Button handling (called from EXTI callback)
 * ========================================================= */
static void Handle_ButtonPress(uint16_t gpio_pin)
{
    uint32_t now = HAL_GetTick();
    if (now - last_btn_tick < 200) return;  /* Debounce */
    last_btn_tick = now;

    if (g_sys_state == STATE_IDLE) {
        /* ---- Menu navigation ---- */
        if (gpio_pin == BTN_UP_Pin) {
            if (g_menu_cursor > 0) g_menu_cursor--;
            g_need_redraw = 1;
        }
        else if (gpio_pin == BTN_DOWN_Pin) {
            if (g_menu_cursor < 1) g_menu_cursor++;
            g_need_redraw = 1;
        }
        else if (gpio_pin == BTN_OK_Pin) {
            if (g_menu_cursor == 0) {
                /* SYS START */
                g_sys_state = STATE_RUNNING;
                g_smooth_lux = (float)htim2.Init.Period;
                g_sensor_ok = 0;
                g_read_flag = 1;  /* Read immediately */

                BH1750_Init(&hi2c2);
                HAL_TIM_Base_Start_IT(&htim6);
                HAL_TIM_Base_Start_IT(&htim7);
                HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
                __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, htim2.Init.Period);

                OLED_Clear();
                UART_SendString("SYSTEM_START\r\n");
            } else {
                /* SYS OFF */
                g_sys_state = STATE_OFF;

                BH1750_WriteCmd(&hi2c2, BH1750_POWER_OFF);
                HAL_TIM_Base_Stop_IT(&htim6);
                HAL_TIM_Base_Stop_IT(&htim7);
                HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_4);
                OLED_Clear();
                OLED_Display_Off();

                UART_SendString("SYSTEM_OFF\r\n");
            }
            g_need_redraw = 1;
        }
    }
    else if (g_sys_state == STATE_RUNNING || g_sys_state == STATE_ERROR) {
        /* ---- In running mode, DOWN = back to menu ---- */
        if (gpio_pin == BTN_DOWN_Pin || gpio_pin == BTN_OK_Pin) {
            g_sys_state = STATE_IDLE;

            BH1750_WriteCmd(&hi2c2, BH1750_POWER_OFF);
            HAL_TIM_Base_Stop_IT(&htim6);
            HAL_TIM_Base_Stop_IT(&htim7);
            HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_4);

            OLED_Display_On();
            OLED_Clear();
            UART_SendString("SYSTEM_OFF\r\n");
            g_need_redraw = 1;
        }
    }
    else if (g_sys_state == STATE_OFF) {
        /* ---- Wake up from OFF ---- */
        if (gpio_pin == BTN_OK_Pin) {
            OLED_Display_On();
            g_sys_state = STATE_IDLE;
            g_need_redraw = 1;
        }
    }
}

/* =========================================================
 *  HAL EXTI Callback (overrides weak)
 * ========================================================= */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == BTN_UP_Pin || GPIO_Pin == BTN_DOWN_Pin || GPIO_Pin == BTN_OK_Pin) {
        Handle_ButtonPress(GPIO_Pin);
    }
}

/* =========================================================
 *  HAL TIM Period Elapsed Callback (overrides weak)
 * ========================================================= */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6) {
        g_dim_flag = 1;
    }
    else if (htim->Instance == TIM7) {
        g_read_flag = 1;
    }
}

/* =========================================================
 *  HAL UART TX Complete Callback (overrides weak)
 * ========================================================= */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        g_uart_busy = 0;
    }
}
