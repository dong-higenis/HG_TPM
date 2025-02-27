#include "adc.h"
#include "cli.h"




#ifdef _USE_HW_CLI
static void cliAdc(cli_args_t *args);
#endif


extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;


static bool is_init = false;

static uint16_t adc_data_buf[ADC_MAX_CH];





bool adcInit(void)
{
  bool ret = true;

  if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK)
  {    
    ret = false;
    logPrintf("adcInit() calibration start failed\n");
  }

  if (HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&adc_data_buf[0], hadc1.Init.NbrOfConversion) != HAL_OK)
  {
    ret = false;
    logPrintf("adcInit() dma start failed\n");
  }

  is_init = ret;

#ifdef _USE_HW_CLI
  cliAdd("adc", cliAdc);
#endif
  return ret;
}

bool adcIsInit(void)
{
  return is_init;
}

int32_t adcRead(uint8_t ch)
{
  return adc_data_buf[ch];
}

int32_t adcRead8(uint8_t ch)
{
  return adcRead(ch)>>4;
}

int32_t adcRead10(uint8_t ch)
{
  return adcRead(ch)>>2;
}

int32_t adcRead12(uint8_t ch)
{
  return adcRead(ch)>>0;
}

int32_t adcRead16(uint8_t ch)
{
  return adcRead(ch)<<4;
}

uint8_t adcGetRes(uint8_t ch)
{
  return 12;
}

float adcReadVoltage(uint8_t ch)
{
  return adcConvVoltage(ch, adcRead(ch));
}

float adcConvVoltage(uint8_t ch, uint32_t adc_value)
{
  float ret = 0;


  switch (ch)
  {
    default :
      ret  = ((float)adc_value * 3.3f) / (4095.f);
      break;
  }

  return ret;
}

#ifdef _USE_HW_CLI
void cliAdc(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "info") == true)
  {
    cliPrintf("adc init : %s\n", is_init? "True" : "False");
    cliPrintf("adc res  : %d\n", adcGetRes(0));
    for (int i=0; i<ADC_MAX_CH; i++)
    {
      cliPrintf("%02d. : %04d\n", i, adcRead(i));
    }

    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "show") == true)
  {
    cliShowCursor(false);
    while(cliKeepLoop())
    {
      for (int i=0; i<ADC_MAX_CH; i++)
      {
        cliPrintf("%02d. : %04d \n", i, adcRead(i));
      }
      delay(50);
      cliPrintf("\x1B[%dA", ADC_MAX_CH);
    }
    cliPrintf("\x1B[%dB", ADC_MAX_CH);
    cliShowCursor(true);
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "show") && args->isStr(1, "vol"))
  {
    cliShowCursor(false);
    while(cliKeepLoop())
    {
      for (int i=0; i<ADC_MAX_CH; i++)
      {
        float adc_data;

        adc_data = adcReadVoltage(i);

        cliPrintf("%02d. : %d.%02dV \n",i, (int)adc_data, (int)(adc_data * 100)%100);
      }
      delay(50);
      cliPrintf("\x1B[%dA", ADC_MAX_CH);
    }
    cliPrintf("\x1B[%dB", ADC_MAX_CH);
    cliShowCursor(true);
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "show") && args->isStr(1, "input"))
  {
    cliShowCursor(false);
    while(cliKeepLoop())
    {
      for (int i=0; i<ADC_MAX_CH; i++)
      {
        float vol;
        float vol_calc;

        vol = adcReadVoltage(i);

        vol_calc = vol * 503.f/33.f ; // 회로에 맞게 변환 된 전압

        cliPrintf("%02d. : %d.%02dV \n",i, (int)vol_calc, (int)(vol_calc * 100)%100);
      }
      delay(50);
      cliPrintf("\x1B[%dA", ADC_MAX_CH);
    }
    cliPrintf("\x1B[%dB", ADC_MAX_CH);
    cliShowCursor(true);
    ret = true;
  }

  if (ret != true)
  {
    cliPrintf("adc info\n");
    cliPrintf("adc show\n");
    cliPrintf("adc show vol\n");
    cliPrintf("adc show input\n");
  }
}
#endif

