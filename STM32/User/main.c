#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "LED.h"
#include "key.h"
#include "Timer.h"
#include "Encoder.h"

#include "adc.h"
#include "usart.h"
#include "stdio.h"

char buffer[32];
int16_t Speed;
uint32_t angle;
volatile uint8_t system_running = 1; // 系统运行标志
// 继电器控制引脚
#define RELAY_PIN GPIO_Pin_5
#define RELAY_PORT GPIOA

void Relay_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = RELAY_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(RELAY_PORT, &GPIO_InitStructure);
    GPIO_ResetBits(RELAY_PORT, RELAY_PIN); // 初始关闭继电器
}

void Relay_Control(uint8_t state)
{
    if(state)
        GPIO_SetBits(RELAY_PORT, RELAY_PIN);   // 继电器闭合
    else
        GPIO_ResetBits(RELAY_PORT, RELAY_PIN); // 继电器断开
}

// 电机控制
#define MOTOR_A GPIO_Pin_3
#define MOTOR_B GPIO_Pin_4
#define MOTOR_PORT GPIOA

void Motor_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = MOTOR_A | MOTOR_B;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(MOTOR_PORT, &GPIO_InitStructure);
    GPIO_ResetBits(MOTOR_PORT, MOTOR_A | MOTOR_B); // 停止
}

void Motor_SetSpeed(int16_t speed)
{
    if(speed > 0)
    {
        GPIO_SetBits(MOTOR_PORT, MOTOR_A);
        GPIO_ResetBits(MOTOR_PORT, MOTOR_B);
    }
    else if(speed < 0)
    {
        GPIO_ResetBits(MOTOR_PORT, MOTOR_A);
        GPIO_SetBits(MOTOR_PORT, MOTOR_B);
    }
    else
    {
        GPIO_ResetBits(MOTOR_PORT, MOTOR_A | MOTOR_B);
    }
}


int main(void)
{
	
	LED_Init();
    OLED_Init();
    Timer_Init();
    Encoder_Init();
    Motor_Init();
    Relay_Init();
	Emergency_Init();
	ADC1_Init();
	USART1_Init();
	
    OLED_ShowString(1, 1, "Speed:");
	OLED_ShowString(2, 1, "angle:");

	

    while(1)
    {
		
		    // 检测急停
		if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0) == 0) // STOP按下
		{
			system_running = 0;
	
			// 停止电机
			Motor_SetSpeed(0);
	
			// 关闭继电器/LED
			Relay_Control(1);
		}
		
		 // 检测复位
		if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0) // RESET按下
		{
			system_running = 1;
		}
		
		if(system_running)
		{
			float angle = Get_Angle();
			OLED_ShowNum(2, 7, angle, 3); // 显示角度
			
			OLED_ShowSignedNum(1, 7, Speed, 5);
			
			// 格式化字符串发送，例如："Speed: 123 Angle: 45.6\n"
			sprintf(buffer, "Speed:%d Angle:%.1f\r\n", Speed, angle);
			USART1_SendString(buffer);
	
			Delay_ms(200); // 每200ms发送一次
	
			// 电机跟随旋转方向
			Motor_SetSpeed(Speed);
	
			if(angle >=90)
			{
				Relay_Control(0); // 点亮 LED
			}
	
			// 速度超过50触发继电器
			if(Speed >= 50 || Speed <= -50)
			{
				Relay_Control(0); // 点亮 LED
				GPIO_ResetBits(MOTOR_PORT, MOTOR_A | MOTOR_B);
			}
			else
			{
				if(angle >=90)
				{
					Relay_Control(0); // 点亮 LED
				}
				else
				Relay_Control(1); // 关闭 LED
				
			}
		
		}
		
    }
}

// TIM2 中断更新速度
void TIM2_IRQHandler(void)
{
    if(TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
    {
        Speed = Encoder_Get(); // 获取编码器速度
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}

