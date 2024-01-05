#include "pjp255fwupdater.h"
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
const byte pjp255FwUpdater::CPU_SYS_BANK = 0x01;

/* ============= Flash & Test control ============== */
const byte pjp255FwUpdater::FLASH_CTRL_BANK = 0x04;

/* ==================== IO Bank ==================== */
const byte pjp255FwUpdater::IO_BANK = 0x06;
// Control watch dog.


/* ================= User Parameter ================= */
const int pjp255FwUpdater::USER_PARAM_BANK_SIZE = 224;
const int pjp255FwUpdater::USER_PARAM_SIZE = 1024;

pjp255FwUpdater::pjp255FwUpdater(DevHelper* devHelper,
        RegisterAccessor* regaccr) :
        mDevHelper(devHelper), mRegAccr(regaccr), mTargetFirmware(0)
{
    mRegAccr->prepare();
    mFlashCtrlr = make_shared<pjp255FlashCtrlr>(mRegAccr);
}

bool pjp255FwUpdater::reset(ResetType type)
{
    switch (type)
    {
        case ResetType::Regular:
            mFlashCtrlr->exitEngineerMode();
            break;
        case ResetType::HwTestMode:
            mFlashCtrlr->enterEngineerMode();
            break;
        default:
            return false;
    }

    return true;
}

bool _loadBin2Vec(ifstream &ifs, vector<byte> &vec)
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

bool pjp255FwUpdater::loadFwBin(const char * path)
{
    bool ret;
    printf("Binary path: %s\n", path);
    ifstream ifs(path, ifstream::in | ios::ate);
    ret=_loadBin2Vec(ifs, mTargetFirmware);
    ifs.close();
    return ret;
}

bool pjp255FwUpdater::loadParameterBin(const char * path)
{
    bool ret;
    printf("Binary path: %s\n", path);
    ifstream ifs(path, ifstream::in | ios::ate);
    ret=_loadBin2Vec(ifs, mTargetParameter);
    ifs.close();
    return ret;
}

void pjp255FwUpdater::releaseFwBin()
{
    mTargetFirmware.clear();
}

void pjp255FwUpdater::releaseParameterBin()
{
    mTargetParameter.clear();
}

bool pjp255FwUpdater::loadUpgradeBin(char const* path)
{
    static const int UPGRADE_FILE_SIZE = (4096*14) + 4096 ; //need confirm 
    printf("Upgrade file path: %s\n", path);
    ifstream ifs(path, ifstream::in | ios::ate);
    ifs.unsetf(std::ios::skipws);
    int size = ifs.tellg();
   
   if (size > UPGRADE_FILE_SIZE)
    {
        printf("File size too large. (%d > %d)\n", size, UPGRADE_FILE_SIZE);
        return false;
    }
    if (size < UPGRADE_FILE_SIZE)
    {
        printf("File size too small. (%d > %d)\n", size, UPGRADE_FILE_SIZE);
        return false;
    }

    ifs.seekg(0, ios::beg);
    // Read code.
    mTargetFirmware.clear();
    mTargetFirmware.reserve(4096*14);
    copy_n(istream_iterator<byte>(ifs), 4096*14,
            std::back_inserter(mTargetFirmware));
    // Read parameters.
    mTargetParameter.clear();
    mTargetParameter.reserve(4096);
    std::copy_n(istream_iterator<byte>(ifs), 4096,
            std::back_inserter(mTargetParameter));
    ifs.close();
    return true;
}

void pjp255FwUpdater::releaseUpgradeBin()
{
    this->releaseFwBin();
    this->releaseParameterBin();
}

uint32_t pjp255FwUpdater::calCheckSum(byte const * const array, int length)
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

void pjp255FwUpdater::get_calchecksum(bool parameterpart)
{
    uint32_t result=0;
    if (parameterpart == false)
    {
        result=calCheckSum(mTargetFirmware.data(), mTargetFirmware.size());    
    }
    else 
    {
        result=calCheckSum(mTargetParameter.data(), mTargetParameter.size());
    }
    
    printf("The CRC content of input bin file is : 0x%8x\n", result);
}

int pjp255FwUpdater::getICType()
{
    int IcType;
    IcType = mRegAccr->readRegister(0, 0x79);
    IcType = (IcType << 8) | mRegAccr->readRegister(0, 0x78);
    return IcType;
}

int pjp255FwUpdater::getPid()
{
    int pid = 0;
    pid = mDevHelper->getPid();
    return pid;
}

int pjp255FwUpdater::getFwVersion()
{
    int fwVer;
    fwVer = mRegAccr->readuserRegister(0, 0x7f);
    fwVer = (fwVer << 8) | mRegAccr->readuserRegister(0, 0x7e);
    return fwVer;
}

int pjp255FwUpdater::getReadSysRegister(byte bank,byte addr)
{
    int value;
    value = mRegAccr->readRegister(bank, addr);
    return value;
}

int pjp255FwUpdater::getReadUserRegister(byte bank,byte addr)
{
    int value;
    value = mRegAccr->readuserRegister(bank, addr);
    return value;
}

bool pjp255FwUpdater::fullyUpgrade()
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

    bool res = mFlashCtrlr->erase(pjp255FlashCtrlr::FIRMWARE_START_PAGE, 1);
    if (!res)
    {
        printf("Erase firmware failed.\n");
        mFlashCtrlr->exitEngineerMode();
        return false;
    }

    res = false;
    res = mFlashCtrlr->writeFlash(mTargetParameter.data(),
            mTargetParameter.size(), pjp255FlashCtrlr::PARAMETER_START_PAGE, true);

    if (!res)
    {
        printf("Update parameter failed.\n");
        mFlashCtrlr->exitEngineerMode();
        return false;
    }
    
    res = false;
    res = mFlashCtrlr->writeFlash(mTargetFirmware.data(),
        mTargetFirmware.size(), pjp255FlashCtrlr::FIRMWARE_START_PAGE, true);

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

void pjp255FwUpdater::writeFirmware(bool erase)
{
    if (mTargetFirmware.size() <= 0)
        return;
    int res;
    mFlashCtrlr->enterEngineerMode();
    res = mFlashCtrlr->writeFlash(mTargetFirmware.data(),
            mTargetFirmware.size(), pjp255FlashCtrlr::FIRMWARE_START_PAGE,
            erase);
    mFlashCtrlr->exitEngineerMode();
    printf("writeFirmware() result: %d\n", res);
}

void pjp255FwUpdater::GetChipCRC(pjp255FlashCtrlr::CRCType typ)
{
   string whichType = "DEFAULT FW"; 
   uint32_t CRC_result= mFlashCtrlr->ReadCRCContent(typ);

   switch(typ)
   {
     case pjp255FlashCtrlr::CRCType::FW_CRC:
        whichType="FW";
        break;
    case pjp255FlashCtrlr::CRCType::PAR_CRC:
        whichType="PARAM";
        break;
   
     case pjp255FlashCtrlr::CRCType::DEF_PAR_CRC:
        whichType="DEFAULT PARAM";
        break;
     default:
        break;

   }
    printf("The %s CRC content is : 0x%8x\n",whichType.c_str(), CRC_result);
   
}

byte pjp255FwUpdater::getHidFwVersion()
{
    return mFlashCtrlr->SingleReaduserRegiser(0x7F);
}

byte pjp255FwUpdater::getHidParversion()
{
    return mFlashCtrlr->SingleReaduserRegiser(0x7E);
}

void pjp255FwUpdater::writeParameter()
{
    if (mTargetParameter.size() == 0)
        return;
    int res;
     mFlashCtrlr->enterEngineerMode();
    res = mFlashCtrlr->writeFlash(mTargetParameter.data(),
            mTargetParameter.size(), pjp255FlashCtrlr::PARAMETER_START_PAGE,
            true);
    mFlashCtrlr->exitEngineerMode();
    printf("writeParameter() result: %d\n", res);
}

void pjp255FwUpdater::ReadFrameData()
{
    while(1)
    {	
        mFlashCtrlr->readFrame();
        usleep(10);
    }
}

void pjp255FwUpdater::ReadBatchUserRegister(byte bank, int length,bool AutoRead)
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

void pjp255FwUpdater::ReadBatchSysRegister(byte bank, int length,bool AutoRead)
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

void pjp255FwUpdater::writeRegister(byte bank,byte addr,byte value)
{
	mFlashCtrlr->writeRegister(bank,addr,value);
}
void pjp255FwUpdater::writeUserRegister(byte bank,byte addr,byte value)
{
	mFlashCtrlr->writeUserRegister(bank,addr,value);
}
