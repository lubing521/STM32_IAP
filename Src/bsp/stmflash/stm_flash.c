/**
  ******************************************************************************
  * 文件名程: stm_flash.c 
  * 作    者: 
  * 版    本: V1.0
  * 编写日期: 2017-03-30
  * 功    能: 内部Falsh读写实现
  ******************************************************************************
/* 包含头文件 ----------------------------------------------------------------*/
#include "stmflash/stm_flash.h"

/* 私有类型定义 --------------------------------------------------------------*/
/* 私有宏定义 ----------------------------------------------------------------*/
#if STM32_FLASH_SIZE < 256
  #define STM_SECTOR_SIZE  1024 //字节
#else 
  #define STM_SECTOR_SIZE	 2048
#endif

#if STM32_FLASH_WREN	//如果使能了写 
static uint16_t STMFLASH_BUF [ STM_SECTOR_SIZE / 2 ];//最多是2K字节
#endif

/* 私有变量 ------------------------------------------------------------------*/
uint32_t FirstSector = 0;
uint32_t SectorError = 0;
static FLASH_EraseInitTypeDef EraseInitStruct;

/* 扩展变量 ------------------------------------------------------------------*/
/* 私有函数原形 --------------------------------------------------------------*/
static uint32_t GetSector(uint32_t Address);

/* 函数体 --------------------------------------------------------------------*/
/**
  * 函数功能: 读取指定地址的半字(16位数据)
  * 输入参数: faddr:读地址(此地址必须为2的倍数!!)
  * 返 回 值: 返回值:对应数据.
  * 说    明：无
  */
uint16_t STMFLASH_ReadHalfWord ( uint32_t faddr )
{
	return *(__IO uint16_t*)faddr; 
}

#if STM32_FLASH_WREN	//如果使能了写   
/**
  * 函数功能: 不检查的写入
  * 输入参数: WriteAddr:起始地址
  *           pBuffer:数据指针
  *           NumToWrite:半字(16位)数
  * 返 回 值: 无
  * 说    明：无
  */
void STMFLASH_Write_NoCheck ( uint32_t WriteAddr, uint16_t * pBuffer, uint16_t NumToWrite )   
{ 			 		 
	uint16_t i;	
	
	for(i=0;i<NumToWrite;i++)
	{
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,WriteAddr,pBuffer[i]);
	  WriteAddr+=2;                                    //地址增加2.
	}  
} 


/**
  * 函数功能: 从指定地址开始写入指定长度的数据
  * 输入参数: WriteAddr:起始地址(此地址必须为2的倍数!!)
  *           pBuffer:数据指针
  *           NumToWrite:半字(16位)数(就是要写入的16位数据的个数.)
  * 返 回 值: 无
  * 说    明：无
  */
void STMFLASH_Write ( uint32_t WriteAddr, uint16_t * pBuffer, uint16_t NumToWrite )	
{
	uint16_t secoff;	   //扇区内偏移地址(16位字计算)
	uint16_t secremain; //扇区内剩余地址(16位字计算)	   
 	uint16_t i;    
	uint32_t secpos;	   //扇区地址
	uint32_t offaddr;   //去掉0X08000000后的地址
	
	if(WriteAddr<FLASH_BASE||(WriteAddr>=(FLASH_BASE+1024*STM32_FLASH_SIZE)))return;//非法地址
	
	HAL_FLASH_Unlock();						//解锁
	
	offaddr=WriteAddr-FLASH_BASE;		//实际偏移地址.
	secpos=offaddr/STM_SECTOR_SIZE;			//扇区地址 
	secoff=(offaddr%STM_SECTOR_SIZE)/2;		//在扇区内的偏移(2个字节为基本单位.)
	secremain=STM_SECTOR_SIZE/2-secoff;		//扇区剩余空间大小   
	if(NumToWrite<=secremain)secremain=NumToWrite;//不大于该扇区范围
	
	while(1) 
	{	
		STMFLASH_Read(secpos*STM_SECTOR_SIZE+FLASH_BASE,STMFLASH_BUF,STM_SECTOR_SIZE/2);//读出整个扇区的内容
		for(i=0;i<secremain;i++)//校验数据
		{
			if(STMFLASH_BUF[secoff+i]!=0XFFFF)break;//需要擦除  	  
		}
		if(i<secremain)//需要擦除
		{
      /* 获取要擦除的首个扇区 */
      FirstSector = GetSector(WriteAddr);
      /* 初始化擦除结构体 */
      EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;  //擦除类型，扇区擦除 
      EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3; //电压范围，VCC=2.7~3.6V
      EraseInitStruct.Sector = FirstSector;                 //要擦除的扇区
      EraseInitStruct.NbSectors = 4;                        //一次只擦除一个扇区
      if(HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) == HAL_OK)
      { 
         printf("擦除完成\n");
      }
      for(i=0;i<secremain;i++)//复制
      {
          STMFLASH_BUF[i+secoff]=pBuffer[i];	  
      }
      STMFLASH_Write_NoCheck(secpos*STM_SECTOR_SIZE+FLASH_BASE,STMFLASH_BUF,STM_SECTOR_SIZE/2);//写入整个扇区  
      }
      else STMFLASH_Write_NoCheck(WriteAddr,pBuffer,secremain);//写已经擦除了的,直接写入扇区剩余区间. 				   
      if(NumToWrite==secremain)
      {
        break;//写入结束了
      }
      else//写入未结束
      {
          secpos++;				//扇区地址增1
          secoff=0;				//偏移位置为0 	 
          pBuffer+=secremain;  	//指针偏移
          WriteAddr+=secremain;	//写地址偏移	   
          NumToWrite-=secremain;	//字节(16位)数递减
          if(NumToWrite>(STM_SECTOR_SIZE/2))secremain=STM_SECTOR_SIZE/2;//下一个扇区还是写不完
          else secremain=NumToWrite;//下一个扇区可以写完了
        }	 
        };	
	HAL_FLASH_Lock();//上锁
}
#endif

/**
  * 函数功能: 从指定地址开始读出指定长度的数据
  * 输入参数: ReadAddr:起始地址
  *           pBuffer:数据指针
  *           NumToRead:半字(16位)数
  * 返 回 值: 无
  * 说    明：无
  */
void STMFLASH_Read ( uint32_t ReadAddr, uint16_t *pBuffer, uint16_t NumToRead )   	
{
	uint16_t i;
	
	for(i=0;i<NumToRead;i++)
	{
		pBuffer[i]=STMFLASH_ReadHalfWord(ReadAddr);//读取2个字节.
		ReadAddr+=2;//偏移2个字节.	
	}
}

/**
  * 函数功能: 向内部flash写入数据测试
  * 输入参数: WriteAddr:起始地址
  *           WriteData:要写入的数据
  * 返 回 值: 无
  * 说    明：无
  */
void Test_Write( uint32_t WriteAddr, uint16_t WriteData )   	
{
	STMFLASH_Write(WriteAddr,&WriteData,1);//写入一个字 
}

/**
  * 函数功能: 根据输入的地址给出它所在的sector
  * 输入参数: Address flash地址
  * 返 回 值: 无
  * 说    明: 无
  */
static uint32_t GetSector(uint32_t Address)
{
  uint32_t sector = 0;
  
  if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
  {
    sector = FLASH_SECTOR_0;  
  }
  else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
  {
    sector = FLASH_SECTOR_1;  
  }
  else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
  {
    sector = FLASH_SECTOR_2;  
  }
  else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
  {
    sector = FLASH_SECTOR_3;  
  }
  else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
  {
    sector = FLASH_SECTOR_4;  
  }
  else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
  {
    sector = FLASH_SECTOR_5;  
  }
  else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
  {
    sector = FLASH_SECTOR_6;  
  }
  else if((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7))
  {
    sector = FLASH_SECTOR_7;  
  }
  else if((Address < ADDR_FLASH_SECTOR_9) && (Address >= ADDR_FLASH_SECTOR_8))
  {
    sector = FLASH_SECTOR_8;  
  }
  else if((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9))
  {
    sector = FLASH_SECTOR_9;  
  }
  else if((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10))
  {
    sector = FLASH_SECTOR_10;  
  }
  else 
  {
    sector = FLASH_SECTOR_11;  
  }

  return sector;
}

/******************* (C) COPYRIGHT 2015-2020 硬石嵌入式开发团队 *****END OF FILE****/
