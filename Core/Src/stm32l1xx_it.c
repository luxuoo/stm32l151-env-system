/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32l1xx_it.h"
#include "app.h"
#include "tim.h"
#include "usart.h"

/* External variables --------------------------------------------------------*/
extern DMA_HandleTypeDef hdma_usart1_tx;

/******************************************************************************/
/*           Cortex-M3 Processor Interruption and Exception Handlers          */
/******************************************************************************/
void NMI_Handler(void)
{
    while (1) {}
}

void HardFault_Handler(void)
{
    while (1) {}
}

void MemManage_Handler(void)
{
    while (1) {}
}

void BusFault_Handler(void)
{
    while (1) {}
}

void UsageFault_Handler(void)
{
    while (1) {}
}

void SVC_Handler(void)    {}
void DebugMon_Handler(void) {}
void PendSV_Handler(void)   {}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

/******************************************************************************/
/* STM32L1xx Peripheral Interrupt Handlers                                    */
/******************************************************************************/

/**
  * @brief EXTI line 1 interrupt (PB1 = BTN_DOWN)
  */
void EXTI1_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_IT(BTN_DOWN_Pin)) {
        __HAL_GPIO_EXTI_CLEAR_IT(BTN_DOWN_Pin);
        HAL_GPIO_EXTI_Callback(BTN_DOWN_Pin);
    }
}

/**
  * @brief EXTI line[9:5] interrupts (PB5 = BTN_OK)
  */
void EXTI9_5_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_IT(BTN_OK_Pin)) {
        __HAL_GPIO_EXTI_CLEAR_IT(BTN_OK_Pin);
        HAL_GPIO_EXTI_Callback(BTN_OK_Pin);
    }
}

/**
  * @brief EXTI line[15:10] interrupts (PC13 = BTN_UP)
  */
void EXTI15_10_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_IT(BTN_UP_Pin)) {
        __HAL_GPIO_EXTI_CLEAR_IT(BTN_UP_Pin);
        HAL_GPIO_EXTI_Callback(BTN_UP_Pin);
    }
}

/**
  * @brief TIM6 global interrupt — 10 ms dimming flag
  */
void TIM6_IRQHandler(void)
{
    if (TIM6->SR & TIM_SR_UIF) {
        TIM6->SR &= ~TIM_SR_UIF;
        g_dim_flag = 1;
    }
}

/**
  * @brief TIM7 global interrupt — 1 s sensor read flag
  */
void TIM7_IRQHandler(void)
{
    if (TIM7->SR & TIM_SR_UIF) {
        TIM7->SR &= ~TIM_SR_UIF;
        g_read_flag = 1;
    }
}

/**
  * @brief DMA1 Channel4 interrupt (USART1_TX)
  */
void DMA1_Channel4_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart1_tx);
}

/**
  * @brief USART1 global interrupt
  */
void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
}
