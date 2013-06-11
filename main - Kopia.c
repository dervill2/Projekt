#include "main.h"

int music1=sizeof(AUDIO_SAMPLE1)/2;
int music2=sizeof(AUDIO_SAMPLE2)/2;
int music3=sizeof(AUDIO_SAMPLE3)/2;
int music4=sizeof(AUDIO_SAMPLE4)/2;
int button=-1;

void przerywanie_zewn()
{

	NVIC_InitTypeDef NVIC_InitStructure;
	// numer przerwania
	NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
	// priorytet g³ówny
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;
	// subpriorytet
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;
	// uruchom dany kana³
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	// zapisz wype³nion¹ strukturê do rejestrów
	NVIC_Init(&NVIC_InitStructure);


	// wybór numeru aktualnie konfigurowanej linii przerwañ
	EXTI_InitStructure.EXTI_Line = GPIO_Pin_0;
	// wybór trybu - przerwanie b¹dŸ zdarzenie
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	// wybór zbocza, na które zareaguje przerwanie
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
	// uruchom dan¹ liniê przerwañ
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	// zapisz strukturê konfiguracyjn¹ przerwañ zewnêtrznych do rejestrów
	EXTI_Init(&EXTI_InitStructure);

	// pod³¹czenie danego pinu portu do kontrolera przerwañ
	SYSCFG_EXTILineConfig(GPIOA, GPIO_Pin_0);

}

//Zewnêtrzne przerywaanie:
void EXTI0_IRQHandler(void)
{
		if(EXTI_GetITStatus(EXTI_Line0) != RESET)
		{

		TIM_Cmd ( TIM2 , ENABLE ) ;

		// miejsce na kod wywo³ywany w momencie wyst¹pienia przerwania
		// wyzerowanie flagi wyzwolonego przerwania
		EXTI_ClearITPendingBit(EXTI_Line0);
		}
}

void timer()
{
	if(TIM_GetCounter(TIM2)==99)
	{
		if(  GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_0)==1  )
		{

			button++;		if(button>4)button=0;

			if(button==0)
			{
				GPIOD->BSRRL = GPIO_Pin_12;
				GPIOD->BSRRH = GPIO_Pin_15;
			}

			if(button==1)
			{
				GPIOD->BSRRL = GPIO_Pin_13;
				GPIOD->BSRRH = GPIO_Pin_12;

			}

			if(button==2)
			{
				GPIOD->BSRRL = GPIO_Pin_14;
				GPIOD->BSRRH = GPIO_Pin_13;
			}

			if(button==3)
			{
				GPIOD->BSRRL = GPIO_Pin_15;
				GPIOD->BSRRH = GPIO_Pin_14;
			}
			if(button==4)
			{
				GPIOD->BSRRH = GPIO_Pin_15;
			}


			sampleCounter=0;

			TIM_SetCounter(TIM2,0);
			TIM_Cmd ( TIM2 , DISABLE ) ;
		}
	}
}


void tim2()
{
	RCC_APB1PeriphClockCmd ( RCC_APB1Periph_TIM2 , ENABLE ) ; // uruchomienie zegara dla Timera 2

	// taktow ani e FCLK=72MHz/7200=10kHz
		TIM_TimeBaseStructure . TIM_Prescaler = 8400;
		// p r z e p e l n i e n i e l i c z n i k a 10000 taktów = 1 s
		TIM_TimeBaseStructure . TIM_Period = 100;
		// z l i c z a n i e w gore
		TIM_TimeBaseStructure . TIM_CounterMode = TIM_CounterMode_Up ;
		// l i c z n i k powtorzen
		TIM_TimeBaseStructure . TIM_RepetitionCounter = 0 ;
		// i n i c j a l i z a c j a l i c z n i k a 2
		TIM_TimeBaseInit ( TIM2 , &TIM_TimeBaseStructure ) ;
}

void gpio_inicjacja()
{
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

	GPIO_Init(GPIOD, &GPIO_InitStructure);

	//Button
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

int main(void)
{
	SystemInit();

	gpio_inicjacja();
	tim2();
	przerywanie_zewn();


	codec_init();
	codec_ctrl_init();

	I2S_Cmd(CODEC_I2S, ENABLE);


while(1)
    {

timer();

if(button==0)
{
    	if (SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE))
    	{
    		sample=AUDIO_SAMPLE1[sampleCounter];

    		SPI_I2S_SendData(CODEC_I2S, sample);

    		sampleCounter++;
  		}

    	if (sampleCounter==music1/2)
    	{
			GPIOD->BSRRH = GPIO_Pin_12;
    	}
    	else if (sampleCounter == music1)
    	{
			GPIOD->BSRRL = GPIO_Pin_12;
    		sampleCounter = 0;
    		button=-1;
    	}

}

	   if(button==1)
        {
        	if (SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE))
        	{
        		sample=AUDIO_SAMPLE2[sampleCounter];

        		SPI_I2S_SendData(CODEC_I2S, sample);

        		sampleCounter++;
      		}

        	if (sampleCounter==music2/2)
        	{
				GPIOD->BSRRH = GPIO_Pin_12;
        	}
        	else if (sampleCounter == music2)
        	{
				GPIOD->BSRRL = GPIO_Pin_12;
        		sampleCounter = 0;
        		button=-1;
        	}
        }


if(button==2)
        {

        	if (SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE))
        	{
        		sample=AUDIO_SAMPLE3[sampleCounter];

        		SPI_I2S_SendData(CODEC_I2S, sample);
        		SPI_I2S_SendData(CODEC_I2S, sample);
        		sampleCounter++;
      		}

        	if (sampleCounter==music3/2)
        	{
				GPIOD->BSRRH = GPIO_Pin_12;
        	}
        	else if (sampleCounter == music3)
        	{
				GPIOD->BSRRL = GPIO_Pin_12;
        		sampleCounter = 0;
        		button=-1;
        	}
        }

    ///MUSIC4
if(button==4)
        {
        	if (SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE))
        	{
        		sample=AUDIO_SAMPLE4[sampleCounter];

        		SPI_I2S_SendData(CODEC_I2S, sample);

        		sampleCounter++;
      		}

        	if (sampleCounter==music4/2)
        	{
				GPIOD->BSRRH = GPIO_Pin_12;
        	}
        	else if (sampleCounter == music4)
        	{
				GPIOD->BSRRL = GPIO_Pin_12;
        		sampleCounter = 0;
        		button=-1;
        	}
        }

}

}


