#ifndef __ADC_H
#define __ADC_H


#include "stm32f10x.h"

void ADC1_Init(void);

// ∂¡»°ADC÷µ
uint16_t ADC_Read_PA0(void);

float Get_Angle(void);

#endif
