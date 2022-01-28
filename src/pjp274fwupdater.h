#ifndef __PJP274_FIRMWARE_UPDATER__
#define __PJP274_FIRMWARE_UPDATER__
#include "devhelper.h"
#include "registeraccessor.h"
#include "pjp274flashctrlr.h"
#include "type.h"

#include <memory>
#include <vector>

namespace pixart
{
    class Pjp274FwUpdater
    {
    private:
        DevHelper* mDevHelper;
        RegisterAccessor* mRegAccr;

        std::shared_ptr<Pjp274FlashCtrlr> mFlashCtrlr;

        std::vector<byte> mTargetFirmware;
        std::vector<byte> mTargetHidDesc;
        std::vector<byte> mTargetParameter;

        
        /* ============== CPU system control ============== */
        static const byte CPU_SYS_BANK;
        // Trigger software (firmware) system reset
       
        /* ============= Flash & Test control ============== */
        static const byte FLASH_CTRL_BANK;
       
        /* ==================== IO Bank ==================== */
        static const byte IO_BANK;
        // Control watch dog.
        
        /* ================= User Parameter ================= */
        static const int USER_PARAM_BANK_SIZE;
        static const int USER_PARAM_SIZE;
 
        bool insertParamHeader(uint32_t ver, int len, long crc);
    public:
        enum class ResetType
        {
            Regular, HwTestMode,
        };

        Pjp274FwUpdater(DevHelper* devHelper, RegisterAccessor* regAccr);
        bool reset(ResetType type);
        /** Binary **/
        bool loadFwBin(char const* fwPath);
        void releaseFwBin();
        bool loadParameterBin(char const* path);
        void releaseParameterBin();

        bool loadUpgradeBin(char const* path);
        void releaseUpgradeBin();
        /** Text **/
        bool loadParameterFile(char const* path);
        /**********/
        int getICType();
        int getFwVersion();
        bool fullyUpgrade();
        void writeFirmware(bool erase = true);
        void writeParameter();

        uint32_t calCheckSum(byte const * const array, int length);
		
	int getReadSysRegister(byte bank,byte addr);
	int getReadUserRegister(byte bank,byte addr);
	void ReadFrameData();
	void ReadBatchUserRegister(byte bank, int length,bool AutoRead);
	void ReadBatchSysRegister(byte bank, int length,bool AutoRead);
	void writeRegister(byte bank,byte addr,byte value);
	void writeUserRegister(byte bank,byte addr,byte value);
	
    };
}

#endif //__PLP239_FIRMWARE_UPDATER__
