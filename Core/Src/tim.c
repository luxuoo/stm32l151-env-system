/**
  * @file  tim.c
  * @brief Timer initialization using direct register access (no HAL TIM)
  *
  * TIM2 CH4 (PA3)  — PWM 1 kHz for LED (low-active)
  * TIM6             — 10 ms dimming tick
  * TIM7             — 1 s sensor read tick
  */

#include "tim.h"
#include "stm32l1xx.h"

/* ---- Timer handles (for compatibility) ---- */
TIM_TypeDef *TIM2_INST = TIM2;
TIM_TypeDef *TIM6_INST = TIM6;
TIM_TypeDef *TIM7_INST = TIM7;

volatile uint8_t g_dim_flag  = 0;
volatile uint8_t g_read_flag = 0;

/* =========================================================
 *  TIM2 CH4 PWM — PA3 (LED, low-active)
 *  32 MHz / 32 / 1000 = 1 kHz
 *  PWM Mode 2: output LOW when CNT < CCR4 → LED ON
 * ========================================================= */
void MX_TIM2_Init(void)
{
    /* Enable clocks */
    RCC->AHBENR  |= RCC_AHBENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    /* PA3 → AF1 (TIM2_CH4) */
    GPIOA->MODER   &= ~(3u << (3 * 2));
    GPIOA->MODER   |=  (2u << (3 * 2));   /* Alternate function */
    GPIOA->OTYPER  &= ~(1u << 3);         /* Push-pull */
    GPIOA->OSPEEDR &= ~(3u << (3 * 2));
    GPIOA->OSPEEDR |=  (1u << (3 * 2));   /* Medium speed */
    GPIOA->PUPDR   &= ~(3u << (3 * 2));   /* No pull */
    GPIOA->AFR[0]  &= ~(0xFu << (3 * 4));
    GPIOA->AFR[0]  |=  (1u   << (3 * 4)); /* AF1 = TIM2 */

    /* Timer configuration */
    TIM2->PSC  = 31;          /* 32 MHz / 32 = 1 MHz */
    TIM2->ARR  = 999;         /* 1 MHz / 1000 = 1 kHz */
    TIM2->CCR4 = 999;         /* Start with LED off */

    /* CCMR2: OC4M = 110 (PWM mode 2), OC4PE enable */
    TIM2->CCMR2 &= ~TIM_CCMR2_OC4M;
    TIM2->CCMR2 |=  (6u << TIM_CCMR2_OC4M_Pos);  /* PWM mode 2 */
    TIM2->CCMR2 |=  TIM_CCMR2_OC4PE;              /* Preload enable */

    /* CCER: Enable CH4 output (CC4P=0 → active high, but PWM2 inverts) */
    TIM2->CCER &= ~TIM_CCER_CC4P;
    TIM2->CCER |=  TIM_CCER_CC4E;

    /* EGR: Generate update event to load PSC/ARR */
    TIM2->EGR = TIM_EGR_UG;
    /* Clear update flag */
    TIM2->SR &= ~TIM_SR_UIF;
}

void TIM2_PWM_Start(void)
{
    TIM2->CR1 |= TIM_CR1_CEN;
}

void TIM2_PWM_Stop(void)
{
    TIM2->CR1 &= ~TIM_CR1_CEN;
    TIM2->CCR4 = 999;  /* LED off */
}

/* =========================================================
 *  TIM6 — 10 ms dimming tick
 *  32 MHz / 3200 / 100 = 100 Hz → 10 ms
 * ========================================================= */
void MX_TIM6_Init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;

    TIM6->PSC  = 3199;
    TIM6->ARR  = 99;
    TIM6->DIER = TIM_DIER_UIE;   /* Update interrupt enable */
    TIM6->EGR  = TIM_EGR_UG;
    TIM6->SR  &= ~TIM_SR_UIF;

    /* NVIC: TIM6 IRQ = 54 */
    NVIC_SetPriority(TIM6_IRQn, 3);
    NVIC_EnableIRQ(TIM6_IRQn);
}

void TIM6_Start(void)
{
    TIM6->SR  &= ~TIM_SR_UIF;
    TIM6->CR1 |=  TIM_CR1_CEN;
}

void TIM6_Stop(void)
{
    TIM6->CR1 &= ~TIM_CR1_CEN;
}

/* =========================================================
 *  TIM7 — ~1 s sensor read tick
 *  32 MHz / 32000 / 1000 = 1 Hz → 1 s
 * ========================================================= */
void MX_TIM7_Init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;

    TIM7->PSC  = 31999;
    TIM7->ARR  = 999;
    TIM7->DIER = TIM_DIER_UIE;   /* Update interrupt enable */
    TIM7->EGR  = TIM_EGR_UG;
    TIM7->SR  &= ~TIM_SR_UIF;

    /* NVIC: TIM7 IRQ = 55 */
    NVIC_SetPriority(TIM7_IRQn, 3);
    NVIC_EnableIRQ(TIM7_IRQn);
}

void TIM7_Start(void)
{
    TIM7->SR  &= ~TIM_SR_UIF;
    TIM7->CR1 |=  TIM_CR1_CEN;
}

void TIM7_Stop(void)
{
    TIM7->CR1 &= ~TIM_CR1_CEN;
}
