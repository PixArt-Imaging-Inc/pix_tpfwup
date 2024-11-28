#include "plp239flashctrlr.h"

#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

using namespace pixart;

Plp239FlashCtrlr::Status::Status(uint16_t flags) :
        mStatusFlags(flags)
{
}

Plp239FlashCtrlr::Status::~Status()
{
}

bool Plp239FlashCtrlr::Status::hasFlag(uint16_t flag)
{
    if ((mStatusFlags & flag) > 0)
        return true;
    return false;
}

const int Plp239FlashCtrlr::SRAM_BURST_SIZE = 0x0000A000;
const int Plp239FlashCtrlr::PAGE_SIZE = 256;
const int Plp239FlashCtrlr::PAGE_COUNT_PERSECTOR = 16;
const int Plp239FlashCtrlr::SECTOR_COUNT = 15;
const int Plp239FlashCtrlr::PAGE_COUNT = SECTOR_COUNT * PAGE_COUNT_PERSECTOR;
const int Plp239FlashCtrlr::SECTOR_SIZE = PAGE_COUNT_PERSECTOR * PAGE_SIZE;

const int Plp239FlashCtrlr::FIRMWARE_START_PAGE = 0;
const int Plp239FlashCtrlr::FIRMWARE_SIZE_IN_SECTOR = 11;
const int Plp239FlashCtrlr::FIRMWARE_SIZE_IN_PAGE = FIRMWARE_SIZE_IN_SECTOR
        * PAGE_COUNT_PERSECTOR;
const int Plp239FlashCtrlr::PARAMETER_START_PAGE = 11 * PAGE_COUNT_PERSECTOR;
const int Plp239FlashCtrlr::HID_DESCRIPTOR_START_PAGE = 12
        * PAGE_COUNT_PERSECTOR;

const byte Plp239FlashCtrlr::IO_BANK = 6;

/* HW SFC register address */
const byte Plp239FlashCtrlr::REG_PROTECT_KEY_LV0 = 0x20;
const byte Plp239FlashCtrlr::REG_PROTECT_KEY_LV1 = 0x22;
const byte Plp239FlashCtrlr::REG_PROGRAM_BIST = 0x21;
const byte Plp239FlashCtrlr::REG_FLASH_SECTOR_ADDR = 0x23;
const byte Plp239FlashCtrlr::REG_FLASH_SECTOR_LENGTH = 0x24;
const byte Plp239FlashCtrlr::REG_FLASH_COMMAND = 0x25;
const byte Plp239FlashCtrlr::REG_SRAM_ACCESS_DATA = 0x26;
const byte Plp239FlashCtrlr::REG_FLASH_STATUS = 0x27;

Plp239FlashCtrlr::Plp239FlashCtrlr(RegisterAccessor* regAccessor) :
        FlashController(regAccessor)
{
}

Plp239FlashCtrlr::~Plp239FlashCtrlr()
{
}

int Plp239FlashCtrlr::writeFlash(byte* data, int length, int startPage,
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

    if (doErase)
    {
        if (!eraseWithSector((byte) startSector, (byte) sectorCount))
        {
            printf("Failed to erase(%d, %d).\n", startSector, sectorCount);
            return -3;
        }
    }

    // Workaround: For the PLP239, there is an ExtendFlash block that is not included in the CRC check.
    // If the burning process is incomplete, this block may not be detected by the CRC verification.
    // To address this issue, the ExtendFlash block is programmed first, followed by the original Flash block.
    // This ensures that any incomplete burning will still be detected by the CRC check.
    // Process ExtendFlash
    for (int i = SRAM_BURST_SIZE; i < length; i += SRAM_BURST_SIZE)
    {
        if (!writeSector(data, length, startPage, i) || !readAndVerifySector(data, length, startPage, i))
        {
            printf("Failed to write or verify sector at index %d.\n", i);
            return -4;
        }
    }

    // Process NormalFlash
    if (!writeSector(data, length, startPage, 0))
        return -4;

    return 1;
}

bool Plp239FlashCtrlr::readAndVerifySector(byte* originalData, int length, int startPage, int index)
{
    int remainedSize = length - index;
    int readSize = remainedSize < SRAM_BURST_SIZE ? remainedSize : SRAM_BURST_SIZE;
    int startReadSector = (startPage + index / PAGE_SIZE) / PAGE_COUNT_PERSECTOR;
    int readSectorCount = ceil((double)readSize / SECTOR_SIZE);

    // Allocate buffer for reading data
    byte* readBuffer = new byte[readSize];

    // Read data from flash
    readFlashToSram(startReadSector, readSectorCount);
    int actualReadSize = readSram(readBuffer, readSize);

    // Compare read data with original data
    bool isValid = (actualReadSize == readSize) && (memcmp(originalData + index, readBuffer, readSize) == 0);

#ifdef DEBUG
    if (!isValid)
    {
        printf("Data verification failed at index %d. Expected size: %d, Actual size: %d\n", index, readSize, actualReadSize);
    }
#endif // DEBUG

    delete[] readBuffer; // Free memory
    return isValid;
}

bool Plp239FlashCtrlr::writeSector(byte* data, int length, int startPage, int index)
{
    int remainedSize = length - index;
    int writeSize = remainedSize < SRAM_BURST_SIZE ? remainedSize : SRAM_BURST_SIZE;
    int startWriteSector = (startPage + index / PAGE_SIZE) / PAGE_COUNT_PERSECTOR;
    int writeSectorCount = ceil((double)writeSize / SECTOR_SIZE);

#ifdef DEBUG
    printf("startWriteSector: %d, writeSectorCount: %d\n", startWriteSector, writeSectorCount);
#endif // DEBUG

    if (!writeSram(data + index, writeSize))
    {
        printf("Failed to writeSram, index: %d, writeSize: %d, startSector: %d, sectorCount: %d\n",
               index, writeSize, startWriteSector, writeSectorCount);
        return false;
    }

    if (!writeSramToFlash(startWriteSector, writeSectorCount))
    {
        printf("Failed to program, index: %d, writeSize: %d, startSector: %d, sectorCount: %d\n",
               index, writeSize, startWriteSector, writeSectorCount);
        return false;
    }

    return true;
}

int Plp239FlashCtrlr::readFlash(byte** data, int startPage, int pageLength)
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

bool Plp239FlashCtrlr::erase(int startPage, int pageLength)
{
    if (startPage % PAGE_COUNT_PERSECTOR != 0)
        return false;

    int startSector = startPage / PAGE_COUNT_PERSECTOR;
    int sectorLength = ceil((double) pageLength / PAGE_COUNT_PERSECTOR);
    if ((startSector + sectorLength) > SECTOR_COUNT)
        sectorLength = SECTOR_COUNT - startSector;

    return eraseWithSector(startSector, sectorLength);
}

void Plp239FlashCtrlr::setCommand(Command cmd)
{
#ifdef DEBUG
    printf("setCommand(), cmd=0x%2X\n", cmd);
#endif //DEBUG
    mRegAccessor->writeRegister(IO_BANK, REG_FLASH_COMMAND, (byte) cmd);
}

void Plp239FlashCtrlr::setFlashAddress(byte sector, byte length)
{
    mRegAccessor->writeRegister(IO_BANK, REG_FLASH_SECTOR_ADDR, sector);
    mRegAccessor->writeRegister(IO_BANK, REG_FLASH_SECTOR_LENGTH, length);
}

Plp239FlashCtrlr::Status Plp239FlashCtrlr::getStatus()
{
    static const int statusSize = 2;
    //byte * raw = new byte[statusSize];
    byte raw[statusSize];
    int read = mRegAccessor->readRegisters(raw, IO_BANK, REG_FLASH_STATUS,
            statusSize);
    if (read != statusSize)
    {
        raw[0] = mRegAccessor->readRegister(IO_BANK, REG_FLASH_STATUS);
        raw[1] = mRegAccessor->readRegister(IO_BANK, REG_FLASH_STATUS + 1);
    }

    int status = raw[0] + (raw[1] << 8);
#ifdef DEBUG   
    printf("status: ");
    int n = status;
    int i = 8;
    while (i--)
    {
        if (n & 1)
        printf("1");
        else
        printf("0");

        n >>= 1;
    }
    printf("\n");
#endif //DEBUG    
    return Plp239FlashCtrlr::Status(status);
}

bool Plp239FlashCtrlr::unlockLevelZeroProtection()
{
#ifdef DEBUG
    printf("unlockLevelZeroProtection()\n");
#endif //DEBUG

    byte protectKey = 0xcc;
    mRegAccessor->writeRegister(IO_BANK, REG_PROTECT_KEY_LV0, protectKey);

    if (!getStatus().hasFlag(Status::Level0AccessEnable))
    {
        printf("Cannot enable level 0 flash control.\n");
        return false;
    }
    return true;
}

bool Plp239FlashCtrlr::unlockLevelOneProtection()
{
#ifdef DEBUG
    printf("unlockLevelOneProtection()\n");
#endif //DEBUG
    byte protectKey = 0xee;
    mRegAccessor->writeRegister(IO_BANK, REG_PROTECT_KEY_LV1, protectKey);

    if (!getStatus().hasFlag(Status::Level1AccessEnable))
    {
        printf("Cannot enable level 1 flash control.\n");
        return false;
    }
    return true;
}

bool Plp239FlashCtrlr::waitHardwareDone()
{
#ifdef DEBUG
    printf("waitHardwareDone()\n");
#endif //DEBUG	
    int retry = 8000;
    do
    {
        Status status = getStatus();
        if (status.hasFlag(Status::Finish))
            return true;
        sleep(1);
    } while (retry-- > 0);
    printf("Timeout!\n");
    return false;
}

bool Plp239FlashCtrlr::finish()
{
#ifdef DEBUG    
    printf("finish()\n");
#endif //DEBUG
    setCommand(Command::Finish);
    if (getStatus().hasFlag(Status::Level0AccessEnable)
            || getStatus().hasFlag(Status::Level1AccessEnable))
    {
        printf("Cannot disable flash controller.\n");
        return false;
    }
    return true;
}

bool Plp239FlashCtrlr::checkBist()
{
    if (getStatus().hasFlag(Status::BistError))
        return false;
    return true;
}

// TODO
// combine enableBist() and checkBist()
void Plp239FlashCtrlr::enableBist()
{
    mRegAccessor->writeRegister(IO_BANK, REG_PROGRAM_BIST, 0x01);
}

bool Plp239FlashCtrlr::eraseWithSector(byte sector, byte length)
{
#ifdef DEBUG
    printf("eraseWithSector() sector=%d, length=%d\n", sector, length);
#endif //DEBUG
    bool res;
    res = unlockLevelZeroProtection();
    if (res)
        res = unlockLevelOneProtection();

    if (res)
    {
        setFlashAddress(sector, length);
        setCommand(Command::Erase);

        waitHardwareDone();
    }
    res &= finish();
    return res;
}

bool Plp239FlashCtrlr::writeSram(byte* data, int length)
{
#ifdef DEBUG
    printf("writeSram(), length=%d\n", length);
#endif //DEBUG
    bool res = true;
    int remapSize = SECTOR_SIZE * ceil((double) length / SECTOR_SIZE);
    byte* sramBuf = new byte[remapSize];
    memset(sramBuf, 0xff, remapSize);
    memcpy(sramBuf, data, length);

    unlockLevelZeroProtection();
    setCommand(Command::SramReadWrite);

    mRegAccessor->burstWriteRegister(IO_BANK, REG_SRAM_ACCESS_DATA, sramBuf,
            remapSize);
    if (getStatus().hasFlag(Status::BufferOverflow))
    {
        printf("SRAM access overflow.\n");
        res = false;
    }
    res &= finish();
    return res;
}

bool Plp239FlashCtrlr::writeSramToFlash(byte sector, byte length)
{
#ifdef DEBUG
    printf("writeSramToFlash(), sector=%d, length=%d\n", sector, length);
#endif //DEBUG    
    bool res;
    // Clear flash SRAM offset address.
    mRegAccessor->writeRegister(4, 0x1c, 0);
    mRegAccessor->writeRegister(4, 0x1d, 0);

    res = unlockLevelZeroProtection();
    if (res)
        res = unlockLevelOneProtection();
    if (res)
    {
        enableBist();

        // The length is start from 1.
        setFlashAddress(sector, length - 1);
        setCommand(Command::Program);

        waitHardwareDone();

        res = checkBist();
        if (!res)
        {
            printf("checkBist() return false.\n");
        }
        else if (getStatus().hasFlag(Status::ProgramOverflow))
        {
            res = false;
            printf("Program access overflow.\n");
        }
    }

    res &= finish();
    return res;
}

void Plp239FlashCtrlr::readFlashToSram(byte sector, byte length)
{
    bool res = unlockLevelZeroProtection();
    if (res)
    {
        setFlashAddress(sector, length);
        setCommand(Command::Read);
        waitHardwareDone();
    }
    res &= finish();
}

int Plp239FlashCtrlr::readSram(byte* data, int length)
{
#ifdef DEBUG
    printf("readSram(), length=%d\n", length);
#endif //DEBUG
    // Clear flash SRAM offset address.
    mRegAccessor->writeRegister(4, 0x1c, 0);
    mRegAccessor->writeRegister(4, 0x1d, 0);

    bool res = unlockLevelZeroProtection();
    int readLength = 0;
    if (res)
    {
        setCommand(Command::SramReadWrite);
        readLength = mRegAccessor->burstReadRegister(data, IO_BANK,
                REG_SRAM_ACCESS_DATA, length);

        if (readLength <= 0)
        {
            printf("readSram() - Failed to read, empty data.\n");
        }
        if (getStatus().hasFlag(Status::BufferOverflow))
        {
            printf("readSram() - SRAM access overflow.");
        }
    }
    else
    {
        printf("readSram() - Failed to unlockLevelZeroProtection()\n");
    }

    finish();
    return readLength;
}
