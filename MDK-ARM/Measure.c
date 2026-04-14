#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "gpio.h"

#define CPU_Ticks_to_ADC_Ticks 4 // на 1 такт АЦП приходится 4 такта ЦПУ
#define ADC_Ticks 60 // 47.5 sampling time + 12 бит разрешение = 60 тактов АЦП для 1 расчёта


uint16_t calcNumMeasure(){
	uint32_t T_active = (TIM1 -> CCR1) * 2; // кол-во тактов активной части сигнала (Умножается на 2, так как ШИМ центральный)
	uint16_t result = (T_active / (CPU_Ticks_to_ADC_Ticks * ADC_Ticks));
	if (result > 0) {
		return result;
	}
	else {
		return 1;
	} 
}