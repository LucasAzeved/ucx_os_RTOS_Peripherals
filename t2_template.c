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

const int temp_max = 40;
const int lumi_max = 100;

struct mq_s *mq1, *mq2, *mq3, *mq4, *mq5;

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

	int val = 0;
	struct message_s msg1;
	struct message_s *pmsg;
	
	while (1) {
		/* critical section: ADC is shared! */
		ucx_sem_wait(adc_mtx);
		adc_channel(ADC_Channel_8);
		f = temperature();
		ucx_sem_signal(adc_mtx);
		
		ftoa(f, fval, 6);
		//printf("temp: %s\n", fval);

		pmsg = &msg1;
		pmsg->data = (void *)(size_t)fval;
		ucx_mq_enqueue(mq1, pmsg);
		
		ucx_task_yield();
	}
}

void task_2(void) // Sensor Luminosidade
{
	float f;
	char fval[50];

	struct message_s msg2;
	struct message_s *pmsg;
	
	while (1) {
		/* critical section: ADC is shared! */
		ucx_sem_wait(adc_mtx);
		adc_channel(ADC_Channel_9);
		f = luminosity();
		ucx_sem_signal(adc_mtx);
		
		ftoa(f, fval, 6);
		//printf("lux: %s\n", fval);
		
		pmsg = &msg2;
		pmsg->data = (void *)(size_t)fval;
		ucx_mq_enqueue(mq2, pmsg);
		
		ucx_task_yield();
	}
}

void task_3(void) // Controle Temperatura (B0)
{
	float temperatura = 0;
	struct message_s *pmsg;
	
	while(1) {

		if (ucx_mq_items(mq3) > 0) {
			pmsg = ucx_mq_dequeue(mq3);
			temperatura = atof(pmsg->data);
		}
		

		if (temperatura < 0){
			TIM3->CCR3 = 0;
		}
		else if (temperatura > 40){
			TIM3->CCR3 = 999;
		}
		else{
			TIM3->CCR3 = 999*(temperatura/temp_max);
		}
		ucx_task_yield();
	}
}

void task_4(void) // Controle Dimerização (B1)
{
	float luminosidade = 0;
	struct message_s *pmsg;

	while(1) {
		
		if (ucx_mq_items(mq4) > 0) {
			pmsg = ucx_mq_dequeue(mq4);
			luminosidade = atof(pmsg->data);
		}
		
		if (luminosidade < 10){
			TIM3->CCR4 = 999;
		}
		else if (luminosidade > 100){
			TIM3->CCR4 = 0;
		}
		else{
			TIM3->CCR4 = 999*(1-(luminosidade/lumi_max));
		}
		ucx_task_yield();
	}
}
void task_5(void) // Gerenciamento Aplicacao
{
	struct message_s msg1, msg2, msg3, msg4;
	float temperatura = 0, luminosidade = 0;
	char str[50];
	struct message_s *pmsg1 , *pmsg2;
	
	while (1) {
//		printf("t2\n");
		
		if (ucx_mq_items(mq1) > 0) {
			pmsg1 = ucx_mq_dequeue(mq1);
			pmsg1 = &msg1;
			temperatura = atof(pmsg1->data);
			ucx_mq_enqueue(mq3, pmsg1);

			pmsg1 = &msg2;
			sprintf(str, "Temperatura: %f", temperatura);
			pmsg1->data = (void *)&str;
			ucx_mq_enqueue(mq5, pmsg1);
		}
		if (ucx_mq_items(mq2) > 0) {
			pmsg2 = ucx_mq_dequeue(mq1);
			pmsg2 = &msg3;
			luminosidade = atof(pmsg2->data);
			ucx_mq_enqueue(mq4, pmsg2);

			pmsg2 = &msg4;
			sprintf(str, "Luminosidade: %f", luminosidade);
			pmsg2->data = (void *)&str;
			ucx_mq_enqueue(mq5, pmsg2);
		}
		
		ucx_task_yield();
	}
}
void task_6(void) // Depuracao
{
	struct message_s *msg1, *msg2;
	// struct message_s dummy;
	
	while (1) {
		
		if (ucx_mq_items(mq5) > 1) {

			msg1 = ucx_mq_dequeue(mq5);
			msg2 = ucx_mq_dequeue(mq5);
			printf("T1 -  %s\n", (char *)msg1->data);
			printf("T2 -  %s\n", (char *)msg2->data);
			
			_delay_ms(100);
			
		}
		
		ucx_task_yield();
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
	ucx_task_spawn(task_3, DEFAULT_STACK_SIZE);
	ucx_task_spawn(task_4, DEFAULT_STACK_SIZE);
	ucx_task_spawn(task_5, DEFAULT_STACK_SIZE);
	ucx_task_spawn(task_6, DEFAULT_STACK_SIZE);
	
	/* ADC mutex */
	adc_mtx = ucx_sem_create(5, 1);

	mq1 = ucx_mq_create(8);
	mq2 = ucx_mq_create(8);
	mq3 = ucx_mq_create(8);
	mq4 = ucx_mq_create(8);
	mq5 = ucx_mq_create(8);


	// start UCX/OS, non-preemptive mode
	return 1;
}
