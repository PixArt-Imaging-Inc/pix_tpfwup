#include "plp239fwupdater.h"
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

/* ============ CPU and power control ============ */
const byte Plp239FwUpdater::CLK_POW_BANK = 0x00;
// CPU control
const byte Plp239FwUpdater::R_CLKS_PU_1 = 0x00;
const byte Plp239FwUpdater::R_CLKS_PU_3 = 0x02;
const byte Plp239FwUpdater::R_CLKS_PD_1 = 0x04;
const byte Plp239FwUpdater::R_FW_VER_L = 0x16;
const byte Plp239FwUpdater::R_FW_VER_H = 0x18;
const byte Plp239FwUpdater::V_CLKS_1_CPU = 0x02;
const byte Plp239FwUpdater::V_CLKS_3_L_LV_FLASH_CTRL = 0x40;
const byte Plp239FwUpdater::V_CLKS_3_H_LV_FLASH_CTRL = 0x80;
// Bank 0 writable control
const byte Plp239FwUpdater::R_BK0_PROTECT_KEY = 0x7c;
const byte Plp239FwUpdater::V_BK0_WRITE_ENABLE = 0x7c;
/* ============== CPU system control ============== */
const byte Plp239FwUpdater::CPU_SYS_BANK = 0x01;
// Trigger software (firmware) system reset
const byte Plp239FwUpdater::R_SW_RESET_KEY_1 = 0x20;
const byte Plp239FwUpdater::R_SW_RESET_KEY_2 = 0x21;
const byte Plp239FwUpdater::V_SW_RESET_KEY_1 = 0xaa;
const byte Plp239FwUpdater::V_SW_RESET_KEY_2 = 0xbb;
/* ============= Flash & Test control ============== */
const byte Plp239FwUpdater::FLASH_CTRL_BANK = 0x04;
// Low level clock control
const byte Plp239FwUpdater::R_LLEVEL_PROTECT_KEY = 0x18;
const byte Plp239FwUpdater::V_LOW_LV_CLOCK_DISABLE = 0x00;
const byte Plp239FwUpdater::V_LOW_LV_CLOCK_ENABLE = 0xcc;
/* ==================== IO Bank ==================== */
const byte Plp239FwUpdater::IO_BANK = 0x06;
// Control watch dog.
const byte Plp239FwUpdater::R_IOA_WD_DISABLE = 0x7d;
const byte Plp239FwUpdater::V_WD_DISABLE_KEY = 0xad;
const byte Plp239FwUpdater::V_WD_ENABLE_KEY = 0x01;
// Boot state flags
const byte Plp239FwUpdater::R_BOOT_STATE = 0x10;
/* ================= IO Control Bank ================ */
const byte Plp239FwUpdater::IO_CTRL_BANK = 0x08;
// Firmware ready flag
const byte Plp239FwUpdater::R_HID_FW_RDY = 0x68;
;
/* ================= User Parameter ================= */
const int Plp239FwUpdater::USER_PARAM_BANK_SIZE = 128;
const int Plp239FwUpdater::USER_PARAM_SIZE = 1024;

Plp239FwUpdater::Plp239FwUpdater(DevHelper* devHelper,
        RegisterAccessor* regaccr) :
        mDevHelper(devHelper), mRegAccr(regaccr), mTargetFirmware(0)
{
    mRegAccr->prepare();
    mFlashCtrlr = make_shared<Plp239FlashCtrlr>(mRegAccr);
}

void Plp239FwUpdater::setBank0Writeable()
{
#ifdef DEBUG
    printf("setBank0Writeable()\n");
#endif // DEBUG	
    mRegAccr->writeRegister(CLK_POW_BANK, R_BK0_PROTECT_KEY,
            V_BK0_WRITE_ENABLE);
}

void Plp239FwUpdater::resetSofeware()
{
#ifdef DEBUG
    printf("resetSofeware()\n");
#endif // DEBUG	
    mRegAccr->writeRegister(CPU_SYS_BANK, R_SW_RESET_KEY_1, V_SW_RESET_KEY_1);
    mRegAccr->writeRegister(CPU_SYS_BANK, R_SW_RESET_KEY_2, V_SW_RESET_KEY_2);
}

void Plp239FwUpdater::clearHidFwReadyFlag()
{
#ifdef DEBUG
    printf("clearHidFwReadyFlag()\n");
#endif // DEBUG		
    mRegAccr->writeRegister(IO_CTRL_BANK, R_HID_FW_RDY, 0);
}

void Plp239FwUpdater::ctrlWatchDog(bool enable)
{
#ifdef DEBUG
    printf("ctrlWatchDog()\n");
#endif // DEBUG
    if (enable)
    {
        mRegAccr->writeRegister(IO_BANK, R_IOA_WD_DISABLE, V_WD_ENABLE_KEY);
    }
    else
    {
        mRegAccr->writeRegister(IO_BANK, R_IOA_WD_DISABLE, V_WD_DISABLE_KEY);
    }
}

void Plp239FwUpdater::ctrlCPU(bool enable)
{
#ifdef DEBUG
    printf("ctrlCPU()\n");
#endif // DEBUG
    if (enable)
    {
        mRegAccr->writeRegister(CLK_POW_BANK, R_CLKS_PU_1, V_CLKS_1_CPU);
        usleep(20000);
        mRegAccr->writeRegister(CLK_POW_BANK, R_CLKS_PD_1, 0x00);
    }
    else
    {
        mRegAccr->writeRegister(CLK_POW_BANK, R_CLKS_PD_1, V_CLKS_1_CPU);
        mRegAccr->writeRegister(CLK_POW_BANK, R_CLKS_PU_3,
                V_CLKS_3_L_LV_FLASH_CTRL | V_CLKS_3_H_LV_FLASH_CTRL);
    }
}

void Plp239FwUpdater::ctrlLowLevelClock(bool enable)
{
#ifdef DEBUG
    printf("ctrlLowLevelClock()\n");
#endif // DEBUG
    if (enable)
    {
        mRegAccr->writeRegister(FLASH_CTRL_BANK, R_LLEVEL_PROTECT_KEY,
                V_LOW_LV_CLOCK_ENABLE);
    }
    else
    {
        mRegAccr->writeRegister(FLASH_CTRL_BANK, R_LLEVEL_PROTECT_KEY,
                V_LOW_LV_CLOCK_DISABLE);
    }
}

void Plp239FwUpdater::releaseQuadMode()
{
#ifdef DEBUG
    printf("releaseQuadMode()\n");
#endif // DEBUG
    mRegAccr->writeRegister(4, 0x18, 0xcc);
    mRegAccr->writeRegister(4, 0x19, 0x01);

    mRegAccr->writeRegister(4, 0x2f, 0x10);
    mRegAccr->writeRegister(4, 0x1e, 0x01);
    mRegAccr->writeRegister(4, 0x1f, 0x00);
    mRegAccr->writeRegister(4, 0x20, 0x00);
    mRegAccr->writeRegister(4, 0x21, 0x00);
    mRegAccr->writeRegister(4, 0x22, 0x00);
    mRegAccr->writeRegister(4, 0x30, 0x00);
    mRegAccr->writeRegister(4, 0x31, 0x00);
    mRegAccr->writeRegister(4, 0x32, 0x00);
    mRegAccr->writeRegister(4, 0x2c, 0x04);

    mRegAccr->writeRegister(4, 0x2f, 0x41);
    mRegAccr->writeRegister(4, 0x1a, 0xff);
    mRegAccr->writeRegister(4, 0x1e, 0x00);
    mRegAccr->writeRegister(4, 0x1f, 0x00);
    mRegAccr->writeRegister(4, 0x2c, 0x02);

    mRegAccr->writeRegister(4, 0x2f, 0x41);
    mRegAccr->writeRegister(4, 0x1a, 0xff);
    mRegAccr->writeRegister(4, 0x1e, 0x00);
    mRegAccr->writeRegister(4, 0x1f, 0x00);
    mRegAccr->writeRegister(4, 0x2c, 0x02);

    mRegAccr->writeRegister(4, 0x2f, 0x00);
    mRegAccr->writeRegister(4, 0x1a, 0xff);
    mRegAccr->writeRegister(4, 0x1e, 0x00);
    mRegAccr->writeRegister(4, 0x1f, 0x00);
    mRegAccr->writeRegister(4, 0x2c, 0x02);

    mRegAccr->writeRegister(4, 0x2f, 0x41);
    mRegAccr->writeRegister(4, 0x1a, 0xff);
    mRegAccr->writeRegister(4, 0x1e, 0x00);
    mRegAccr->writeRegister(4, 0x1f, 0x00);
    mRegAccr->writeRegister(4, 0x2c, 0x02);

    mRegAccr->writeRegister(4, 0x2f, 0x41);
    mRegAccr->writeRegister(4, 0x1a, 0xff);
    mRegAccr->writeRegister(4, 0x1e, 0x00);
    mRegAccr->writeRegister(4, 0x1f, 0x00);
    mRegAccr->writeRegister(4, 0x2c, 0x02);

    mRegAccr->writeRegister(4, 0x2f, 0x00);
    mRegAccr->writeRegister(4, 0x1a, 0xff);
    mRegAccr->writeRegister(4, 0x1e, 0x00);
    mRegAccr->writeRegister(4, 0x1f, 0x00);
    mRegAccr->writeRegister(4, 0x2c, 0x02);

    mRegAccr->writeRegister(4, 0x2f, 0x00);
    mRegAccr->writeRegister(4, 0x1a, 0x01);
    mRegAccr->writeRegister(4, 0x1e, 0x02);
    mRegAccr->writeRegister(4, 0x1f, 0x00);
    mRegAccr->writeRegister(4, 0x24, 0x00);
    mRegAccr->writeRegister(4, 0x25, 0x00);
    mRegAccr->writeRegister(4, 0x26, 0x00);
    mRegAccr->writeRegister(4, 0x27, 0x00);
    mRegAccr->writeRegister(4, 0x2c, 0x02);

    mRegAccr->writeRegister(4, 0x2f, 0x00);
    mRegAccr->writeRegister(4, 0x1a, 0xff);
    mRegAccr->writeRegister(4, 0x1e, 0x01);
    mRegAccr->writeRegister(4, 0x1f, 0x00);
    mRegAccr->writeRegister(4, 0x24, 0xff);
    mRegAccr->writeRegister(4, 0x25, 0xff);
    mRegAccr->writeRegister(4, 0x26, 0xff);
    mRegAccr->writeRegister(4, 0x27, 0xff);
    mRegAccr->writeRegister(4, 0x2c, 0x02);

    mRegAccr->writeRegister(4, 0x2f, 0x00);
    mRegAccr->writeRegister(4, 0x1a, 0xab);
    mRegAccr->writeRegister(4, 0x1e, 0x01);
    mRegAccr->writeRegister(4, 0x1f, 0x00);
    mRegAccr->writeRegister(4, 0x20, 0x00);
    mRegAccr->writeRegister(4, 0x21, 0x00);
    mRegAccr->writeRegister(4, 0x22, 0x00);
    mRegAccr->writeRegister(4, 0x2c, 0x04);
}

bool Plp239FwUpdater::tryToDisableCPU()
{
#ifdef DEBUG
    printf("tryToDisableCPU()\n");
#endif // DEBUG
    int retry = 10;
    do
    {
        setBank0Writeable();
        ctrlCPU(false);
        byte valClksPU3 = mRegAccr->readRegister(CLK_POW_BANK, R_CLKS_PU_3);
        byte valClksPD1 = mRegAccr->readRegister(CLK_POW_BANK, R_CLKS_PD_1);
        if ((valClksPU3 == (V_CLKS_3_L_LV_FLASH_CTRL | V_CLKS_3_H_LV_FLASH_CTRL))
                && (valClksPD1 == V_CLKS_1_CPU))
        {
#ifdef DEBUG
            printf("Disable CPU succeed.\n");
#endif // DEBUG			
            return true;
        }
#ifdef DEBUG
        printf("Disable CPU fail, retry\n");
#endif // DEBUG
    } while (--retry > 0);
    return false;
}

bool Plp239FwUpdater::checkFlags()
{
    int flag = mRegAccr->readRegister(IO_BANK, R_BOOT_STATE);
    BootStatus currBootState(flag);
    if (!currBootState.hasFlag(BootStatus::HwReady))
    {
        printf("HwReady flag bit is not set.\n");
        return false;
    }
    if (!currBootState.hasFlag(BootStatus::FwCodePass))
    {
        printf("FwCodePass flag bit is not set.\n");
        return false;
    }
    if (!currBootState.hasFlag(BootStatus::IfbCheckSumPass))
    {
        printf("IfbCheckSumPass flag bit is not set.\n");
        return false;
    }
    if (currBootState.hasFlag(BootStatus::WDog))
    {
        printf("WDog flag bit is not clear.\n");
        return false;
    }
    return true;
}

bool Plp239FwUpdater::insertHidDescHeader(uint32_t ver, int len, long crc)
{
    // Prepare header
    vector<byte> header;
    // Descriptor size.
    auto descSize = mTargetHidDesc.size();
    if (len != -1 && (unsigned int) len != descSize)
    {
        printf("Length does not matched.");
        return false;
    }
    header.push_back(descSize & 0xFF);
    header.push_back((descSize >> 8) & 0xFF);
    // Reserved
    header.push_back(0xFF);
    header.push_back(0xFF);
    // CRC
    uint32_t myCrc = calCheckSum(mTargetHidDesc.data(), mTargetHidDesc.size());
    if (crc != -1 && crc != myCrc)
    {
        printf("CRC does not matched.");
        return false;
    }
    header.push_back(myCrc & 0xFF);
    header.push_back((myCrc >> 8) & 0xFF);
    header.push_back((myCrc >> 16) & 0xFF);
    header.push_back((myCrc >> 24) & 0xFF);
    // Version
    header.push_back(ver & 0xFF);
    header.push_back((ver >> 8) & 0xFF);
    header.push_back((ver >> 16) & 0xFF);
    header.push_back((ver >> 24) & 0xFF);
    // Reserved
    header.push_back(0xFF);
    header.push_back(0xFF);
    header.push_back(0xFF);
    // Check sum complement
    byte sum = 0;
    for (auto& n : header)
        sum += n;
    sum = ~sum + 1;
    header.push_back(sum);

    // Insert header back.
    mTargetHidDesc.insert(mTargetHidDesc.begin(), header.begin(), header.end());

#ifdef DEBUG
    printf("sum: 0x%x\n", sum);
    printf("Header: ");
    for (auto& n : header)
    {
        printf("%02X ", n);
    }
    printf("\n");
#endif //DEBUG

    return true;
}

bool Plp239FwUpdater::insertParamHeader(uint32_t ver, int len, long crc)
{
    /**
     * HEADER
     *  UINT16 Parameter Size
     *  UINT16 Rev
     *  UINT32 CRC
     *  UINT32 Ver
     *  BYTE[3] Revs
     *  BYTE   CheckSumComplement
     * PARAMETER
     *  BYTE[USER_PARAM_SIZE]
     **/
    int headerSize = 2 + 2 + 4 + 4 + 3 + 1;
    vector<byte> header;
    // Parameter Size
    header.push_back(USER_PARAM_SIZE & 0xff);
    header.push_back((USER_PARAM_SIZE >> 8) & 0xff);
    // Reserved
    header.push_back(0xff);
    header.push_back(0xff);
    // CRC
    uint32_t myCrc = calCheckSum(mTargetParameter.data(),
            mTargetParameter.size());
    if (crc != -1 && myCrc != crc)
    {
        printf("CRC does not matched.");
        return false;
    }
    header.push_back(myCrc & 0xFF);
    header.push_back((myCrc >> 8) & 0xFF);
    header.push_back((myCrc >> 16) & 0xFF);
    header.push_back((myCrc >> 24) & 0xFF);
    // Ver
    header.push_back(ver & 0xFF);
    header.push_back((ver >> 8) & 0xFF);
    header.push_back((ver >> 16) & 0xFF);
    header.push_back((ver >> 24) & 0xFF);
    // Reserved
    header.push_back(0xff);
    header.push_back(0xff);
    header.push_back(0xff);
    // Check sum complement
    byte sum = 0;
    for (auto& n : header)
        sum += n;
    sum = ~sum + 1;
    header.push_back(sum);

    // Insert header back.
    mTargetParameter.insert(mTargetParameter.begin(), header.begin(),
            header.end());

#ifdef DEBUG
    printf("sum: 0x%x\n", sum);
    printf("Header: ");
    for (auto& n : header)
    {
        printf("%02X ", n);
    }
    printf("\n");
#endif //DEBUG

    return true;
}

bool Plp239FwUpdater::reset(ResetType type)
{
    switch (type)
    {
    case ResetType::Regular:
    {
        // Disable low level clock
        ctrlLowLevelClock(false);
        // Enable watchdog
        ctrlWatchDog(true);
        // Reset software (firmware)
        resetSofeware();
        usleep(50000);
        // Enable CPU.
        setBank0Writeable();
        ctrlCPU(true);
        usleep(20000);
#ifdef DEBUG
        printf("BootStatus - ");
        unsigned n = mRegAccr->readRegister(IO_BANK, R_BOOT_STATE);
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
#endif // DEBUG
        if (!checkFlags())
            return false;
    }
        break;
    case ResetType::HwTestMode:
    {
        clearHidFwReadyFlag();
        usleep(200000);
        // Disable watchdog
        ctrlWatchDog(false);
        // Disable CPU
        bool isCpuDisabled = tryToDisableCPU();
        if (!isCpuDisabled)
        {
            printf("Failed to disable CPU");
            return false;
        }
#ifdef DEBUG
        unsigned n = mRegAccr->readRegister(IO_BANK, R_BOOT_STATE);
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
#endif // DEBUG			
        // Enable low level clock.
        ctrlLowLevelClock(true);
        releaseQuadMode();
    }
        break;
    default:
        return false;
    }

    return true;
}

bool loadBin2Vec(ifstream &ifs, vector<byte> &vec)
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

bool Plp239FwUpdater::loadFwBin(const char * path)
{
    bool ret;
    printf("Binary path: %s\n", path);
    ifstream ifs(path, ifstream::in | ios::ate);
    loadBin2Vec(ifs, mTargetFirmware);
    ifs.close();
    return ret;
}

bool Plp239FwUpdater::loadParameterBin(const char * path)
{
    bool ret;
    printf("Binary path: %s\n", path);
    ifstream ifs(path, ifstream::in | ios::ate);
    loadBin2Vec(ifs, mTargetParameter);
    ifs.close();
    return ret;
}

bool Plp239FwUpdater::loadHidDescBin(const char * path)
{
    bool ret;
    printf("Binary path: %s\n", path);
    ifstream ifs(path, ifstream::in | ios::ate);
    loadBin2Vec(ifs, mTargetHidDesc);
    ifs.close();
    return ret;
}

void Plp239FwUpdater::releaseFwBin()
{
    mTargetFirmware.clear();
}

bool Plp239FwUpdater::loadHidDescFile(char const* path)
{
    printf("HID Descriptor file path: %s\n", path);
    ifstream ifs(path);
    std::string line;
    mTargetHidDesc.clear();

    uint32_t version = 0;
    int lengthFromFile = -1;
    long crcFromFile = -1;

    regex verP("Version:\\s*(\\d+).(\\d+).(\\d+).(\\d+)",
            regex_constants::icase);
    regex lenP("Length:\\s*0x([0-9|a-f]+) \\((\\d+)\\)",
            regex_constants::icase);
    regex crcP("CRC:\\s*0x([0-9|a-f]+)", regex_constants::icase);
    while (std::getline(ifs, line))
    {
        const auto strBegin = line.find_first_not_of(" \t");
        if (strBegin == std::string::npos)
            continue;
        if (line.find("#") == strBegin)
        {
            printf("Comment line: %s\n", line.c_str());

            smatch matches;
            if (regex_search(line, matches, verP))
            {
                if (matches.size() != 5)
                    continue;

                for (size_t i = 1; i < matches.size(); ++i)
                {
                    version += stoi(matches[i].str(), nullptr, 16)
                            << ((3 - i) * 8);
                }
                printf("...Found version (0x%X).\n", version);
            }
            else if (regex_search(line, matches, lenP))
            {
                if (matches.size() != 3)
                    continue;

                int lenHex = stoi(matches[1].str(), nullptr, 16);
                int lenDec = stoi(matches[2].str());
                if (lenHex != lenDec)
                    continue;
                lengthFromFile = lenHex;
                printf("...Found length (%d)\n", lengthFromFile);
            }
            else if (regex_search(line, matches, crcP))
            {
                crcFromFile = stoul(matches[1].str(), nullptr, 16);
                printf("...Found CRC: 0x%" PRIx64 "\n", crcFromFile);
            }
            continue;
        }

        int b = stoi(line, nullptr, 16);
#ifdef DEBUG            
        printf("%02X ", b);
        if (mTargetHidDesc.size()%30==29)
            printf("\n");
#endif //DEBUG
        mTargetHidDesc.push_back(b);
    }

#ifdef DEBUG
    printf("\n");
    printf("HID Descriptor length: %lu\n", mTargetHidDesc.size());
#endif //DEBUG

    // Insert header
    if (!insertHidDescHeader(version, lengthFromFile, crcFromFile))
    {
        printf("Prepare header failed, please check.");
        return false;
    }

#ifdef DEBUG
    printf("HID Descriptor (with header) length: %lu\n", mTargetHidDesc.size());
#endif //DEBUG    
    return true;
}

bool Plp239FwUpdater::loadParameterFile(char const* path)
{
    printf("Parameter file path: %s\n", path);
    ifstream ifs(path);
    std::string line;
    mTargetParameter.clear();
    mTargetParameter.resize(USER_PARAM_SIZE);

    int64_t crcFromFile = -1;
    uint32_t version = 0;
    int parmNumFromFile = -1;

    regex verP("Version:\\s*(\\d+).(\\d+).(\\d+).(\\d+)",
            regex_constants::icase);
    regex chkSumP("Checksum:\\s*0x([0-9|a-f]+)", regex_constants::icase);
    regex pairNumP("Number of parameters:\\s*(\\d+)", regex_constants::icase);

    regex paiP("^\\s*(?:(\\S+)\\s*,\\s*(\\S+)\\s*,\\s*(\\w+)?)\\s*(?:#.*)?$",
            regex_constants::icase);

    while (std::getline(ifs, line))
    {
        const auto strBegin = line.find_first_not_of(" \t");
        if (strBegin == std::string::npos)
            continue;
        if (line.find("#") == strBegin)
        {
            printf("Comment line: %s\n", line.c_str());

            smatch matches;
            if (regex_search(line, matches, verP))
            {
                if (matches.size() != 5)
                    continue;

                for (size_t i = 1; i < matches.size(); ++i)
                {
                    version += stoi(matches[i].str(), nullptr, 16)
                            << ((3 - i) * 8);
                }
                printf("...Found version (0x%X).\n", version);
            }
            else if (regex_search(line, matches, chkSumP))
            {
                crcFromFile = stoul(matches[1].str(), nullptr, 16);
                printf("...Found check sum: 0x%" PRIx64 "\n", crcFromFile);
            }
            else if (regex_search(line, matches, pairNumP))
            {
                parmNumFromFile = stoi(matches[1].str());
                printf("...Found Parameter number: %d\n", parmNumFromFile);
            }

            continue;
        }

        // Parser parameter pair.
        smatch matches;
        uint8_t bank, reg, val;
        if (!regex_search(line, matches, paiP))
            continue;
        if (matches.size() != 4)
            continue;
        if (matches[1].str().find("0x") == 0)
            bank = stoi(matches[1].str(), nullptr, 16);
        else
            bank = stoi(matches[1].str());
        if (matches[2].str().find("0x") == 0)
            reg = stoi(matches[2].str(), nullptr, 16);
        else
            reg = stoi(matches[2].str());
        if (matches[3].str().find("0x") == 0)
            val = stoi(matches[3].str(), nullptr, 16);
        else
            val = stoi(matches[3].str());

        mTargetParameter[bank * USER_PARAM_BANK_SIZE + reg] = val;
    }
#ifdef DEBUG
    printf("Value of all register: \n");
    int i = 0;
    for (auto& n : mTargetParameter)
    {
        printf("%02X ", n);
        if (i++%30==29) printf("\n");
    }
    printf("\n");
#endif //DEBUG
    // Prepare header
    if (!insertParamHeader(version, parmNumFromFile, crcFromFile))
    {
        printf("Prepare header failed, please check.");
        return false;
    }
#ifdef DEBUG
    printf("Parameter (with header) length: %lu\n", mTargetParameter.size());
#endif //DEBUG   
    return true;
}

void Plp239FwUpdater::releaseParameterBin()
{
    mTargetParameter.clear();
}

bool Plp239FwUpdater::loadUpgradeBin(char const* path)
{
    static const int UPGRADE_FILE_SIZE = 45056 + 4096 + 8192;
    printf("Upgrade file path: %s\n", path);
    ifstream ifs(path, ifstream::in | ios::ate);
    ifs.unsetf(std::ios::skipws);
    int size = ifs.tellg();
    if (size < UPGRADE_FILE_SIZE)
    {
        printf("File size too small. (%d)\n", size);
        return false;
    }
    else if (size > UPGRADE_FILE_SIZE)
    {
        printf("File size too large. (%d > %d)\n", size, UPGRADE_FILE_SIZE);
        return false;
    }

    ifs.seekg(0, ios::beg);
    // Read code.
    mTargetFirmware.clear();
    mTargetFirmware.reserve(45056);
    copy_n(istream_iterator<byte>(ifs), 45056,
            std::back_inserter(mTargetFirmware));
    // Read parameters.
    mTargetParameter.clear();
    mTargetParameter.reserve(4096);
    std::copy_n(istream_iterator<byte>(ifs), 4096,
            std::back_inserter(mTargetParameter));
    // Read HID descriptor.
    mTargetHidDesc.clear();
    mTargetHidDesc.reserve(8192);
    std::copy_n(istream_iterator<byte>(ifs), 8192,
            std::back_inserter(mTargetHidDesc));
    ifs.close();
    return true;
}

void Plp239FwUpdater::releaseUpgradeBin()
{
    this->releaseFwBin();
    this->releaseParameterBin();
    this->releaseHidDescBin();
}

uint32_t Plp239FwUpdater::calCheckSum(byte const * const array, int length)
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

void Plp239FwUpdater::releaseHidDescBin()
{
    mTargetHidDesc.clear();
}

int Plp239FwUpdater::getFwVersion()
{
    int fwVer;
    fwVer = mRegAccr->readRegister(CLK_POW_BANK, R_FW_VER_H);
    fwVer = (fwVer << 8) | mRegAccr->readRegister(CLK_POW_BANK, R_FW_VER_L);
    return fwVer;
}

int Plp239FwUpdater::getReadSysRegister(byte bank,byte addr)
{
    int value;
    value = mRegAccr->readRegister(bank, addr);
    return value;
}

int Plp239FwUpdater::getReadUserRegister(byte bank,byte addr)
{
    int value;
    value = mRegAccr->readuserRegister(bank, addr);
    return value;
}

bool Plp239FwUpdater::fullyUpgrade()
{
    // 1. Make sure upgrade binaries are ready.
    if (mTargetFirmware.size() <= 0)
        return false;
    if (mTargetParameter.size() == 0)
        return false;
    if (mTargetHidDesc.size() == 0)
        return false;
    // 2. Erase firmware first.
    bool res = mFlashCtrlr->erase(Plp239FlashCtrlr::FIRMWARE_START_PAGE,
            Plp239FlashCtrlr::FIRMWARE_SIZE_IN_PAGE);
    if (!res)
    {
        printf("Erase firmware failed.");
        return false;
    }
    // 3. Flash parameter.
    writeParameter();
    // 4. Flash HID descriptor.
    writeHidDesc();
    // 5. Flash firmware without erase.
    writeFirmware(false);
    return true;
}

void Plp239FwUpdater::writeFirmware(bool erase)
{
    if (mTargetFirmware.size() <= 0)
        return;
    int res;
    res = mFlashCtrlr->writeFlash(mTargetFirmware.data(),
            mTargetFirmware.size(), Plp239FlashCtrlr::FIRMWARE_START_PAGE,
            erase);
    printf("writeFirmware() result: %d\n", res);
}

void Plp239FwUpdater::writeHidDesc()
{
    if (mTargetHidDesc.size() == 0)
        return;
    int res;
    res = mFlashCtrlr->writeFlash(mTargetHidDesc.data(), mTargetHidDesc.size(),
            Plp239FlashCtrlr::HID_DESCRIPTOR_START_PAGE, true);
    printf("writeHidDesc() result: %d\n", res);
}

void Plp239FwUpdater::writeParameter()
{
    if (mTargetParameter.size() == 0)
        return;
    int res;
    res = mFlashCtrlr->writeFlash(mTargetParameter.data(),
            mTargetParameter.size(), Plp239FlashCtrlr::PARAMETER_START_PAGE,
            true);
    printf("writeParameter() result: %d\n", res);
}
