#ifndef __TIM_H
#define __TIM_H

#include "stm32l1xx_hal.h"

/* Timer period constants */
#define TIM2_PWM_PERIOD  999

/* Global flags (set in ISR, consumed in main loop) */
extern volatile uint8_t g_dim_flag;
extern volatile uint8_t g_read_flag;

void MX_TIM2_Init(void);
void MX_TIM6_Init(void);
void MX_TIM7_Init(void);

void TIM2_PWM_Start(void);
void TIM2_PWM_Stop(void);
void TIM6_Start(void);
void TIM6_Stop(void);
void TIM7_Start(void);
void TIM7_Stop(void);

/* Direct register macros */
#define LED_SET_CCR(val)  (TIM2->CCR4 = (val))
#define LED_GET_PERIOD()  TIM2_PWM_PERIOD

#endif /* __TIM_H */
