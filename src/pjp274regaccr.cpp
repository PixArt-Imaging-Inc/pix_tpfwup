#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "pjp274regaccr.h"

using namespace pixart;
static const int RW_REG_REPORT_ID = 0x42;
static const int BURST_RW_REG_REPORT_ID = 0x41;
static const int RW_USER_REG_REPORT_ID = 0x43;

Pjp274RegAccr::Pjp274RegAccr(HidDevice* dev) :
        mHidDevice(dev), mMaxBurstLength(0)
{
}

Pjp274RegAccr::~Pjp274RegAccr()
{
}

void Pjp274RegAccr::prepare()
{
    /**
     * Read HID report descriptor and try to find the capability of
     * register burst r/w, and make sure the register r/w feature ID
     * is exist.
     *
     **/
    // TODO
    // Because we are only one type firmware now, we skip the step
    // of parsing report descriptor, assign the burst length directly.
    mMaxBurstLength = 256;
}

byte Pjp274RegAccr::readuserRegister(byte bank, byte address)
{
    byte buffer[4];
    int len = 0;
    writeUserRegister((byte) (bank | 0x10), address, 0);

    bool res = mHidDevice->getFeature(RW_USER_REG_REPORT_ID, buffer, &len, 4);
    if (res && len == 4)
        return buffer[3];
    else
        return -1;
}
byte Pjp274RegAccr::readRegister(byte bank, byte address)
{
    byte buffer[4];
    int len = 0;
    writeRegister((byte) (bank | 0x10), address, 0);

    bool res = mHidDevice->getFeature(RW_REG_REPORT_ID, buffer, &len, 4);
    if (res && len == 4)
        return buffer[3];
    else
        return -1;
}



int Pjp274RegAccr::readRegisters(byte* data, byte bank, byte startAddress,
        uint64_t length)
{
    for (uint64_t i = 0; i < length; ++i)
    {
        int regVal = readRegister(bank, startAddress + i);
        if (regVal < 0)
            return -1;
        data[i] = (byte) regVal;
    }
    return length;
}

int Pjp274RegAccr::readUserRegisters(byte* data, byte bank, byte startAddress,
        uint64_t length)
{
    for (uint64_t i = 0; i < length; ++i)
    {
        int regVal = readuserRegister(bank, startAddress + i);
        if (regVal < 0)
            return -1;
        data[i] = (byte) regVal;
    }
    return length;
}


int Pjp274RegAccr::burstReadRegister(byte* data, byte bank, byte address,
        uint64_t length)
{
    // 1 byte for report ID.
    uint64_t reportSize = mMaxBurstLength + 1;
    if (mMaxBurstLength > 1)
    {
        // Assign bank and address.
        writeRegister((byte) (bank | 0x10), address, 0);
        uint64_t read = 0;
        byte buffer[reportSize];
        int len;
        do
        {
            bool res = mHidDevice->getFeature(BURST_RW_REG_REPORT_ID, buffer,
                    &len, reportSize);
            if (!res)
            {
                printf("burstReadRegister() getFeature failed.\n");
                return read;
            }
            else
            {
                if (len <= 1)
                {
                    printf(
                            "burstReadRegister() illegal response from getFeature.\n");
                    return read;
                }
                int remine = length - read;
                int dataLen = --len > remine ? remine : len;
                // First byte is report ID.
                memcpy(data + read, buffer + 1, dataLen);
                read += dataLen;
            }
            len = 0;
        } while (read < length);
    }
    else
    {
        for (uint64_t i = 0; i < length; ++i)
        {
            data[i] = readRegister(bank, address);
#ifdef DEBUG
            printf("%2X", data[i]);
            if ((i+1)%64==0) printf("  %" PRIu64 " \n", (i+1)/64);
#endif //DEBUG
        }
    }
#ifdef DEBUG
    printf("\n");
#endif //DEBUG
    return length;
}

void Pjp274RegAccr::writeRegister(byte bank, byte address, byte value)
{
    byte buffer[] =
    { address, bank, value };
    mHidDevice->setFeature(RW_REG_REPORT_ID, buffer, 3);
}
void Pjp274RegAccr::writeUserRegister(byte bank, byte address, byte value)
{
    byte buffer[] =
    { address, bank, value };
    mHidDevice->setFeature(RW_USER_REG_REPORT_ID, buffer, 3);
}
void Pjp274RegAccr::writeRegisters(byte bank, byte startAddress, byte* values,
        uint64_t length)
{
    for (uint64_t i = 0; i < length; ++i)
    {
        writeRegister(bank, startAddress + i, *(values + i));
    }
}
byte Pjp274RegAccr::readInputReport(void)
{
    bool res = mHidDevice->Read();
    return res;
}
void Pjp274RegAccr::burstWriteRegister(byte bank, byte address, byte* values,
        uint64_t length)
{
#ifdef DEBUG
    printf("burstWriteRegister(), bank=%d, address=0x%2X, length=%" PRIu64 "\n", bank, address, length);
#endif //DEBUG
    if (mMaxBurstLength > 1)
    {

        uint64_t remain = length;
        byte buffer[mMaxBurstLength];
        //writeRegister((byte)(bank | 0x10), address, 0);
        for (uint64_t i = 0; i < length; i += mMaxBurstLength)
        {
            // Assign bank and address.
            writeRegister((byte) (bank | 0x10), address, 0);
            uint64_t wLength =
                    remain < mMaxBurstLength ? remain : mMaxBurstLength;
            remain -= wLength;
            memcpy(buffer, values + i, wLength);
#ifdef DEBUG
            printf("write idx: %" PRIu64 ", length: %" PRIu64 ", remain: %" PRIu64 "\n",
                    i, wLength, remain);
#endif //DEBUG
            mHidDevice->setFeature(BURST_RW_REG_REPORT_ID, buffer, wLength);
        }
    }
    else
    {
        for (uint64_t i = 0; i < length; ++i)
        {
            byte val = values[i];
            writeRegister(bank, address, val);
#ifdef DEBUG
            printf("%2X", val);
            if ((i+1)%64==0) printf("  %" PRIu64 " \n", (i+1)/64);
#endif //DEBUG
        }
    }
#ifdef DEBUG
    printf("\n");
#endif //DEBUG
    return;
}
