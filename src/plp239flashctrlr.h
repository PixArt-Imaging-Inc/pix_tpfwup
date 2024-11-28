#ifndef __PLP239_FLASH_CONTROLLER__
#define __PLP239_FLASH_CONTROLLER__
#include <stdint.h>

#include "type.h"
#include "registeraccessor.h"
#include "flashcontroller.h"

namespace pixart
{
    class Plp239FlashCtrlr: public FlashController
    {
    public:
        class Status
        {
        private:
            uint16_t mStatusFlags;
        public:
            static const uint16_t Finish = 1 << 0;
            static const uint16_t BufferOverflow = 1 << 1;
            static const uint16_t ProgramOverflow = 1 << 2;
            static const uint16_t BistError = 1 << 3;
            static const uint16_t ProtectError = 1 << 4;
            static const uint16_t Level0AccessEnable = 1 << 5;
            static const uint16_t Level1AccessEnable = 1 << 6;
            static const uint16_t CpuClockEn = 1 << 7;

            Status(uint16_t flags);
            ~Status();

            bool hasFlag(uint16_t flag);
        };

        enum class Command
            : byte
            {
                SramReadWrite = 0x11,
            Program = 0x33,
            Erase = 0x44,
            Read = 0x77,
            Finish = 0xdd,
        };

    public:
        static const int SRAM_BURST_SIZE;
        static const int PAGE_SIZE;
        static const int PAGE_COUNT_PERSECTOR;
        static const int SECTOR_COUNT;
        static const int PAGE_COUNT;
        static const int SECTOR_SIZE;
        static const int FIRMWARE_START_PAGE;
        static const int FIRMWARE_SIZE_IN_SECTOR;
        static const int FIRMWARE_SIZE_IN_PAGE;
        static const int PARAMETER_START_PAGE;
        static const int HID_DESCRIPTOR_START_PAGE;

    private:
        /* HW SFC register address */
        static const byte REG_PROTECT_KEY_LV0;
        static const byte REG_PROTECT_KEY_LV1;
        static const byte REG_PROGRAM_BIST;
        static const byte REG_FLASH_SECTOR_ADDR;
        static const byte REG_FLASH_SECTOR_LENGTH;
        static const byte REG_FLASH_COMMAND;
        static const byte REG_SRAM_ACCESS_DATA;
        static const byte REG_FLASH_STATUS;

        static const byte IO_BANK;

    protected:
        //RegisterAccessor* mRegAccessor;

    public:
        Plp239FlashCtrlr(RegisterAccessor* regAccessor);
        ~Plp239FlashCtrlr();

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
        void setCommand(Command command);
        void setFlashAddress(byte sector, byte length);
        bool unlockLevelZeroProtection();
        bool unlockLevelOneProtection();
        Status getStatus();
        bool waitHardwareDone();
        bool finish();
        bool checkBist();
        void enableBist();

        bool eraseWithSector(byte sector, byte length);
        // Write
        bool writeSram(byte* data, int length);
        bool writeSramToFlash(byte sector, byte length);
        // Read
        void readFlashToSram(byte sector, byte length);
        int readSram(byte * data, int length);

        uint32_t calCheckSum(byte* array, int length);

    private:
        bool writeSector(byte* data, int length, int startPage, int index); 
        bool readAndVerifySector(byte* originalData, int length, int startPage, int index);
    };
}
#endif //__PLP239_FLASH_CONTROLLER__
