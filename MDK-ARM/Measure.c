#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "gpio.h"

#define T_adc 341000 // (2.5 + 12) / 42.5 MHz = 341 мкс = 341 000 нс
#define T_tim 5.88 // время одного такта таймера TIM8
#define adcTicks 58 // 341 000 / 5.88 количество тактов таймера на 1 измерение АЦП

uint16_t calcNumMeasure(){
	uint32_t T_active = (TIM1 -> CCR1) ; // кол-во тактов активной части сигнала
	return (T_active / adcTicks);
}