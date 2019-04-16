/**
  ******************************************************************************
  * �ļ�����: main.c 
  * ��    ��: 
  * ��    ��: V1.0
  * ��д����: 2018-11-22
  * ��    ��: ����IAP����ʵ��
  ******************************************************************************
*/
#include "stm32f4xx_hal.h"
#include "string.h"
#include "usart/bsp_debug_usart.h"
#include "stmflash/stm_flash.h"
#include "IAP/bsp_iap.h"
#include "crc16.h"
#include "spiflash/bsp_spiflash.h"

/* ˽�����Ͷ��� --------------------------------------------------------------*/
/* ˽�к궨�� ----------------------------------------------------------------*/
/* ˽�б��� ------------------------------------------------------------------*/
uint8_t Rxdata = 0;
uint8_t recv_flag = 0;//������ɸ��±�־
uint8_t updata_flag = 0;//��¼�ȴ�����ʱ��
uint8_t flag = 0; //���Զ˿ڿ��ƽ���ɰ�����־
uint16_t recv = 0;//��¼���յ���׿���ݵ��ֽ���
uint8_t android_flag = 0;//���յ���׿����&��־λ 0��ʾû�н��յ� 1��ʾ���յ���Ȼ��ʼ���ո���
uint8_t FLAG = 1;
uint16_t crcValue = 1;
uint16_t crcValueTrue = 1;
char ErrorInfo[100] = {0};

/* ��չ���� ------------------------------------------------------------------*/
/* ˽�к���ԭ�� --------------------------------------------------------------*/
/* ������ --------------------------------------------------------------------*/
/**
  * ��������: ϵͳʱ������
  * �������: ��
  * �� �� ֵ: ��
  * ˵    ��: ��
  */
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
 
  __HAL_RCC_PWR_CLK_ENABLE();                                     //ʹ��PWRʱ��

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);  //���õ�ѹ�������ѹ����1

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;      // �ⲿ����8MHz
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;                        //��HSE 
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;                    //��PLL
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;            //PLLʱ��Դѡ��HSE
  RCC_OscInitStruct.PLL.PLLM = 8;                                 //8��ƵMHz
  RCC_OscInitStruct.PLL.PLLN = 336;                               //336��Ƶ
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;                     //2��Ƶ���õ�168MHz��ʱ��
  RCC_OscInitStruct.PLL.PLLQ = 7;                                 //USB/SDIO/������������ȵ���PLL��Ƶϵ��
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;       // ϵͳʱ�ӣ�168MHz
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;              // AHBʱ�ӣ� 168MHz
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;               // APB1ʱ�ӣ�42MHz
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;               // APB2ʱ�ӣ�84MHz
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);

  HAL_RCC_EnableCSS();                                            // ʹ��CSS���ܣ�����ʹ���ⲿ�����ڲ�ʱ��ԴΪ����
  
 	// HAL_RCC_GetHCLKFreq()/1000    1ms�ж�һ��
	// HAL_RCC_GetHCLKFreq()/100000	 10us�ж�һ��
	// HAL_RCC_GetHCLKFreq()/1000000 1us�ж�һ��
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);                // ���ò�����ϵͳ�δ�ʱ��
  /* ϵͳ�δ�ʱ��ʱ��Դ */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* ϵͳ�δ�ʱ���ж����ȼ����� */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}
void Log()
{
    printf("��־��Ϣ:%s",ErrorInfo);
    uint8_t log[110] = {0};
    
    strcat(log,"<");
    strcat(log,ErrorInfo);
    strcat(log,">");
           
    HAL_UART_Transmit(&husart_debug,log,strlen((char *)log),1000);
           
    return;
}

void HexStrToByte(const char* source, unsigned char* dest, int sourceLen)
{
    short i;
    unsigned char highByte, lowByte;
    
    for (i = 0; i < sourceLen; i += 2)
    {
        highByte = toupper(source[i]);
        lowByte  = toupper(source[i + 1]);
 
        if (highByte > 0x39)
            highByte -= 0x37;
        else
            highByte -= 0x30;
 
        if (lowByte > 0x39)
            lowByte -= 0x37;
        else
            lowByte -= 0x30;
 
        dest[i / 2] = (highByte << 4) | lowByte;
    }
    return ;
} 

/**
  * ��������: ������.
  * �������: ��
  * �� �� ֵ: ��
  * ˵    ��: ��
  */
int main(void)
{
    /* ��λ�������裬��ʼ��Flash�ӿں�ϵͳ�δ�ʱ�� */
    HAL_Init();
    /* ����ϵͳʱ�� */
    SystemClock_Config();
    /* ��ʼ�����ڲ����ô����ж����ȼ� */
    MX_DEBUG_USART_Init();
    MX_SPIFlash_Init();
    MX_USARTx_Init();
    HAL_UART_Receive_IT(&husart_debug,&Rxdata,1);
    HAL_UART_Receive_IT(&husartx,&Rxdata,1);
    printf("ϵͳ������������\n�ȴ�60*10s��,û�н��յ�����ָ����ִ�оɰ�APP");
    printf("�����Ҫ���� APP���뷢�� APP �̼���.bin�ļ���\n");
    recv = strAppBin .usLength;
    
    uint8_t temp_8_t[5] = {0}; 
    unsigned char CrcTemp[2] = {0};
    /*
    android_flag = 1;
    recv_flag = 0;
    strAppBin .usLength = 0;
    */
    
    
    /* ����ѭ�� */
    while(1)                            
    {	
	       
        //���յ���׿&���ҽ����������
        if(recv_flag && android_flag)             
        {
            printf("\nAPP ���ȣ�%d�ֽ�\n", strAppBin .usLength );				
            printf("��ʼ���� APP\n");
	    sprintf(ErrorInfo,"BootLoader:app download finish\n");
            Log();
            //��֤����
            crcValue = CRC16_MODBUS(strAppBin .ucDataBuf,strAppBin .usLength);
	    printf("��֤���:%x\n",crcValue);
            sprintf(ErrorInfo,"BootLoader:CRCValue = %x\n",crcValue);
            Log();
            SPI_FLASH_BufferRead(temp_8_t, FLASH_CRC_CODE,10);
            printf ( "CRC��֤��:%c%c%c%c\n",temp_8_t[0],temp_8_t[1],temp_8_t[2],temp_8_t[3]);
            temp_8_t[4] = '\0';
            HexStrToByte(temp_8_t,CrcTemp,4);
	    crcValueTrue = CrcTemp[1] + CrcTemp[0] * 256;
	    printf ( "CRC��֤��:%x\n",crcValueTrue);
            sprintf(ErrorInfo,"BootLoader:crcValueTrue = %x\n",crcValueTrue);
            Log();
            //if(1)
	    if(crcValueTrue == crcValue)
            {
                //����FLASH����  
                printf ( "������֤�ɹ�\n" ); 
		IAP_Write_App_Bin(APP_START_ADDR, strAppBin .ucDataBuf, strAppBin .usLength );
                printf ( "�ȴ�1s��ʼִ��APP\n" );
                printf ( "��ʼִ�� APP\n" ); 
		sprintf(ErrorInfo,"BootLoader:CRC verification success\n");
                Log();
                recv_flag = 0;  
            }
            else
            {
                printf ( "������֤ʧ�ܣ�ִ�оɰ����\n" );
                sprintf(ErrorInfo,"BootLoader:CRC verification fail\n");
                Log();
                HAL_UART_Transmit(&husart_debug,")",1,1000);
            }
            //ִ��FLASH APP����
            IAP_ExecuteApp(APP_START_ADDR);  
        }
        //�ȴ�10s֮��û���յ�&�������ɰ����
        else if(android_flag == 0 && updata_flag >= 10)
        {
            flag = 1;
            updata_flag = 0;
        }
        else
        {
            //û�н��յ�&������С��10s��
            if(!android_flag)
            {
                HAL_Delay(1000);
                updata_flag++;
                printf("%d %02x\n",updata_flag,flag);
            }
            //�յ�&������û�н������
            else
            {
            }
        }
        if(flag)
        {
            printf("ִ�оɰ�APP\n");
            updata_flag = 0;
	    IAP_ExecuteApp(APP_START_ADDR);
        }
        //���ڼ����������ݣ����յ������ۼ�����
        if(recv != strAppBin .usLength && strAppBin .usLength != 0)
        {
            recv = strAppBin .usLength;
        }
        //û�н��յ�
        else if(strAppBin .usLength == 0)
        {
            recv_flag = 0;
        }
	//�������
        else
        {
            HAL_Delay(1000);
            if(recv == strAppBin .usLength)
            {
                recv_flag = 1;
            }
        }
    }  
}

/**
  * ��������: ���ڽ�����ɻص�����
  * �������: ��
  * �� �� ֵ: ��
  * ˵    ������
  */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{
  
  //&��β����ʾ�������
  if(UartHandle->Instance == DEBUG_USARTx)
  {
      strAppBin.ucDataBuf[strAppBin .usLength] = Rxdata;
      strAppBin .usLength ++;
      HAL_UART_Receive_IT(&husart_debug,&Rxdata,1);
     // printf("%02x %d\n",Rxdata,strAppBin .usLength);
      //printf("%c ",Rxdata);
      //����ϵͳǰ��Ҫ����&���ţ�ȷ�ϣ����������͵�����Ϊ���³���
      if(Rxdata == '&' && strAppBin .usLength < 10 && FLAG)
      {
          printf("���յ�%c����ʼ���ո�����Ϣ\n",Rxdata);
          android_flag = 1;
          recv_flag = 0;
          strAppBin .usLength = 0;
          FLAG = 0;
      }
  }
  //���յ����Զ˿ڷ��������ַ������ɰ����
  if(UartHandle->Instance == USARTx)
  {
      printf("������%02x\n",Rxdata);
      flag = 1;
      HAL_UART_Receive_IT(&husartx,&Rxdata,1);
  }
}

