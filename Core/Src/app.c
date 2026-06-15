#include "app.h"
#include "bh1750.h"
#include "XMF_OLED_STM32Cube.h"
#include "i2c.h"
#include "usart.h"
#include "tim.h"
#include <stdio.h>
#include <string.h>

/* ---- Private variables ---- */
static char oled_buf[32];
static char uart_buf[48];
static volatile uint32_t last_btn_tick = 0;
static float g_current_lux = 0.0f;

/* ---- Global variables ---- */
volatile SystemState_t g_sys_state   = STATE_IDLE;
volatile uint8_t       g_menu_cursor = 0;
volatile uint8_t       g_need_redraw = 1;
volatile uint8_t       g_read_flag   = 0;
volatile uint8_t       g_dim_flag    = 0;
volatile uint8_t       g_uart_busy   = 0;
volatile float         g_smooth_lux  = 999.0f;
volatile uint8_t       g_sensor_ok   = 0;

/* =========================================================
 *  UART DMA transmit
 * ========================================================= */
void UART_SendString(const char *str)
{
    uint16_t len = strlen(str);
    if (len == 0) return;
    uint32_t tick = HAL_GetTick();
    while (g_uart_busy && (HAL_GetTick() - tick < 50));
    g_uart_busy = 1;
    HAL_UART_Transmit_DMA(&huart1, (uint8_t *)str, len);
}

/* =========================================================
 *  I2C2 bus recovery
 * ========================================================= */
static void I2C2_Recover(void)
{
    __HAL_RCC_I2C2_CLK_DISABLE();
    HAL_Delay(10);
    __HAL_RCC_I2C2_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_10; gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL; gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);

    for (int i = 0; i < 9; i++) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
        HAL_Delay(1);
    }

    gpio.Pin = GPIO_PIN_10 | GPIO_PIN_11; gpio.Mode = GPIO_MODE_AF_OD;
    gpio.Pull = GPIO_NOPULL; gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF4_I2C2;
    HAL_GPIO_Init(GPIOB, &gpio);

    MX_I2C2_Init();
    HAL_Delay(10);
}

/* =========================================================
 *  BH1750 read with error recovery
 * ========================================================= */
static void Sensor_Read(void)
{
    uint8_t buf[2];
    HAL_StatusTypeDef ret = HAL_I2C_Master_Receive(&hi2c2, BH1750_ADDR, buf, 2, 200);

    if (ret == HAL_OK) {
        uint16_t raw = ((uint16_t)buf[0] << 8) | buf[1];
        g_current_lux = (float)raw / 1.2f;
        if (!g_sensor_ok) {
            g_sensor_ok = 1;
            if (g_sys_state == STATE_ERROR) g_sys_state = STATE_RUNNING;
        }
    } else {
        g_sensor_ok = 0; g_current_lux = 0.0f;
        if (g_sys_state == STATE_RUNNING) {
            g_sys_state = STATE_ERROR;
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, 999);
            UART_SendString("SENSOR_DISCONNECT\r\n");
        }
        I2C2_Recover();
    }
}

/* =========================================================
 *  Dimming (10ms tick)
 *  PWM1 + OCPOLARITY_LOW: CCR=999 → LED OFF, CCR=0 → LED ON
 * ========================================================= */
static void Dimming_Algorithm(void)
{
    uint16_t target;

    if (!g_sensor_ok || g_sys_state != STATE_RUNNING) {
        target = 999;
    } else if (g_current_lux >= 300.0f) {
        target = 999;
    } else {
        float ratio = g_current_lux / 300.0f;
        target = (uint16_t)(999.0f * (1.0f - ratio));
        /* Invert: PWM1+LOW polarity means lower CCR = brighter */
        target = 999 - target;
    }

    /* First-order lag filter α=0.05 */
    g_smooth_lux = g_smooth_lux + 0.05f * ((float)target - g_smooth_lux);
    uint16_t ccr = (uint16_t)g_smooth_lux;
    if (ccr > 999) ccr = 999;

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, ccr);
}

/* =========================================================
 *  Menu screen
 * ========================================================= */
static void Menu_Draw(void)
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
 *  Running/Error screen
 * ========================================================= */
static void Running_Draw(void)
{
    if (g_sys_state == STATE_ERROR) {
        OLED_Clear();
        OLED_ShowString(0, 0, (unsigned char *)"NO DATA     ");
        OLED_ShowString(0, 2, (unsigned char *)"LED: OFF    ");
        return;
    }

    uint16_t lux_x10 = (uint16_t)(g_current_lux * 10.0f + 0.5f);
    sprintf(oled_buf, "%d.%d       ", lux_x10 / 10, lux_x10 % 10);

    OLED_ShowString(0, 0, (unsigned char *)"Light:");
    OLED_ShowString(0, 2, (unsigned char *)oled_buf);
    OLED_ShowString(0, 4, (unsigned char *)"Lux         ");

    if (g_current_lux < 300.0f && g_sensor_ok)
        OLED_ShowString(0, 6, (unsigned char *)"LED: ON     ");
    else
        OLED_ShowString(0, 6, (unsigned char *)"LED: OFF    ");
}

/* =========================================================
 *  App_Init
 * ========================================================= */
void App_Init(void)
{
    g_sys_state   = STATE_IDLE;
    g_menu_cursor = 0;
    g_need_redraw = 1;
    g_uart_busy   = 0;
    g_smooth_lux  = 999.0f;
    g_sensor_ok   = 0;

    /* Fix TIM6 to 10ms (CubeMX set it to 1s) */
    HAL_TIM_Base_Stop_IT(&htim6);
    htim6.Init.Prescaler = 3199;
    htim6.Init.Period    = 99;
    HAL_TIM_Base_Init(&htim6);

    OLED_Init();
    OLED_Clear();
    OLED_ShowString(0, 0, (unsigned char *)"BH1750 System");
    OLED_ShowString(0, 2, (unsigned char *)"Ready...");
    HAL_Delay(800);

    BH1750_Init(&hi2c2);
    HAL_Delay(200);
    g_need_redraw = 1;
}

/* =========================================================
 *  App_Loop
 * ========================================================= */
void App_Loop(void)
{
    if (g_read_flag && g_sys_state == STATE_RUNNING) {
        g_read_flag = 0;
        Sensor_Read();
        g_need_redraw = 1;
    }

    if (g_dim_flag) {
        g_dim_flag = 0;
        if (g_sys_state == STATE_RUNNING || g_sys_state == STATE_ERROR)
            Dimming_Algorithm();
    }

    if (g_need_redraw) {
        g_need_redraw = 0;
        switch (g_sys_state) {
        case STATE_IDLE:              Menu_Draw();    break;
        case STATE_RUNNING:
        case STATE_ERROR:             Running_Draw(); break;
        case STATE_OFF:                             break;
        }
    }
}

/* =========================================================
 *  Button handler
 * ========================================================= */
static void Handle_ButtonPress(uint16_t pin)
{
    uint32_t now = HAL_GetTick();
    if (now - last_btn_tick < 200) return;
    last_btn_tick = now;

    if (g_sys_state == STATE_IDLE) {
        if (pin == BTN_UP_Pin) {
            if (g_menu_cursor > 0) g_menu_cursor--;
            g_need_redraw = 1;
        } else if (pin == BTN_DOWN_Pin) {
            if (g_menu_cursor < 1) g_menu_cursor++;
            g_need_redraw = 1;
        } else if (pin == BTN_OK_Pin) {
            if (g_menu_cursor == 0) {
                g_sys_state  = STATE_RUNNING;
                g_smooth_lux = 999.0f;
                g_sensor_ok  = 0;
                g_read_flag  = 1;
                BH1750_Init(&hi2c2);
                HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
                __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, 999);
                HAL_TIM_Base_Start_IT(&htim6);
                HAL_TIM_Base_Start_IT(&htim7);
                OLED_Clear();
                UART_SendString("SYSTEM_START\r\n");
            } else {
                g_sys_state = STATE_OFF;
                BH1750_WriteCmd(&hi2c2, BH1750_POWER_OFF);
                HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_4);
                HAL_TIM_Base_Stop_IT(&htim6);
                HAL_TIM_Base_Stop_IT(&htim7);
                OLED_Clear();
                OLED_Display_Off();
                UART_SendString("SYSTEM_OFF\r\n");
            }
            g_need_redraw = 1;
        }
    }
    else if (g_sys_state == STATE_RUNNING || g_sys_state == STATE_ERROR) {
        if (pin == BTN_DOWN_Pin || pin == BTN_OK_Pin) {
            g_sys_state = STATE_IDLE;
            BH1750_WriteCmd(&hi2c2, BH1750_POWER_OFF);
            HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_4);
            HAL_TIM_Base_Stop_IT(&htim6);
            HAL_TIM_Base_Stop_IT(&htim7);
            OLED_Display_On();
            OLED_Clear();
            UART_SendString("SYSTEM_OFF\r\n");
            g_need_redraw = 1;
        }
    }
    else if (g_sys_state == STATE_OFF) {
        if (pin == BTN_OK_Pin) {
            OLED_Display_On();
            g_sys_state = STATE_IDLE;
            g_need_redraw = 1;
        }
    }
}

/* =========================================================
 *  HAL callbacks
 * ========================================================= */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == BTN_UP_Pin || GPIO_Pin == BTN_DOWN_Pin || GPIO_Pin == BTN_OK_Pin)
        Handle_ButtonPress(GPIO_Pin);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6) g_dim_flag = 1;
    if (htim->Instance == TIM7) g_read_flag = 1;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) g_uart_busy = 0;
}
