/**
  ******************************************************************************
  * �ļ�����: bsp_iap.c 
  * ��    ��: 
  * ��    ��: V1.0
  * ��д����: 2018-11-22
  * ��    ��: IAP�ײ�����ʵ��
  ******************************************************************************

/* ����ͷ�ļ� ----------------------------------------------------------------*/
#include "IAP/bsp_iap.h"
#include "stmflash/stm_flash.h"
/* ˽�����Ͷ��� --------------------------------------------------------------*/
/* ˽�к궨�� ----------------------------------------------------------------*/
/* ˽�б��� ------------------------------------------------------------------*/

struct STRUCT_IAP_RECIEVE strAppBin;//={{0},0};

static uint16_t ulBuf_Flash_App[1024];

/* ��չ���� ------------------------------------------------------------------*/
/* ˽�к���ԭ�� --------------------------------------------------------------*/
/* ������ --------------------------------------------------------------------*/
void IAP_Write_App_Bin ( uint32_t ulStartAddr, uint8_t * pBin_DataBuf, uint32_t ulBufLength )
{
	uint16_t us, usCtr=0, usTemp;
	uint32_t ulAdd_Write = ulStartAddr;                                //��ǰд��ĵ�ַ
	uint8_t * pData = pBin_DataBuf;
	
        
        //printf("APP��ַ��%d\n",ulStartAddr);
        
	for (us = 0; us < ulBufLength; us += 2)
	{						    
          usTemp =  ( uint16_t ) pData[1]<<8;
          usTemp += ( uint16_t ) pData[0];	  
          pData += 2;                                                      //ƫ��2���ֽ�
          ulBuf_Flash_App [ usCtr ++ ] = usTemp;	    
          if ( usCtr == 1024 )
          {
            printf("APPд�������%d\n",ulAdd_Write);
            usCtr = 0;
            STMFLASH_Write ( ulAdd_Write, ulBuf_Flash_App, 1024 );	
            //STMFLASH_Write_NoCheck(ulAdd_Write, ulBuf_Flash_App, 1024);
	    ulAdd_Write += 2048;                                           //ƫ��2048  16=2*8.����Ҫ����2.
          }
	}
	if (usCtr) 
        { 
          STMFLASH_Write (ulAdd_Write,ulBuf_Flash_App,usCtr );//������һЩ�����ֽ�д��ȥ. 
          //STMFLASH_Write_NoCheck(ulAdd_Write, ulBuf_Flash_App, usCtr);
        }
}
/*
void Write_App_Bin_To_Flash ( uint32_t ulStartAddr, uint8_t * pBin_DataBuf, uint32_t ulBufLength )
{
	uint16_t us, usCtr=0, usTemp;
	uint32_t ulAdd_Write = ulStartAddr;                                //��ǰд��ĵ�ַ
	uint8_t * pData = pBin_DataBuf;
	
        
        printf("APP��ַ��%d\n",ulStartAddr);
        
	for (us = 0; us < ulBufLength; us += 2)
	{						    
          usTemp =  ( uint16_t ) pData[1]<<8;
          usTemp += ( uint16_t ) pData[0];	  
          pData += 2;                                                      //ƫ��2���ֽ�
          ulBuf_Flash_App [ usCtr ++ ] = usTemp;	    
          if ( usCtr == 1024 )
          {
            printf("APPд�������%d\n",ulAdd_Write);
            usCtr = 0;
            STMFLASH_Write ( ulAdd_Write, ulBuf_Flash_App, 1024 );	
            //STMFLASH_Write_NoCheck(ulAdd_Write, ulBuf_Flash_App, 1024);
	    ulAdd_Write += 2048;                                           //ƫ��2048  16=2*8.����Ҫ����2.
          }
	}
	if (usCtr) 
        { 
          STMFLASH_Write (ulAdd_Write,ulBuf_Flash_App,usCtr );//������һЩ�����ֽ�д��ȥ. 
          //STMFLASH_Write_NoCheck(ulAdd_Write, ulBuf_Flash_App, usCtr);
        }
}*/

void MSR_MSP ( uint32_t ulAddr ) 
{
    asm("MSR MSP, r0"); 			                   //set Main Stack value
    asm("BX r14");
}

//��ת��Ӧ�ó����
//ulAddr_App:�û�������ʼ��ַ.
void IAP_ExecuteApp ( uint32_t ulAddr_App )
{
	pIapFun_TypeDef pJump2App; 
	
	if ( ( ( * ( __IO uint32_t * ) ulAddr_App ) & 0x2FF00000 ) == 0x20000000 )	  //���ջ����ַ�Ƿ�Ϸ�.
	{ 
                printf("��ʼִ��\n");
		pJump2App = ( pIapFun_TypeDef ) * ( __IO uint32_t * ) ( ulAddr_App + 4 );	//�û��������ڶ�����Ϊ����ʼ��ַ(��λ��ַ)		
		MSR_MSP( * ( __IO uint32_t * ) ulAddr_App );			     //��ʼ��APP��ջָ��(�û��������ĵ�һ�������ڴ��ջ����ַ)
		pJump2App ();								  	//��ת��APP.
	}
}		



/******************* (C) COPYRIGHT 2015-2020 ӲʯǶ��ʽ�����Ŷ� *****END OF FILE****/
