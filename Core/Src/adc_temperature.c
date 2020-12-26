/*
 * adc_temperature.c
 *
 *  Created on: Apr 6, 2020
 *      Author: Yuan
 */
#include "main.h"

ADC_HandleTypeDef    AdcHandle;
int32_t ConvertedValue = 0;
int32_t JTemp = 0x0;

void ADC_Config(void)
{
  ADC_ChannelConfTypeDef sConfig;

  /* Configure the ADC peripheral */
  AdcHandle.Instance          = ADC1;

  AdcHandle.Init.ClockPrescaler        = ADC_CLOCKPRESCALER_PCLK_DIV4;
  AdcHandle.Init.Resolution            = ADC_RESOLUTION_12B;
  AdcHandle.Init.ScanConvMode          = DISABLE;                       /* Sequencer disabled (ADC conversion on only 1 channel: channel set on rank 1) */
  AdcHandle.Init.ContinuousConvMode    = ENABLE;                        /* Continuous mode enabled to have continuous conversion */
  AdcHandle.Init.DiscontinuousConvMode = DISABLE;                       /* Parameter discarded because sequencer is disabled */
  AdcHandle.Init.NbrOfDiscConversion   = 0;
  AdcHandle.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;        /* Conversion start not trigged by an external event */
  AdcHandle.Init.ExternalTrigConv      = ADC_EXTERNALTRIGCONV_T1_CC1;
  AdcHandle.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
  AdcHandle.Init.NbrOfConversion       = 1;
  AdcHandle.Init.DMAContinuousRequests = ENABLE;
  AdcHandle.Init.EOCSelection          = DISABLE;

  if (HAL_ADC_Init(&AdcHandle) != HAL_OK)
  {
    /* ADC initialization Error */
	  while(1);
  }

  /* Configure ADC Temperature Sensor Channel */
  sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_56CYCLES;
  sConfig.Offset = 0;

  if (HAL_ADC_ConfigChannel(&AdcHandle, &sConfig) != HAL_OK)
  {
    /* Channel Configuration Error */
    while(1);
  }
  HAL_ADC_Start_DMA(&AdcHandle, (uint32_t*)&ConvertedValue, 1);

}

#define TEMP_REFRESH_PERIOD   1000    /* Internal temperature refresh period */
#define MAX_CONVERTED_VALUE   4095    /* Max converted value */
#define AMBIENT_TEMP            25    /* Ambient Temperature */
#define VSENS_AT_AMBIENT_TEMP  760    /* VSENSE value (mv) at ambient temperature */
#define AVG_SLOPE               25    /* Avg_Solpe multiply by 10 */
#define VREF                  3300
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	JTemp = ((((ConvertedValue * VREF)/MAX_CONVERTED_VALUE) - VSENS_AT_AMBIENT_TEMP) * 10 / AVG_SLOPE) + AMBIENT_TEMP;
}
