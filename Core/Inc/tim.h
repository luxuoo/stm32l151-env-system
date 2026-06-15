#ifndef __TIM_H
#define __TIM_H

#include "stm32l1xx_hal.h"

extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim6;
extern TIM_HandleTypeDef htim7;

void MX_TIM2_Init(void);
void MX_TIM6_Init(void);
void MX_TIM7_Init(void);

#endif /* __TIM_H */
