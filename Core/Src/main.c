/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "math.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint16_t adc_results[200];
float adcVoltage[200];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void TIM6_start(){
	
	TIM6->CR1 |= TIM_CR1_CEN;
	TIM6->DIER |= TIM_DIER_UIE;
}

void TIM1_config(){
	
	TIM1->BDTR |= TIM_BDTR_MOE;
	TIM1 -> CR1 |= TIM_CR1_CMS_1;
	
	TIM1->CCER |= TIM_CCER_CC1E; 
	TIM1->CCER |= TIM_CCER_CC1NE; 
	
	TIM1->CCER |= TIM_CCER_CC2E; 
	TIM1->CCER |= TIM_CCER_CC2NE; 
	
	TIM1->CCER |= TIM_CCER_CC3E; 
	TIM1->CCER |= TIM_CCER_CC3NE; 

}

void TIM1_start(){
	TIM1 -> CR1 |= TIM_CR1_CEN;
}

/*
void DMA_ADC_config(uint16_t *data){
	
	DMA1_Channel1 -> CCR &= ~DMA_CCR_EN; // вырубить DMA 
	
	//настройка DMA
	DMA1_Channel1 -> CCR &= ~DMA_CCR_DIR; //read from peripheral
	DMA1_Channel1 -> CCR |= DMA_CCR_TCIE; // включить TC прерывание
	DMA1_Channel1 -> CCR |= DMA_CCR_CIRC; // circ режим
	DMA1_Channel1 -> CCR |= DMA_CCR_MINC; // memory increment
	DMA1_Channel1 -> CCR |= DMA_CCR_MSIZE_0;
	DMA1_Channel1 -> CCR &= ~DMA_CCR_MSIZE_1; // 16 bits 
	
	DMA1_Channel1 -> CNDTR = 30;
	DMA1_Channel1 -> CPAR = (uint32_t)&ADC1->DR;
	DMA1_Channel1 -> CMAR = (uint32_t)data;

	
	// настройка ADC
	ADC1->CR |= ADC_CR_ADVREGEN; // калибровка напряжения
	HAL_Delay(10);
	ADC1->CR |= ADC_CR_ADCALDIF; 
	ADC1->CR |= ADC_CR_ADCAL; // калибровка ADC
	HAL_Delay(10);
	
	ADC1-> CFGR |= ADC_CFGR_CONT; // continous mode
	ADC1-> CFGR |= ADC_CFGR_EXTEN_0;
	ADC1 -> CFGR &= ~(ADC_CFGR_RES_0 | ADC_CFGR_RES_1);
	
	ADC1->CFGR &= ~(ADC_CFGR_RES_0 | ADC_CFGR_RES_1);
	
	ADC1->CFGR |= ADC_CFGR_DMACFG; 
	ADC1->CFGR |= ADC_CFGR_DMAEN; 
	
	ADC1->CR |= ADC_CR_ADEN; // врубить ADC
	DMA1_Channel1 -> CCR |= DMA_CCR_EN; // врубить DMA
	
	//ADC1->CR |= ADC_CR_ADSTART;

}
*/

void DMA_ADC_config(uint16_t *data)
{
    /* ---------------- DMA ---------------- */

    /* 1. Disable DMA channel */
    DMA1_Channel1->CCR &= ~DMA_CCR_EN;
    while (DMA1_Channel1->CCR & DMA_CCR_EN) {}

    /* 2. Clear DMA flags */
    DMA1->IFCR = DMA_IFCR_CGIF1 | DMA_IFCR_CTCIF1 |
                 DMA_IFCR_CHTIF1 | DMA_IFCR_CTEIF1;

    /* 3. Configure DMA: peripheral -> memory, circular, half-word */
    DMA1_Channel1->CCR &= ~DMA_CCR_DIR;      // read from peripheral
    DMA1_Channel1->CCR |= DMA_CCR_TCIE;      // transfer complete interrupt
    DMA1_Channel1->CCR |= DMA_CCR_CIRC;      // circular mode
    DMA1_Channel1->CCR |= DMA_CCR_MINC;      // memory increment

    DMA1_Channel1->CCR |= DMA_CCR_MSIZE_0;   // memory size = 16 bit
    DMA1_Channel1->CCR &= ~DMA_CCR_MSIZE_1;

    DMA1_Channel1->CCR |= DMA_CCR_PSIZE_0;   // peripheral size = 16 bit
    DMA1_Channel1->CCR &= ~DMA_CCR_PSIZE_1;

    DMA1_Channel1->CNDTR = 30;
    DMA1_Channel1->CPAR  = (uint32_t)&ADC1->DR;
    DMA1_Channel1->CMAR  = (uint32_t)data;

    /* ---------------- ADC power / calibration ---------------- */

    /* 4. If ADC enabled, disable it first */
    if (ADC1->CR & ADC_CR_ADEN)
    {
        ADC1->CR |= ADC_CR_ADDIS;
        while (ADC1->CR & ADC_CR_ADEN) {}
    }

    /* 5. Exit deep power down, enable regulator */
    ADC1->CR &= ~ADC_CR_DEEPPWD;
    ADC1->CR |= ADC_CR_ADVREGEN;

    /* regulator startup delay */
    for (volatile uint32_t i = 0; i < 2000; i++) { __NOP(); }

    /* 6. Set differential mode for the needed channel
       Example below is for channel 2 only.
       Replace if your ADC regular channel is different. */
    ADC1->DIFSEL |= ADC_DIFSEL_DIFSEL_1;

    /* 7. Calibrate ADC in differential mode */
    ADC1->CR |= ADC_CR_ADCALDIF;
    ADC1->CR |= ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL) {}

    /* 8. Clear ADC flags */
    ADC1->ISR = ADC_ISR_ADRDY | ADC_ISR_EOC | ADC_ISR_EOS | ADC_ISR_OVR;

    /* ---------------- ADC configuration ---------------- */

    /* 9. 12-bit resolution */
    ADC1->CFGR &= ~(ADC_CFGR_RES_0 | ADC_CFGR_RES_1);

    /* 10. Disable continuous mode if conversion must start from TIM1 event */
    ADC1->CFGR &= ~ADC_CFGR_CONT;

    /* 11. Enable DMA from ADC */
    ADC1->CFGR |= ADC_CFGR_DMAEN;
    ADC1->CFGR |= ADC_CFGR_DMACFG;   // circular DMA requests

    /* 12. External trigger edge */
    ADC1->CFGR &= ~ADC_CFGR_EXTEN;
    ADC1->CFGR |= ADC_CFGR_EXTEN_0;  // rising edge

    /* 13. Select external trigger source
       IMPORTANT:
       Put here the exact TIM1 source you use in CubeMX:
       - TIM1_CC1 event
       - or TIM1_TRGO
       - or TIM1_TRGO2

       Example for TIM1_CC1 with HAL macro:
       ADC1->CFGR &= ~ADC_CFGR_EXTSEL;
       ADC1->CFGR |= ADC_EXTERNALTRIG_T1_CC1;
    */
    ADC1->CFGR &= ~ADC_CFGR_EXTSEL;
    ADC1->CFGR |= ADC_EXTERNALTRIG_T1_CC1;

    /* ---------------- ADC enable / DMA enable ---------------- */

    /* 14. Enable ADC */
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY)) {}

    /* 15. Enable DMA channel */
    DMA1_Channel1->CCR |= DMA_CCR_EN;

    /* 16. Arm ADC regular group
       For external trigger mode this usually puts ADC in waiting state.
       This matches HAL behavior more closely. */
    ADC1->CR |= ADC_CR_ADSTART;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM1_Init();
  MX_TIM6_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
	
	//HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_1);
	//HAL_TIMEx_PWMN_Start(&htim8, TIM_CHANNEL_1);
	
	//HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_results, 20);
	DMA_ADC_config(adc_results);
	
	TIM1_config();
	TIM1_start();
	
	TIM6_start();
	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
