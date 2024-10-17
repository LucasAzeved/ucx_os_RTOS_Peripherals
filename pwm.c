#include <ucx.h>

void pwm_config()
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;
	TIM_OCInitTypeDef TIM_OCStruct;
	GPIO_InitTypeDef GPIO_InitStruct;

	/* Enable clock for TIM3 */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	
	/* TIM3 by default has clock of 84MHz
	 * Update Event (Hz) = timer_clock / ((TIM_Prescaler + 1) * (TIM_Period + 1))
	 * Update Event (Hz) = 84MHz / ((839 + 1) * (999 + 1)) = 1000 Hz
	 */
	TIM_TimeBaseInitStruct.TIM_Prescaler = 839;
	TIM_TimeBaseInitStruct.TIM_Period = 999;
	TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_CenterAligned1;
	
	/* TIM3 initialize */
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStruct);
	/* Enable TIM3 interrupt */
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
	/* Start TIM3 */
	TIM_Cmd(TIM3, ENABLE);
	
	/* Clock for GPIOB */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	/* Alternating functions for pins */
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource0, GPIO_AF_TIM3); // PINOS ALTERADOS
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource1, GPIO_AF_TIM3); // PINOS ALTERADOS

	/* Set pins */
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1; // PINOS ALTERADOS
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* set OC mode */
	TIM_OCStruct.TIM_OCMode = TIM_OCMode_PWM2;
	TIM_OCStruct.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCStruct.TIM_OCIdleState = TIM_OCIdleState_Reset;
	
	/* set TIM3 CH3, 50% duty cycle */
	TIM_OCStruct.TIM_OCPolarity = TIM_OCPolarity_Low;
	TIM_OCStruct.TIM_Pulse = 499;
	TIM_OC3Init(TIM3, &TIM_OCStruct);
	TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);

	/* set TIM3 CH4, 50% duty cycle */
	TIM_OCStruct.TIM_OCPolarity = TIM_OCPolarity_Low;
	TIM_OCStruct.TIM_Pulse = 499;
	TIM_OC4Init(TIM3, &TIM_OCStruct);
	TIM_OC4PreloadConfig(TIM3, TIM_OCPreload_Enable);
}
