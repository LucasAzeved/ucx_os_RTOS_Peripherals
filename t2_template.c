#include <ucx.h>
#include <ieee754.h>
#include <math.h>

struct sem_s *adc_mtx;

/* ADC library */
void analog_config();
void adc_config(void);
void adc_channel(uint8_t channel);
uint16_t adc_read();

/* PWM library */
void pwm_config();

/* sensors parameters */
const float F_VOLTAGE = 635.0;		// 590 ~ 700mV typical diode forward voltage
const float T_COEFF = -2.0;			// 1.8 ~ 2.2mV change per degree Celsius
const float V_RAIL = 3300.0;		// 3300mV rail voltage
const float ADC_MAX = 4095.0;		// max ADC value
const int ADC_SAMPLES = 1024;		// ADC read samples
const int REF_RESISTANCE = 4700;

/* sensor aquisition functions */
float temperature()
{
	float temp = 0.0;
	float voltage;
	
	for (int i = 0; i < ADC_SAMPLES; i++) {
		voltage = adc_read() * (V_RAIL / ADC_MAX);
		temp += ((voltage - F_VOLTAGE) / T_COEFF);
	}
	
	return (temp / ADC_SAMPLES);
}

float luminosity()
{
	float voltage, lux = 0.0, rldr;
	
	for (int i = 0; i < ADC_SAMPLES; i++) {
		voltage = adc_read() * (V_RAIL / ADC_MAX);
		rldr = (REF_RESISTANCE * (V_RAIL - voltage)) / voltage;
		lux += 500 / (rldr / 650);
	}
	
	return (lux / ADC_SAMPLES);
}

/* application threads */
void idle(void)
{
	while (1) {
	}
}

void task_1(void) // Sensor Temperatura
{
	float f;
	char fval[50];
	
	while (1) {
		/* critical section: ADC is shared! */
		ucx_sem_wait(adc_mtx);
		adc_channel(ADC_Channel_8);
		f = temperature();
		ucx_sem_signal(adc_mtx);
		
		ftoa(f, fval, 6);
		printf("temp: %s\n", fval);
		
		ucx_task_delay(100);
	}
}

void task_2(void) // Sensor Luminosidade
{
	float f;
	char fval[50];
	
	while (1) {
		/* critical section: ADC is shared! */
		ucx_sem_wait(adc_mtx);
		adc_channel(ADC_Channel_9);
		f = luminosity();
		ucx_sem_signal(adc_mtx);
		
		ftoa(f, fval, 6);
		printf("lux: %s\n", fval);
		
		ucx_task_delay(100);
	}
}

void task_3(void) // Controle Temperatura
{
	while(1) {

	}
}
void task_4(void) // Controle Dimerização
{
	while(1) {
		
	}
}
void task_5(void) // Gerenciamento Aplicacao
{
	while(1) {
		
	}
}
void task_6(void) // Depuracao
{
	while(1) {
		// printf("task ...\n");
	}
}

/* main application entry point */
int32_t app_main(void)
{
	analog_config();
	adc_config();
	pwm_config();

	ucx_task_spawn(idle, DEFAULT_STACK_SIZE);
	ucx_task_spawn(task_1, DEFAULT_STACK_SIZE);
	ucx_task_spawn(task_2, DEFAULT_STACK_SIZE);
	
	/* ADC mutex */
	adc_mtx = ucx_sem_create(5, 1);

	// start UCX/OS, non-preemptive mode
	return 1;
}
