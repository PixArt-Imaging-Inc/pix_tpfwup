#include "pjp274flashctrlr.h"

#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

using namespace pixart;


const int Pjp274FlashCtrlr::SRAM_BURST_SIZE = 0x00000100;
const int Pjp274FlashCtrlr::PAGE_SIZE = 256;
const int Pjp274FlashCtrlr::PAGE_COUNT_PERSECTOR = 16;
const int Pjp274FlashCtrlr::SECTOR_COUNT = 15;
const int Pjp274FlashCtrlr::PAGE_COUNT = SECTOR_COUNT * PAGE_COUNT_PERSECTOR;
const int Pjp274FlashCtrlr::SECTOR_SIZE = PAGE_COUNT_PERSECTOR * PAGE_SIZE;

const int Pjp274FlashCtrlr::FIRMWARE_START_PAGE = 0;
const int Pjp274FlashCtrlr::PARAMETER_START_PAGE = 224;

const byte Pjp274FlashCtrlr::IO_BANK = 6;

/* HW SFC register address */

const byte Pjp274FlashCtrlr::REG_SRAM_ACCESS_DATA = 0x0b;


Pjp274FlashCtrlr::Pjp274FlashCtrlr(RegisterAccessor* regAccessor) :
        FlashController(regAccessor)
{
}

Pjp274FlashCtrlr::~Pjp274FlashCtrlr()
{
}
bool Pjp274FlashCtrlr::flashframeStart()
{
  int retry = 0;
  int value=0;
  //frame start
   mRegAccessor->writeRegister(4, 0x56, 0x01);//frame start
   while(true)
   {
      usleep(1000);
       value = mRegAccessor->readRegister(4, 0x56);
       if(value==0)
         break;
       if(retry>10)
         return false;
       retry++;
   }
  return true;
}
bool Pjp274FlashCtrlr::writeEnable()
{
   int retry = 0;
   int value=0;
   // Flash Execute with WEN
   mRegAccessor->writeRegister(4, 0x2c, 0x00);//inst_cmd
   mRegAccessor->writeRegister(4, 0x40, 0x06);//CCR_cmd
   mRegAccessor->writeRegister(4, 0x41, 0x01);//CCR_cmd
   mRegAccessor->writeRegister(4, 0x42, 0x00);//CCR_cmd
   mRegAccessor->writeRegister(4, 0x43, 0x00);//CCR_cmd
   mRegAccessor->writeRegister(4, 0x44, 0x00);//data_cnt
   mRegAccessor->writeRegister(4, 0x45, 0x00);//data_cnt
   mRegAccessor->writeRegister(4, 0x46, 0x00);//data_cnt
   mRegAccessor->writeRegister(4, 0x47, 0x00);//data_cnt
   flashframeStart();
   value=0;
   while(true)
   {
     // Flash Execute with RDSR
     mRegAccessor->writeRegister(4, 0x2c, 0x01);//inst_cmd
     mRegAccessor->writeRegister(4, 0x40, 0x05);//CCR_cmd
     mRegAccessor->writeRegister(4, 0x41, 0x01);//CCR_cmd
     mRegAccessor->writeRegister(4, 0x42, 0x00);//CCR_cmd
     mRegAccessor->writeRegister(4, 0x43, 0x01);//CCR_cmd
     mRegAccessor->writeRegister(4, 0x44, 0x01);//data_cnt
     mRegAccessor->writeRegister(4, 0x45, 0x00);//data_cnt
     mRegAccessor->writeRegister(4, 0x46, 0x00);//data_cnt
     mRegAccessor->writeRegister(4, 0x47, 0x00);//data_cnt
     flashframeStart();
     usleep(1000);
     value = mRegAccessor->readRegister(4, 0x1c);
     if((value&0x2)!=0)
      break;
     if(retry>10)
      return false;
     retry++;
   }
  return true;
}

bool Pjp274FlashCtrlr::checkBusy()
{
   int retry = 0;
   int value;
   while(true)
   {
     // Flash Execute with RDSR
     mRegAccessor->writeRegister(4, 0x2c, 0x01);//inst_cmd
     mRegAccessor->writeRegister(4, 0x40, 0x05);//CCR_cmd
     mRegAccessor->writeRegister(4, 0x41, 0x01);//CCR_cmd
     mRegAccessor->writeRegister(4, 0x42, 0x00);//CCR_cmd
     mRegAccessor->writeRegister(4, 0x43, 0x01);//CCR_cmd
     mRegAccessor->writeRegister(4, 0x44, 0x01);//data_cnt
     mRegAccessor->writeRegister(4, 0x45, 0x00);//data_cnt
     mRegAccessor->writeRegister(4, 0x46, 0x00);//data_cnt
     mRegAccessor->writeRegister(4, 0x47, 0x00);//data_cnt

     flashframeStart();
     usleep(1000);
     value = mRegAccessor->readRegister(4, 0x1c);
     if((value&0x1)==0)
      break;
     if(retry>1000)
      return false;
     retry++;
   }
   return true;

}

void Pjp274FlashCtrlr::powerOnFlashcontroller()
{
   mRegAccessor->writeRegister(1, 0x0d, 0x02);

}
void Pjp274FlashCtrlr::enterEngineerMode()
{
   mRegAccessor->writeRegister(1, 0x2c, 0xaa);
   mRegAccessor->writeRegister(1, 0x2d, 0xcc);
   usleep(10000);
}

bool Pjp274FlashCtrlr::exitEngineerMode()
{
   int i = 0;
   byte res;
   mRegAccessor->writeRegister(1, 0x2c, 0xaa);
   mRegAccessor->writeRegister(1, 0x2d, 0xbb);
   usleep(500000);
   res=mRegAccessor->readRegister(6, 0x70);
   
   while ((res&0x01)==0)
   {
	usleep(10000);
        i++;
	if(i>50)
        {
           return false;
	}
        res=mRegAccessor->readRegister(6, 0x70);
   }
   
   return true;

}




int Pjp274FlashCtrlr::writeFlash(byte* data, int length, int startPage,
        bool doErase)
{
#ifdef DEBUG
    printf("writeFlash() length=%d, startPage=%d\n", length, startPage);
#endif //DEBUG
    // Check alignment with sector.
    if (startPage % PAGE_COUNT_PERSECTOR != 0)
        return -2;

    int startSector = startPage / PAGE_COUNT_PERSECTOR;
    int sectorCount = ceil((double) length / SECTOR_SIZE);
    if ((startSector + sectorCount) > SECTOR_COUNT)
        sectorCount = SECTOR_COUNT - startSector;
    
    mRegAccessor->writeRegister(1, 0x0d, 0x02);
   
    if (doErase)
    {
        if (!eraseWithSector((byte) startSector, (byte) sectorCount))
        {
            printf("Failed to erase(%d, %d).\n", startSector, sectorCount);
            return -3;
        }
    }
    
    bool res = true;
    for (int i = 0; i < length; i += PAGE_SIZE)
    {
        int remainedSize = length - i;
        int writeSize =
                remainedSize < SRAM_BURST_SIZE ? remainedSize : SRAM_BURST_SIZE;
        int startWritePage = startPage+(i / PAGE_SIZE);
#ifdef DEBUG
        printf("startWritePage: %d, writeSize: %d\n", startWritePage, writeSize);
#endif 
        
        res = writeSram(data + i, writeSize);
        if (!res)
        {
            printf(
                    "Failed to writeSram, index: %d, writeSize: %d, startWritePage: %d\n",
                    i, writeSize, startWritePage);
            break;
        }

        res = writeSramToFlash(startWritePage, sectorCount);
        if (!res)
        {
            printf(
                    "Failed to program, index: %d, writeSize: %d, startWritePage: %d\n",
                    i, writeSize, startWritePage);
            break;
        }
        
    }
    if (!res)
        return -4;
    return 1;
}

int Pjp274FlashCtrlr::readFlash(byte** data, int startPage, int pageLength)
{
    if (pageLength == -1 || pageLength > (PAGE_COUNT - startPage))
        pageLength = PAGE_COUNT - startPage;

    int dataSize = pageLength * PAGE_SIZE;
    byte* pData = new byte[dataSize];
    *data = pData;

    for (int i = 0; i < dataSize; i += SRAM_BURST_SIZE)
    {
        int remainedSize = dataSize - i;
        int readSize =
                remainedSize < SRAM_BURST_SIZE ? remainedSize : SRAM_BURST_SIZE;
        int startReadSector = (startPage + i / PAGE_SIZE)
                / PAGE_COUNT_PERSECTOR;
        int readSectorCount = ceil((double) readSize / SECTOR_SIZE);

        readFlashToSram(startReadSector, readSectorCount);

        int realRead = readSram(pData + i, readSize);
        if (realRead != readSize)
        {
            printf(
                    "readFlash() !! Try to read %d bytes, but only read %d bytes.\n",
                    readSize, realRead);
        }
    }
    return dataSize;
}

bool Pjp274FlashCtrlr::erase(int startPage, int pageLength)
{
    if (startPage % PAGE_COUNT_PERSECTOR != 0)
        return false;

    int startSector = startPage / PAGE_COUNT_PERSECTOR;
    int sectorLength = ceil((double) pageLength / PAGE_COUNT_PERSECTOR);
    if ((startSector + sectorLength) > SECTOR_COUNT)
        sectorLength = SECTOR_COUNT - startSector;

    return eraseWithSector(startSector, sectorLength);
}


void Pjp274FlashCtrlr::setFlashAddress(unsigned int addr)
{
    mRegAccessor->writeRegister(4, 0x48, (addr&0xff));
    mRegAccessor->writeRegister(4, 0x49, (((addr&0xff00)>>8)&0xff));
    mRegAccessor->writeRegister(4, 0x4a, (((addr&0xff0000)>>16)&0xff));
    mRegAccessor->writeRegister(4, 0x4b, (((addr&0xff000000)>>24)&0xff));
}


// TODO

bool Pjp274FlashCtrlr::eraseWithSector(byte sector, byte length)
{
#ifdef DEBUG
    printf("eraseWithSector() sector=%d, length=%d\n", sector, length);
#endif //DEBUG
    byte sectorcnt=0;
    bool res;

    for(sectorcnt=0;sectorcnt<length;sectorcnt++)
    {    
        do
	{
	 res=checkBusy();
	} while(!res) ;	
 	
	writeEnable();
	setFlashAddress((sector+sectorcnt)*4096);
    
    mRegAccessor->writeRegister(4, 0x2c, 0x00);//inst_cmd
    mRegAccessor->writeRegister(4, 0x40, 0x20);//CCR_cmd
    mRegAccessor->writeRegister(4, 0x41, 0x25);//CCR_cmd
    mRegAccessor->writeRegister(4, 0x42, 0x00);//CCR_cmd
    mRegAccessor->writeRegister(4, 0x43, 0x00);//CCR_cmd
    mRegAccessor->writeRegister(4, 0x44, 0x00);//data_cnt
    mRegAccessor->writeRegister(4, 0x45, 0x00);//data_cnt
    mRegAccessor->writeRegister(4, 0x46, 0x00);//data_cnt
    mRegAccessor->writeRegister(4, 0x47, 0x00);//data_cnt
    flashframeStart();
      
    }
    return true;
}

bool Pjp274FlashCtrlr::writeSram(byte* data, int length)
{
#ifdef DEBUG
    printf("writeSram(), length=%d\n", length);
#endif //DEBUG
    bool res = true;
    int remapSize = PAGE_SIZE;
    byte* sramBuf = new byte[remapSize];
    memset(sramBuf, 0xff, remapSize);
    memcpy(sramBuf, data, length);
    mRegAccessor->writeRegister(IO_BANK, 0x09, 0x08);//sram select
    mRegAccessor->writeRegister(IO_BANK, 0x0a, 0x00);//NCS = 0
    mRegAccessor->burstWriteRegister(IO_BANK, REG_SRAM_ACCESS_DATA, sramBuf,
            remapSize);
    mRegAccessor->writeRegister(IO_BANK, 0x0a, 0x01);//NCS = 0
    return res;
}

bool Pjp274FlashCtrlr::writeSramToFlash(byte sector, byte length)
{
#ifdef DEBUG
    printf("writeSramToFlash(), sector=%d, length=%d\n", sector, length);
#endif //DEBUG    
    bool res=true;
   do
   {
     res=checkBusy();
   } while(!res) ;	
   writeEnable();
   setFlashAddress(sector*256);
   mRegAccessor->writeRegister(4, 0x2e, 0x00);//sram access addr
   mRegAccessor->writeRegister(4, 0x2f, 0x00);//sram access addr

   mRegAccessor->writeRegister(4, 0x2c, 0x84);//inst_cmd
   mRegAccessor->writeRegister(4, 0x40, 0x02);//CCR_cmd
   mRegAccessor->writeRegister(4, 0x41, 0x25);//CCR_cmd
   mRegAccessor->writeRegister(4, 0x42, 0x00);//CCR_cmd
   mRegAccessor->writeRegister(4, 0x43, 0x01);//CCR_cmd
   mRegAccessor->writeRegister(4, 0x44, 0x00);//data_cnt
   mRegAccessor->writeRegister(4, 0x45, 0x01);//data_cnt
   mRegAccessor->writeRegister(4, 0x46, 0x00);//data_cnt
   mRegAccessor->writeRegister(4, 0x47, 0x00);//data_cnt
   res=flashframeStart();
    
    return res;
}

void Pjp274FlashCtrlr::readFlashToSram(byte sector, byte length)
{


    
 
    
}

int Pjp274FlashCtrlr::readSram(byte* data, int length)
{
#ifdef DEBUG
    printf("readSram(), length=%d\n", length);
#endif //DEBUG
    // Clear flash SRAM offset address.
    mRegAccessor->writeRegister(4, 0x1c, 0);
    mRegAccessor->writeRegister(4, 0x1d, 0);

    bool res = true;;
    int readLength = 0;
    if (res)
    {
  
        readLength = mRegAccessor->burstReadRegister(data, IO_BANK,
                REG_SRAM_ACCESS_DATA, length);

        if (readLength <= 0)
        {
            printf("readSram() - Failed to read, empty data.\n");
        }
       
    }
    else
    {
        printf("readSram() - Failed to unlockLevelZeroProtection()\n");
    }

    return readLength;
}
void Pjp274FlashCtrlr::readSysRegisterBatch(byte bank,int length,bool AutoRead)
{
	byte* pData = new byte[length];
	mRegAccessor->readRegisters(pData,bank,0,length);
        printf("sysbank=%d",bank);
        for (int i = 0; i < length; i++)
	{
	   printf(",%d",pData[i]);
	}
	
	delete [] pData;
}
void Pjp274FlashCtrlr::readUserRegisterBatch(byte bank,int length,bool AutoRead)
{
	byte* pData = new byte[length];
	mRegAccessor->readUserRegisters(pData,bank,0,length);
        printf("userbank=%d",bank);
        for (int i = 0; i < length; i++)
	{
	   printf(",%d",pData[i]);
	}
	
	delete [] pData;
}
void Pjp274FlashCtrlr::writeRegister(byte bank,byte addr,byte value)
{
   mRegAccessor->writeRegister(bank, addr, value);
}
void Pjp274FlashCtrlr::writeUserRegister(byte bank,byte addr,byte value)
{
   mRegAccessor->writeUserRegister(bank,addr,value);
}
void Pjp274FlashCtrlr::readFrame(void)
{     
    mRegAccessor->writeUserRegister(2,0x7,0x10);
    byte NumDrive = mRegAccessor->readRegister(9, 0x01);
    byte NumSense = mRegAccessor->readRegister(9, 0x02);
    mRegAccessor->writeRegister(IO_BANK, 0x0E, NumDrive);
    mRegAccessor->writeRegister(IO_BANK, 0x0F, NumSense);
    mRegAccessor->writeRegister(IO_BANK, 0x08, 0x00);
    mRegAccessor->writeRegister(IO_BANK, 0x09, 0x05);
    mRegAccessor->writeRegister(IO_BANK, 0x0A, 0x00);
    mRegAccessor->writeRegister(4, 0x1c, 0);
    mRegAccessor->writeRegister(4, 0x1d, 0);
        
    int dataSize = (NumDrive+1)*(NumSense+1)*2;
    byte* pData = new byte[dataSize];
    
    printf("\n NumDrive=%d,NumSense=%d,dataSize=%d\n,",NumDrive,NumSense,dataSize);
    for (int i = 0; i < dataSize; i += SRAM_BURST_SIZE)
    {
        int remainedSize = dataSize - i;
        int readSize =
                remainedSize < SRAM_BURST_SIZE ? remainedSize : SRAM_BURST_SIZE;

        int realRead = readSram(pData + i, readSize);

        if (realRead != readSize)
        {
            printf(
                    "readFlash() !! Try to read %d bytes, but only read %d bytes.\n",
                    readSize, realRead);
        }
    }
    printf("[");
    for (int i = 0; i < dataSize; i+=2)
    {
        short value = pData[i]|(pData[i+1]<<8);
        if(i%((NumDrive+1)*2)==0 && i>0)
            printf("\n"); 
        printf("%2d,", value);
        
    }
    printf("]");	
    mRegAccessor->writeRegister(IO_BANK, 0x0A, 0x01);
    delete [] pData;
}
