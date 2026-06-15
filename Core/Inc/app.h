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
    STATE_IDLE,
    STATE_RUNNING,
    STATE_OFF,
    STATE_ERROR
} SystemState_t;

/* ---- Global variables ---- */
extern volatile SystemState_t  g_sys_state;
extern volatile uint8_t        g_menu_cursor;
extern volatile uint8_t        g_need_redraw;
extern volatile uint8_t        g_read_flag;
extern volatile uint8_t        g_dim_flag;
extern volatile uint8_t        g_uart_busy;
extern volatile float          g_smooth_lux;
extern volatile uint8_t        g_sensor_ok;
extern volatile uint16_t       g_btn_event;     /* button pin that was pressed */

/* ---- Functions ---- */
void App_Init(void);
void App_Loop(void);
void UART_SendString(const char *str);

#endif /* __APP_H */
