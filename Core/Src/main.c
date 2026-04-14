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
extern uint16_t calcNumMeasure(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void TIM6_start(){
	
	TIM6->CR1 |= TIM_CR1_CEN;
	TIM6->DIER |= TIM_DIER_UIE;
}


void TIM2_config(){
	//TIM2->DIER |= TIM_DIER_CC1IE;
}

void TIM2_start(){
	TIM2->DIER |= TIM_DIER_UIE;
	TIM2->CR1 |= TIM_CR1_CEN;
}

void TIM1_config(){
	
	TIM1->BDTR |= TIM_BDTR_MOE;
	
	TIM1 -> CR1 &= ~TIM_CR1_CMS_0;
	TIM1 -> CR1 |= TIM_CR1_CMS_1; // режим где флаг прерывания устанавливается только на подьёме и OC1REF тоже только на подьёме - для триггера АЦП
	
	
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

void DMA_ADC_config(uint16_t *data, uint16_t size)
{

    DMA1_Channel1->CCR &= ~DMA_CCR_EN;
    while (DMA1_Channel1->CCR & DMA_CCR_EN) {}


		DMA1_Channel1 -> CCR |= DMA_CCR_TCIE; // включить прерывание по концу передачи
    DMA1_Channel1->CNDTR = size;
    DMA1_Channel1->CPAR  = (uint32_t)&ADC1->DR;
    DMA1_Channel1->CMAR  = (uint32_t)data;

    if (ADC1->CR & ADC_CR_ADEN)
    {
        ADC1->CR |= ADC_CR_ADDIS;
        while (ADC1->CR & ADC_CR_ADEN) {}
    }

    ADC1->CR |= ADC_CR_ADVREGEN; // отрегулировать ADC

    for (volatile uint32_t i = 0; i < 2000; i++) { __NOP(); }

    /* 7. Calibrate ADC in differential mode */
    ADC1->CR |= ADC_CR_ADCALDIF;
    ADC1->CR |= ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL) {}



    ADC1->CFGR |= ADC_CFGR_CONT; // continous режим

    ADC1->CFGR |= ADC_CFGR_DMAEN;
    ADC1->CFGR |= ADC_CFGR_DMACFG;   // circular DMA 


    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY)) {}

    DMA1_Channel1->CCR |= DMA_CCR_EN;

    ADC1->CR |= ADC_CR_ADSTART; // врубить ADC
}

void ADC_restart(uint16_t *data, uint16_t size)
{
    /* 1. Остановить regular conversions */
    if (ADC1->CR & ADC_CR_ADSTART)
    {
        ADC1->CR |= ADC_CR_ADSTP;
        while (ADC1->CR & ADC_CR_ADSTP) {}
    }

    /* 2. Выключить DMA channel */
    DMA1_Channel1->CCR &= ~DMA_CCR_EN;
    while (DMA1_Channel1->CCR & DMA_CCR_EN) {}

    /* 3. Очистить DMA flags */
    DMA1->IFCR = DMA_IFCR_CGIF1 |
                 DMA_IFCR_CTCIF1 |
                 DMA_IFCR_CHTIF1 |
                 DMA_IFCR_CTEIF1;

    /* 4. Очистить флаги ADC */
    ADC1->ISR = ADC_ISR_EOC |
                ADC_ISR_EOS |
                ADC_ISR_OVR;

    /* 5. Перезагрузить DMA */
    DMA1_Channel1->CNDTR = size;
    DMA1_Channel1->CPAR  = (uint32_t)&ADC1->DR;
    DMA1_Channel1->CMAR  = (uint32_t)data;

    /* 6. Снова включить DMA */
    DMA1_Channel1->CCR |= DMA_CCR_EN;

    /* 7. Снова взвести ADC. */
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
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
	
	//HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_1);
	//HAL_TIMEx_PWMN_Start(&htim8, TIM_CHANNEL_1);
	
	//HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_results, 20);
	
	DMA_ADC_config(adc_results, calcNumMeasure());
	
	TIM2_config();
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
