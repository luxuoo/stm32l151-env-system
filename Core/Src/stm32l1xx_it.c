/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32l1xx_it.h"
#include "app.h"

/* External variables --------------------------------------------------------*/
extern TIM_HandleTypeDef htim6;
extern TIM_HandleTypeDef htim7;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;
extern UART_HandleTypeDef huart1;

/******************************************************************************/
/*           Cortex-M3 Processor Interruption and Exception Handlers          */
/******************************************************************************/
void NMI_Handler(void)          { while (1) {} }
void HardFault_Handler(void)    { while (1) {} }
void MemManage_Handler(void)    { while (1) {} }
void BusFault_Handler(void)     { while (1) {} }
void UsageFault_Handler(void)   { while (1) {} }
void SVC_Handler(void)          {}
void DebugMon_Handler(void)     {}
void PendSV_Handler(void)       {}

void SysTick_Handler(void)
{
  HAL_IncTick();
}

/******************************************************************************/
/* STM32L1xx Peripheral Interrupt Handlers                                    */
/******************************************************************************/

/* EXTI line 1 (PB1 = BTN_DOWN) */
void EXTI1_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(BTN_DOWN_Pin);
}

/* EXTI line[9:5] (PB5 = BTN_OK) */
void EXTI9_5_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(BTN_OK_Pin);
}

/* EXTI line[15:10] (PC13 = BTN_UP) */
void EXTI15_10_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(BTN_UP_Pin);
}

/* DMA1 Channel4 (USART1_TX) */
void DMA1_Channel4_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_usart1_tx);
}

/* DMA1 Channel5 (USART1_RX) */
void DMA1_Channel5_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_usart1_rx);
}

/* USART1 */
void USART1_IRQHandler(void)
{
  HAL_UART_IRQHandler(&huart1);
}

/* TIM6 (10ms dimming) */
void TIM6_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim6);
}

/* TIM7 (1s sensor read) */
void TIM7_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim7);
}
