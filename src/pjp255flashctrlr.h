#ifndef __pjp255_FLASH_CONTROLLER__
#define __pjp255_FLASH_CONTROLLER__
#include <stdint.h>

#include "type.h"
#include "registeraccessor.h"
#include "flashcontroller.h"

namespace pixart
{
    class pjp255FlashCtrlr: public FlashController
    {
    public:
  

    public:
        static const int SRAM_BURST_SIZE;
        static const int PAGE_SIZE;
        static const int PAGE_COUNT_PERSECTOR;
        static const int SECTOR_COUNT;
        static const int PAGE_COUNT;
        static const int SECTOR_SIZE;
        static const int FIRMWARE_START_PAGE;
        static const int PARAMETER_START_PAGE;

    private:
        /* HW SFC register address */
        static const byte REG_SRAM_ACCESS_DATA;
        static const byte FIRMWARE_CRC;
        static const byte PARAMETER_CRC;
        static const byte DEFAULT_FIRMWARE_CRC;
        static const byte DEFAULT_PARAMETER_CRC;
        static const byte IO_BANK;

    protected:
        //RegisterAccessor* mRegAccessor;

    public:
        pjp255FlashCtrlr(RegisterAccessor* regAccessor);
        ~pjp255FlashCtrlr();

        enum class CRCType
        {
            FW_CRC, PAR_CRC, DEF_FW_CRC, DEF_PAR_CRC,
        };

        virtual int writeFlash(byte* data, int length, int startPage,
                bool doErase);
        virtual int readFlash(byte** data, int startPage, int pageLength = -1);
        virtual bool erase(int startPage, int pageLength);
        virtual int getPageAmount()
        {
            return PAGE_COUNT;
        };
        virtual int getPageSize()
        {
            return PAGE_SIZE;
        };

    public:
   
        void setFlashAddress(unsigned int addr);
        bool writeEnable();
	bool checkBusy();
        void powerOnFlashcontroller();
        void enterEngineerMode();
	bool exitEngineerMode();
        bool eraseWithSector(byte sector, byte length);
        // Write
        bool writeSram(byte* data, int length);
        bool writeSramToFlash(byte sector, byte length);
        // Read
        void readFlashToSram(byte sector, byte length);
        int readSram(byte * data, int length);
        bool flashframeStart();
        uint32_t calCheckSum(byte* array, int length);
    bool WriteCommand(byte cmd); //add by shawn
    bool FlashExecuteWith(byte cmd, byte flash_cmd, uint16_t cnt); //add byte shawn
	void readFrame(void);
	void readSysRegisterBatch(byte bank,int length,bool AutoRead);
	void readUserRegisterBatch(byte bank,int length,bool AutoRead);
	void writeRegister(byte bank,byte addr,byte value);
	void writeUserRegister(byte bank,byte addr,byte value);
    uint32_t ReadCRCContent(CRCType type);
    byte SingleReaduserRegiser(byte addr);

    };
}
#endif //__PLP239_FLASH_CONTROLLER__
