#ifndef _ultr_H//���û��_ultr_H
#define _ultr_H//����_ultr_H

//# include "system.h"//λ���������ļ�
#include "stm32f4xx.h"
//#include "usart.h"
#include "Drv_time.h"
//��������

//void ultr_Init(void);
//void UltrasonicWave_StartMeasure(void);
//float UltrasonicWave_Measure(void) ;
//void _TIM4_IRQHandler(void);
void TIM4_CH1_Cap_Init(u32 arr,u16 psc);

void T265_loc1(u8 dT_ms);

#endif

