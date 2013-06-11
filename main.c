#include "main.h"
#include "pdm_filter.h"
#include "codec.h"

#define Przerywanie_od_MP45DT02          SPI2_IRQHandler

	I2S_InitTypeDef I2S_InitType;


int button=0;
int Switch=0;
int counter=0;
uint16_t *writebuffer;
int aktyw=0;
int lock=0;

int aktyw_low=0;
int aktyw_hi=0;
int lock_low=0;
int lock_hi=0;

//Deklaracje funkcji u¿ytych w póŸniejszym czasie:
static void Inicjacja_GPIO_dla_MP45DT02(void);
static void Konfiguracja_SPI(uint32_t Freq);
static void Konfiguracja_przerywan_NVIC(void);

PDMFilter_InitStruct Filter;

/* Current state of the audio recorder interface intialization */
static uint32_t AudioRecInited = 0;

uint32_t AudioRecBitRes = 16;
__IO uint32_t Data_Status =0;

/* Audio recording number of channels (1 for Mono or 2 for Stereo) */
uint32_t AudioRecChnlNbr = 1;

/* Main buffer pointer for the recorded data storing */
uint16_t* pAudioRecBuf;

/* Current size of the recorded buffer */
uint32_t AudioRecCurrSize = 0;

uint16_t RecBuf[16], RecBuf1[16];

int RecBufIter=0;
#define SIZE_GENERAL_BUF 63500
uint16_t RecBufPCM[SIZE_GENERAL_BUF];

int iterator=0;

/* Temporary data sample */
int INTERNAL_BUFF_SIZE=64;
static uint16_t InternalBuffer[64];
static uint32_t InternalBufferSize = 0;


/////////////////


/////////////////////////////////////////MIC///////////////////////////////////////////


static void Inicjacja_GPIO_dla_MP45DT02(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable GPIO clocks */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC, ENABLE);

  /* Enable GPIO clocks */
 RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC, ENABLE);

  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

  /* SPI SCK pin configuration */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  /* Connect SPI pins to AF5 */
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_SPI2);

  /* SPI MOSI pin configuration */
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_3;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource3, GPIO_AF_SPI2);
}

static void Konfiguracja_SPI(uint32_t Freq)
{
  I2S_InitTypeDef I2S_InitStructure;

  /* Enable the SPI clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2,ENABLE);

  /* SPI configuration */
  SPI_I2S_DeInit(SPI2);
  I2S_InitStructure.I2S_AudioFreq = 24000;
  I2S_InitStructure.I2S_Standard = I2S_Standard_LSB;
  I2S_InitStructure.I2S_DataFormat = I2S_DataFormat_16b;
  I2S_InitStructure.I2S_CPOL = I2S_CPOL_High;
  I2S_InitStructure.I2S_Mode = I2S_Mode_MasterRx;
  I2S_InitStructure.I2S_MCLKOutput = I2S_MCLKOutput_Disable;
  /* Initialize the I2S peripheral with the structure above */
  I2S_Init(SPI2, &I2S_InitStructure);

  /* Enable the Rx buffer not empty interrupt */
  SPI_I2S_ITConfig(SPI2, SPI_I2S_IT_RXNE, ENABLE);
}


uint32_t WaveRecorderInit(uint32_t AudioFreq, uint32_t BitRes, uint32_t ChnlNbr)
{
  /* Check if the interface is already initialized */
  if (AudioRecInited)
  {
    /* No need for initialization */
    return 0;
  }
  else
  {
    /* Enable CRC module */
    RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;

    /* Filter LP & HP Init */
    Filter.LP_HZ = 8000;
    Filter.HP_HZ = 10;
    Filter.Fs = 16000;
    Filter.Out_MicChannels = 1;
    Filter.In_MicChannels = 1;

    PDM_Filter_Init((PDMFilter_InitStruct *)&Filter);

    /* Konfiguracja GPIO dla uk³adu MP45DT02 zawieraj¹cego mikrofon wielokierunkowy */
    Inicjacja_GPIO_dla_MP45DT02();

    /*Konfiguracja przerywañ*/
    Konfiguracja_przerywan_NVIC();

    /* Configure the SPI */
    Konfiguracja_SPI(AudioFreq);

    /* Set the local parameters */
    AudioRecBitRes = BitRes;
    AudioRecChnlNbr = ChnlNbr;

    /* Set state of the audio recorder to initialized */
    AudioRecInited = 1;


    /* Return 0 if all operations are OK */
    return 0;
  }
}

uint8_t WaveRecorderStart(uint16_t* pbuf, uint32_t size)
{
/* Check if the interface has already been initialized */
  if (AudioRecInited)
  {
    /* Store the location and size of the audio buffer */
    pAudioRecBuf = pbuf;
    AudioRecCurrSize = size;

    /* Enable the Rx buffer not empty interrupt */
    SPI_I2S_ITConfig(SPI2, SPI_I2S_IT_RXNE, ENABLE);

    /* The Data transfer is performed in the SPI interrupt routine */
    /* Enable the SPI peripheral */
    I2S_Cmd(SPI2, ENABLE);
    /* Return 0 if all operations are OK */
    return 0;
  }
  else
  {
    /* Cannot perform operation */
    return 1;
  }
}

uint32_t WaveRecorderStop(void)
{
  /* Check if the interface has already been initialized */
  if (AudioRecInited)
  {

    /* Stop conversion */
    I2S_Cmd(SPI2, DISABLE);

    /* Return 0 if all operations are OK */
    return 0;
  }
  else
  {
    /* Cannot perform operation */
    return 1;
  }
}


void Przerywanie_od_MP45DT02(void)
{

	u16 app;
	u16 volume;

  /* Check if data are available in SPI Data register */
  if (SPI_GetITStatus(SPI2, SPI_I2S_IT_RXNE) != RESET)
  {

	  app = SPI_I2S_ReceiveData(SPI2);
	  InternalBuffer[InternalBufferSize++] = HTONS(app);


	    if (InternalBufferSize >= INTERNAL_BUFF_SIZE)
	    {
	      InternalBufferSize = 0;

	      volume = 20;

	      PDM_Filter_64_LSB((uint16_t *)InternalBuffer, (uint16_t *)pAudioRecBuf, volume , (PDMFilter_InitStruct *)&Filter);
	      Data_Status = 1;

	      if (Switch ==1)
	     {
	       pAudioRecBuf = RecBuf;
	       writebuffer = RecBuf1;
	       Switch = 0;
	     }
	      else
	      {
	        pAudioRecBuf = RecBuf1;
	        writebuffer = RecBuf;
	        Switch = 1;
	      }

	      for(counter=0;counter<16;counter++)
	      {
	      	if(Switch==0)
	      		RecBufPCM[RecBufIter]=RecBuf1[counter];
	      	if(Switch==1)
	          	RecBufPCM[RecBufIter]=RecBuf[counter];

	      	RecBufIter++;
		      if(RecBufIter==SIZE_GENERAL_BUF)
		      	{
		      	RecBufIter=0;
					if(button==3 || button==2)
					{
						WaveRecorderStop();
					}
		      	if(button==3)aktyw_hi=1;
		      	if(button==2)aktyw_low=1;
		      	}
	      }





	    }

  }

}

static void Konfiguracja_przerywan_NVIC(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;

  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_3);
  /* Configure the SPI interrupt priority */
  NVIC_InitStructure.NVIC_IRQChannel = SPI2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

}

/////////////////////////////////////////MIC///////////////////////////////////////////

/* Przerwanie od przycisniêcia przycisku */
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

			I2S_Cmd(CODEC_I2S, DISABLE);
			button++;
			if(button>3)button=0;


			WaveRecorderStop();
			sampleCounter=0;
			RecBufIter=0;
			InternalBufferSize = 0;
			Switch = 0;

			WaveRecorderStart(RecBuf, 16);

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

//////////////////////////////

int main(void)
{
	SystemInit();

	gpio_inicjacja();
	tim2();
	przerywanie_zewn();

	//WaveRecorderInit(uint32_t AudioFreq, uint32_t BitRes, uint32_t ChnlNbr);
	WaveRecorderInit(22000,16,1);

	codec_init();
	codec_ctrl_init();

	I2S_Cmd(CODEC_I2S, ENABLE);


while(1)
    {
loop:
timer();

if(button==1)
{
	GPIOD->BSRRL = GPIO_Pin_14;
	GPIOD->BSRRH = GPIO_Pin_13;
}

if(button==2)
{
	GPIOD->BSRRL = GPIO_Pin_15;
	GPIOD->BSRRH = GPIO_Pin_14;
}

if(button==0)
{
	GPIOD->BSRRL = GPIO_Pin_13;
	GPIOD->BSRRH = GPIO_Pin_12;
}

if(button==3)
{
	GPIOD->BSRRL = GPIO_Pin_12;
	GPIOD->BSRRH = GPIO_Pin_15;
}
if(button==1)
{

	//NAGRYWANIE

if(lock==0 && RecBufIter==16)
{
	I2S_Cmd(CODEC_I2S, ENABLE);
	lock=1;
	aktyw=1;
}

	   if (SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE) && aktyw==1)
	   {
		sample=RecBufPCM[sampleCounter];

		SPI_I2S_SendData(CODEC_I2S, sample);
		sampleCounter++;
		}

	if (sampleCounter==SIZE_GENERAL_BUF/2)
		{
			GPIOD->BSRRL = GPIO_Pin_12;
		}


	else if (sampleCounter == SIZE_GENERAL_BUF)
		{
			GPIOD->BSRRH = GPIO_Pin_12;
		sampleCounter = 0;
		}

    }

if(button==2 && aktyw_low==1)
{

	I2S_Cmd(CODEC_I2S, DISABLE);

	//ZMIANA G£OSU

	SPI_I2S_DeInit(CODEC_I2S);
	I2S_InitType.I2S_AudioFreq = I2S_AudioFreq_7k;//11
	I2S_InitType.I2S_MCLKOutput = I2S_MCLKOutput_Enable;
	I2S_InitType.I2S_DataFormat = I2S_DataFormat_16b;
	I2S_InitType.I2S_Mode = I2S_Mode_MasterTx;
	I2S_InitType.I2S_Standard = I2S_Standard_Phillips;
	I2S_InitType.I2S_CPOL = I2S_CPOL_Low;

	I2S_Init(CODEC_I2S, &I2S_InitType);
	I2S_Cmd(CODEC_I2S, ENABLE);

int i=0;
sampleCounter=0;
for(i=0;i<SIZE_GENERAL_BUF;)
{

	   if (SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE))
	   {
		sample=RecBufPCM[sampleCounter];

		SPI_I2S_SendData(CODEC_I2S, sample);
		sampleCounter++;
		i++;
		}

	if (sampleCounter==SIZE_GENERAL_BUF/2)
		{
			GPIOD->BSRRL = GPIO_Pin_12;
		}


	else if (sampleCounter == SIZE_GENERAL_BUF)
		{
			GPIOD->BSRRH = GPIO_Pin_12;
		sampleCounter = 0;
		}
}
SPI_I2S_SendData(CODEC_I2S, 0);
aktyw_low=0;
    }

if(button==3 && aktyw_hi==1)
{
	I2S_Cmd(CODEC_I2S, DISABLE);

	//ZMIANA G£OSU

	SPI_I2S_DeInit(CODEC_I2S);
	I2S_InitType.I2S_AudioFreq = I2S_AudioFreq_5k;//11
	I2S_InitType.I2S_MCLKOutput = I2S_MCLKOutput_Enable;
	I2S_InitType.I2S_DataFormat = I2S_DataFormat_16b;
	I2S_InitType.I2S_Mode = I2S_Mode_MasterTx;
	I2S_InitType.I2S_Standard = I2S_Standard_Phillips;
	I2S_InitType.I2S_CPOL = I2S_CPOL_Low;

	I2S_Init(CODEC_I2S, &I2S_InitType);
	I2S_Cmd(CODEC_I2S, ENABLE);

int i=0;
sampleCounter=0;
for(i=0;i<SIZE_GENERAL_BUF;)
{

	   if (SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE))
	   {
		sample=RecBufPCM[sampleCounter];

		SPI_I2S_SendData(CODEC_I2S, sample);
		sampleCounter++;
		i++;
		}


	else if (sampleCounter == SIZE_GENERAL_BUF)
		{
		sampleCounter = 0;
		}
}
aktyw_hi=0;
    }
    }

}

