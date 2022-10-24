#include "pjp255flashctrlr.h"

#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

using namespace pixart;


const int pjp255FlashCtrlr::SRAM_BURST_SIZE = 0x00000100;
const int pjp255FlashCtrlr::PAGE_SIZE = 256;
const int pjp255FlashCtrlr::PAGE_COUNT_PERSECTOR = 16;
const int pjp255FlashCtrlr::SECTOR_COUNT = 15;
const int pjp255FlashCtrlr::PAGE_COUNT = SECTOR_COUNT * PAGE_COUNT_PERSECTOR;
const int pjp255FlashCtrlr::SECTOR_SIZE = PAGE_COUNT_PERSECTOR * PAGE_SIZE;

const int pjp255FlashCtrlr::FIRMWARE_START_PAGE = 0;
const int pjp255FlashCtrlr::PARAMETER_START_PAGE = 224;

const byte pjp255FlashCtrlr::IO_BANK = 2;
const byte pjp255FlashCtrlr::FIRMWARE_CRC=0x02;
const byte pjp255FlashCtrlr::PARAMETER_CRC=0x04;
const byte pjp255FlashCtrlr::DEFAULT_FIRMWARE_CRC=0x10;
const byte pjp255FlashCtrlr::DEFAULT_PARAMETER_CRC=0x20;

/* HW SFC register address */

const byte pjp255FlashCtrlr::REG_SRAM_ACCESS_DATA = 0x0b;


pjp255FlashCtrlr::pjp255FlashCtrlr(RegisterAccessor* regAccessor) :
        FlashController(regAccessor)
{
}

pjp255FlashCtrlr::~pjp255FlashCtrlr()
{
}
bool pjp255FlashCtrlr::flashframeStart()
{
  /*  
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
   }*/
  return true;
}
bool pjp255FlashCtrlr::WriteCommand(byte cmd)  // -----------------done
{
   int retry = 0;
   int value=0;
   //write command
   mRegAccessor->writeRegister(4, 0x2c, cmd);//frame start
   while(true)
   {
       usleep(1000);
       value = mRegAccessor->readRegister(4, 0x2c);
       if((value && cmd) ==0)
         break;
       if(retry>10)
         return false;
       retry++;
   }
  return true;
}

bool pjp255FlashCtrlr::FlashExecuteWith(byte cmd, byte flash_cmd, uint16_t cnt)  // -----------------done
{ //clean Bank 4 0x2C
   mRegAccessor->writeRegister(4, 0x2c, 0x00);
   // Write flash command
   mRegAccessor->writeRegister(4, 0x20, flash_cmd);
    //Write data count
   mRegAccessor->writeRegister(4, 0x22, (cnt & 0xFF));// Low byte
   mRegAccessor->writeRegister(4, 0x23, (cnt>>8));    //high byte
   //write command
   return WriteCommand(cmd);
}


bool pjp255FlashCtrlr::writeEnable()    // -----------------done
{
   int retry = 0;
   int value=0;

   //cmd=0x02; flash cmd =0x06, cnt=0x0000 
   FlashExecuteWith(0x02, 0x06, 0x0000); 
   value=0;
   while(true)
   {
    //cmd=0x08; flash cmd =0x05, cnt=0x0001   
     FlashExecuteWith(0x08, 0x05, 0x0001);   
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

bool pjp255FlashCtrlr::checkBusy()  // -----------------done
{
   int retry = 0;
   int value;
   while(true)
   { 
    //cmd=0x08; flash cmd =0x05, cnt=0x0001   
     FlashExecuteWith(0x08, 0x05, 0x0001);        
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



uint32_t pjp255FlashCtrlr::ReadCRCContent(CRCType type)
{
    byte control_data= FIRMWARE_CRC;

    switch(type)
    {
        case CRCType::PAR_CRC:
            control_data=PARAMETER_CRC;
          break;
        case CRCType::DEF_PAR_CRC:
            control_data = DEFAULT_PARAMETER_CRC;
         break;
        case CRCType::DEF_FW_CRC:
            control_data = DEFAULT_FIRMWARE_CRC;
        break;
        default:
          break;
    }

    byte* pData = new byte[1];
    mRegAccessor->writeUserRegister(0,0x82,control_data);
    byte loop=0;

    do
    {
        usleep(1000);
	    mRegAccessor->readUserRegisters(pData,0,0x82,1);
        if((pData[0]& 0x01)==0) break;
        loop++;
    } while (loop<50);
    
    pData=new byte[4];
    mRegAccessor->readUserRegisters(pData,0,0x84,4);
    uint32_t CRC = (pData[3]<<24)+(pData[2]<<16)+(pData[1]<<8)+ pData[0];
    delete [] pData;
    return CRC; 
}

byte pjp255FlashCtrlr::SingleReaduserRegiser(byte addr)
{
    byte* pData = new byte[1];
    mRegAccessor->readUserRegisters(pData,0,addr,1);
    byte ret=pData[0];
    delete [] pData;
    return ret;
}


void pjp255FlashCtrlr::powerOnFlashcontroller() // -----------------done
{
   mRegAccessor->writeRegister(1, 0x0d, 0x02);

}
void pjp255FlashCtrlr::enterEngineerMode()  // -----------------done
{
   mRegAccessor->writeRegister(1, 0x2c, 0xaa);
   mRegAccessor->writeRegister(1, 0x2d, 0xcc);
   usleep(10000);
}

bool pjp255FlashCtrlr::exitEngineerMode() // -----------------done
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

int pjp255FlashCtrlr::writeFlash(byte* data, int length, int startPage,
        bool doErase)    // -----------------done
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

int pjp255FlashCtrlr::readFlash(byte** data, int startPage, int pageLength)
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

bool pjp255FlashCtrlr::erase(int startPage, int pageLength)
{
    if (startPage % PAGE_COUNT_PERSECTOR != 0)
        return false;

    int startSector = startPage / PAGE_COUNT_PERSECTOR;
    int sectorLength = ceil((double) pageLength / PAGE_COUNT_PERSECTOR);
    if ((startSector + sectorLength) > SECTOR_COUNT)
        sectorLength = SECTOR_COUNT - startSector;

    return eraseWithSector(startSector, sectorLength);
}


void pjp255FlashCtrlr::setFlashAddress(unsigned int addr) // -----------------done
{
    mRegAccessor->writeRegister(4, 0x24, (addr&0xff));
    mRegAccessor->writeRegister(4, 0x25, (((addr&0xff00)>>8)&0xff));
    mRegAccessor->writeRegister(4, 0x26, (((addr&0xff0000)>>16)&0xff));
}

// TODO

bool pjp255FlashCtrlr::eraseWithSector(byte sector, byte length)  // -----------------done
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
        //cmd =0x02; flash_cmd=0x20; cnt=3
        FlashExecuteWith(0x02, 0x20, 0x0003); 
        res=checkBusy();      
    }
    return true;
}

bool pjp255FlashCtrlr::writeSram(byte* data, int length) // -----------------done
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

bool pjp255FlashCtrlr::writeSramToFlash(byte sector, byte length)   
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
   setFlashAddress(sector*256);  // sector :it means startWithPage
   mRegAccessor->writeRegister(4, 0x2e, 0x00);//sram access addr
   mRegAccessor->writeRegister(4, 0x2f, 0x00);//sram access addr
   //cmd=0x81; flash_cmd=0x02; cnt =256 
   FlashExecuteWith(0x81, 0x02, 0x0100);  
   res=checkBusy();  
   return res;
}

void pjp255FlashCtrlr::readFlashToSram(byte sector, byte length)
{


}

int pjp255FlashCtrlr::readSram(byte* data, int length)
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
void pjp255FlashCtrlr::readSysRegisterBatch(byte bank,int length,bool AutoRead)
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
void pjp255FlashCtrlr::readUserRegisterBatch(byte bank,int length,bool AutoRead)
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
void pjp255FlashCtrlr::writeRegister(byte bank,byte addr,byte value)
{
   mRegAccessor->writeRegister(bank, addr, value);
}
void pjp255FlashCtrlr::writeUserRegister(byte bank,byte addr,byte value)
{
   mRegAccessor->writeUserRegister(bank,addr,value);
}
void pjp255FlashCtrlr::readFrame(void)
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
