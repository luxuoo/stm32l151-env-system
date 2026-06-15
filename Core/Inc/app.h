#ifndef __APP_H
#define __APP_H

#include "stm32l1xx_hal.h"

/* ---- Button pins ---- */
#define BTN_UP_Pin       GPIO_PIN_13
#define BTN_UP_Port      GPIOC
#define BTN_DOWN_Pin     GPIO_PIN_1
#define BTN_DOWN_Port    GPIOB
#define BTN_OK_Pin       GPIO_PIN_5
#define BTN_OK_Port      GPIOB

/* ---- System states ---- */
typedef enum {
    STATE_IDLE,       /* Menu screen */
    STATE_RUNNING,    /* Sensor reading + LED control */
    STATE_OFF,        /* Low-power sleep */
    STATE_ERROR       /* Sensor disconnected */
} SystemState_t;

/* ---- Global variables ---- */
extern TIM_HandleTypeDef htim2;   /* PWM for LED */
extern TIM_HandleTypeDef htim6;   /* 10ms dimming */
extern TIM_HandleTypeDef htim7;   /* 1s read flag */

extern volatile SystemState_t  g_sys_state;
extern volatile uint8_t        g_menu_cursor;    /* 0=SYS START, 1=SYS OFF */
extern volatile uint8_t        g_need_redraw;     /* OLED refresh flag */
extern volatile uint8_t        g_read_flag;       /* TIM7: time to read sensor */
extern volatile uint8_t        g_dim_flag;        /* TIM6: time to run dimming */
extern volatile uint8_t        g_uart_busy;       /* DMA TX in progress */
extern volatile float          g_smooth_lux;      /* filtered lux value */
extern volatile uint16_t       g_pwm_target;      /* target CCR value */
extern volatile uint8_t        g_sensor_ok;       /* BH1750 responding */

/* ---- Functions ---- */
void App_Init(void);
void App_Loop(void);

void MX_TIM2_Init(void);
void MX_TIM6_Init(void);
void MX_TIM7_Init(void);

void Menu_Draw(void);
void Running_Draw(void);

void Dimming_Algorithm(void);
void Sensor_Read(void);

void UART_SendString(const char *str);
void I2C2_Recover(void);

#endif /* __APP_H */
