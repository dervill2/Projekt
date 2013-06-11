
#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_tim.h"
#include "stm32f4xx_exti.h"
#include "misc.h"
#include "codec.h"
#include "sound.h"

TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure ; // struktura timerów
NVIC_InitTypeDef NVIC_InitStructure ; // struktura przerwañ
EXTI_InitTypeDef EXTI_InitStructure; // struktura przerwañ
GPIO_InitTypeDef GPIO_InitStructure; // struktura gpio

volatile uint32_t sampleCounter = 0;
volatile int16_t sample = 0;
