#include "pjp274fwupdater.h"
#include <unistd.h>
#include <inttypes.h>

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <iterator>
#include <string>

#include<regex>

using namespace pixart;
using namespace std;

/* ============== CPU system control ============== */
const byte Pjp274FwUpdater::CPU_SYS_BANK = 0x01;

/* ============= Flash & Test control ============== */
const byte Pjp274FwUpdater::FLASH_CTRL_BANK = 0x04;

/* ==================== IO Bank ==================== */
const byte Pjp274FwUpdater::IO_BANK = 0x06;
// Control watch dog.


/* ================= User Parameter ================= */
const int Pjp274FwUpdater::USER_PARAM_BANK_SIZE = 224;
const int Pjp274FwUpdater::USER_PARAM_SIZE = 1024;

Pjp274FwUpdater::Pjp274FwUpdater(DevHelper* devHelper,
        RegisterAccessor* regaccr) :
        mDevHelper(devHelper), mRegAccr(regaccr), mTargetFirmware(0)
{
    mRegAccr->prepare();
    mFlashCtrlr = make_shared<Pjp274FlashCtrlr>(mRegAccr);
}



bool Pjp274FwUpdater::reset(ResetType type)
{
    switch (type)
    {
    case ResetType::Regular:
    {
       mFlashCtrlr->exitEngineerMode();
    }
        break;
    case ResetType::HwTestMode:
    {
       mFlashCtrlr->enterEngineerMode();
    }
        break;
    default:
        return false;
    }

    return true;
}

bool loadBin2Vec_(ifstream &ifs, vector<byte> &vec)
{
    bool ret;
    ifs.unsetf(std::ios::skipws);
    int size = ifs.tellg();
    ifs.seekg(0, ios::beg);

    vec.clear();
    vec.reserve(size);
    vec.insert(vec.begin(), istream_iterator<byte>(ifs),
            istream_iterator<byte>());

#ifdef DEBUG
    printf("Binary content:\n");
    int i = 0;
    for (auto& n : vec)
    {
        printf("%02X ", n);
        if (i++%30==29) printf("\n");
    }
    printf("\n");
#endif //DEBUG

    if (size >= 0 && (unsigned int) size == vec.size())
    {
#ifdef DEBUG
        printf("Binary size: %lu\n", vec.size());
#endif //DEBUG
        ret = true;
    }
    else
    {
#ifdef DEBUG
        printf("loadBin2Vec() read binary abnormal, read %lu bytes.\n", vec.size());
#endif //DEBUG
        vec.clear();
        ret = false;
    }
    return ret;
}

bool Pjp274FwUpdater::loadFwBin(const char * path)
{
    bool ret = 1;
    printf("Binary path: %s\n", path);
    ifstream ifs(path, ifstream::in | ios::ate);
    loadBin2Vec_(ifs, mTargetFirmware);
    ifs.close();
    return ret;
}

bool Pjp274FwUpdater::loadParameterBin(const char * path)
{
    bool ret = 1;
    printf("Binary path: %s\n", path);
    ifstream ifs(path, ifstream::in | ios::ate);
    loadBin2Vec_(ifs, mTargetParameter);
    ifs.close();
    return ret;
}

void Pjp274FwUpdater::releaseFwBin()
{
    mTargetFirmware.clear();
}

void Pjp274FwUpdater::releaseParameterBin()
{
    mTargetParameter.clear();
}

bool Pjp274FwUpdater::loadUpgradeBin(char const* path)
{
    static const int UPGRADE_FILE_SIZE = 57344 + 4096; // PJP274 Flash block 60kB (firmware 56kB + parameter 4kB)
    printf("Upgrade file path: %s\n", path);
    ifstream ifs(path, ifstream::in | ios::ate);
    ifs.unsetf(std::ios::skipws);
    int size = ifs.tellg();
   
    if (size > UPGRADE_FILE_SIZE)
    {
        printf("File size too large. (%d > %d)\n", size, UPGRADE_FILE_SIZE);
        return false;
    }

    ifs.seekg(0, ios::beg);
    // Read code.
    mTargetFirmware.clear();
    mTargetFirmware.reserve(57344);
    copy_n(istream_iterator<byte>(ifs), 57344,
            std::back_inserter(mTargetFirmware));
    // Read parameters.
    mTargetParameter.clear();
    mTargetParameter.reserve(4096);
    std::copy_n(istream_iterator<byte>(ifs), 4096,
            std::back_inserter(mTargetParameter));
    ifs.close();
    return true;
}

void Pjp274FwUpdater::releaseUpgradeBin()
{
    this->releaseFwBin();
    this->releaseParameterBin();
}

uint32_t Pjp274FwUpdater::calCheckSum(byte const * const array, int length)
{
    uint32_t crc;
    uint32_t sum1 = 0xFFFF, sum2 = 0xFFFF;
    for (int i = 0; i < length; i += 2)
    {
        uint16_t val = array[i] + (array[i + 1] << 8);
        sum1 += val;
        sum2 += sum1;
        sum1 = (sum1 & 0x0000ffff) + (sum1 >> 16);
        sum2 = (sum2 & 0x0000ffff) + (sum2 >> 16);
    }
    sum1 = (sum1 & 0x0000ffff) + (sum1 >> 16);
    sum2 = (sum2 & 0x0000ffff) + (sum2 >> 16);
    crc = sum2 << 16 | sum1;
    return crc;
}

int Pjp274FwUpdater::getICType()
{
    int IcType;
    IcType = mRegAccr->readRegister(0, 0x79);
    IcType = (IcType << 8) | mRegAccr->readRegister(0, 0x78);
    return IcType;
}

int Pjp274FwUpdater::getPid()
{
    int pid = 0;
    pid = mDevHelper->getPid();
    return pid;
}

int Pjp274FwUpdater::getFwVersion()
{
    int fwVer;
    fwVer = mRegAccr->readuserRegister(0, 0xb3);
    fwVer = (fwVer << 8) | mRegAccr->readuserRegister(0, 0xb2);
    return fwVer;
}

int Pjp274FwUpdater::getReadSysRegister(byte bank,byte addr)
{
    int value;
    value = mRegAccr->readRegister(bank, addr);
    return value;
}

int Pjp274FwUpdater::getReadUserRegister(byte bank,byte addr)
{
    int value;
    value = mRegAccr->readuserRegister(bank, addr);
    return value;
}

bool Pjp274FwUpdater::fullyUpgrade()
{
    if (mTargetFirmware.size() <= 0)
    {
        printf("Load firmware size error.");
        return false;
    }
    if (mTargetParameter.size() <= 0)
    {
        printf("Load parameter size error.");
        return false;
    }

    mFlashCtrlr->enterEngineerMode();

    bool res = mFlashCtrlr->erase(Pjp274FlashCtrlr::FIRMWARE_START_PAGE, 1);
    if (!res)
    {
        printf("Erase firmware failed.");
        mFlashCtrlr->exitEngineerMode();
        return false;
    }

    res = false;
    res = mFlashCtrlr->writeFlash(mTargetParameter.data(),
            mTargetParameter.size(), Pjp274FlashCtrlr::PARAMETER_START_PAGE, true);
    
    if (!res)
    {
        printf("Update parameter failed.\n");
        mFlashCtrlr->exitEngineerMode();
        return false;
    }

    res = false;
    res = mFlashCtrlr->writeFlash(mTargetFirmware.data(),
            mTargetFirmware.size(), Pjp274FlashCtrlr::FIRMWARE_START_PAGE, true);

    if (!res)
    {
        printf("Update firmware failed.\n");
        mFlashCtrlr->exitEngineerMode();
        return false;
    }

    mFlashCtrlr->exitEngineerMode();
    printf("Update completed.\n");
    return true;
}

void Pjp274FwUpdater::writeFirmware(bool erase)
{
    if (mTargetFirmware.size() <= 0)
        return;
    int res;
    mFlashCtrlr->enterEngineerMode(); 
    res = mFlashCtrlr->writeFlash(mTargetFirmware.data(),
            mTargetFirmware.size(), Pjp274FlashCtrlr::FIRMWARE_START_PAGE,
            erase);
    mFlashCtrlr->exitEngineerMode();
    printf("writeFirmware() result: %d\n", res);
}


void Pjp274FwUpdater::writeParameter()
{
    if (mTargetParameter.size() == 0)
        return;
    int res;
     mFlashCtrlr->enterEngineerMode();
    res = mFlashCtrlr->writeFlash(mTargetParameter.data(),
            mTargetParameter.size(), Pjp274FlashCtrlr::PARAMETER_START_PAGE,
            true);
    mFlashCtrlr->exitEngineerMode();
    printf("writeParameter() result: %d\n", res);
}

void Pjp274FwUpdater::ReadFrameData()
{
    while(1)
    {	
        mFlashCtrlr->readFrame();
        usleep(10);
    }
}

void Pjp274FwUpdater::ReadBatchUserRegister(byte bank, int length,bool AutoRead)
{
    mFlashCtrlr->readUserRegisterBatch(bank,length,AutoRead);
    if(AutoRead==false)
      return;
    while(1)
    {	
       mFlashCtrlr->readUserRegisterBatch(bank,length,AutoRead);
       usleep(100);
    }
}

void Pjp274FwUpdater::ReadBatchSysRegister(byte bank, int length,bool AutoRead)
{
    
     mFlashCtrlr->readSysRegisterBatch(bank,length,AutoRead);
     if(AutoRead==false)
      return;
    while(1)
    {	
       mFlashCtrlr->readSysRegisterBatch(bank,length,AutoRead);
       usleep(100);
    } 
}

void Pjp274FwUpdater::writeRegister(byte bank,byte addr,byte value)
{
	mFlashCtrlr->writeRegister(bank,addr,value);
}

void Pjp274FwUpdater::writeUserRegister(byte bank,byte addr,byte value)
{
	mFlashCtrlr->writeUserRegister(bank,addr,value);
}
