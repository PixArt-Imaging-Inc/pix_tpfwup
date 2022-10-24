#ifndef __PLP239_FIRMWARE_UPDATER__
#define __PLP239_FIRMWARE_UPDATER__
#include "devhelper.h"
#include "registeraccessor.h"
#include "plp239flashctrlr.h"
#include "type.h"

#include <memory>
#include <vector>

namespace pixart
{
    class Plp239FwUpdater
    {
    private:
        DevHelper* mDevHelper;
        RegisterAccessor* mRegAccr;

        std::shared_ptr<Plp239FlashCtrlr> mFlashCtrlr;

        std::vector<byte> mTargetFirmware;
        std::vector<byte> mTargetHidDesc;
        std::vector<byte> mTargetParameter;

        /* ============ CPU and power control ============ */
        static const byte CLK_POW_BANK;
        // CPU control
        static const byte R_CLKS_PU_1;
        static const byte R_CLKS_PU_3;
        static const byte R_CLKS_PD_1;
        static const byte R_FW_VER_L;
        static const byte R_FW_VER_H;
        static const byte V_CLKS_1_CPU;
        static const byte V_CLKS_3_L_LV_FLASH_CTRL;
        static const byte V_CLKS_3_H_LV_FLASH_CTRL;
        // Bank 0 writeable control
        static const byte R_BK0_PROTECT_KEY;
        static const byte V_BK0_WRITE_ENABLE;
        /* ============== CPU system control ============== */
        static const byte CPU_SYS_BANK;
        // Trigger software (firmware) system reset
        static const byte R_SW_RESET_KEY_1;
        static const byte R_SW_RESET_KEY_2;
        static const byte V_SW_RESET_KEY_1;
        static const byte V_SW_RESET_KEY_2;
        /* ============= Flash & Test control ============== */
        static const byte FLASH_CTRL_BANK;
        // Low level clock control
        static const byte R_LLEVEL_PROTECT_KEY;
        static const byte V_LOW_LV_CLOCK_DISABLE;
        static const byte V_LOW_LV_CLOCK_ENABLE;
        /* ==================== IO Bank ==================== */
        static const byte IO_BANK;
        // Control watch dog.
        static const byte R_IOA_WD_DISABLE;
        static const byte V_WD_DISABLE_KEY;
        static const byte V_WD_ENABLE_KEY;
        // Boot state flags
        static const byte R_BOOT_STATE;
        /* ================= IO Control Bank ================ */
        static const byte IO_CTRL_BANK;
        // Firmware ready flag
        static const byte R_HID_FW_RDY;

        /* ================= User Parameter ================= */
        static const int USER_PARAM_BANK_SIZE;
        static const int USER_PARAM_SIZE;

        class BootStatus
        {
        private:
            uint16_t mStatusFlags;
        public:
            static const uint16_t HwReady = 1 << 0;
            static const uint16_t FwCodePass = 1 << 1;
            static const uint16_t HidPass = 1 << 2;
            static const uint16_t IfbCheckSumPass = 1 << 3;
            static const uint16_t WDog = 1 << 4;
            static const uint16_t EhiReady = 1 << 5;
            static const uint16_t Error = 1 << 6;
            static const uint16_t NavReady = 1 << 7;

            BootStatus(uint16_t flags) :
                    mStatusFlags(flags)
            {};
            ~BootStatus()
            {};

            bool hasFlag(uint16_t flag)
            {
                if ((mStatusFlags & flag) > 0)
                    return true;
                return false;
            };
        };

        void setBank0Writeable();
        void resetSofeware();
        void clearHidFwReadyFlag();
        void ctrlWatchDog(bool enable);
        void ctrlCPU(bool enable);
        void ctrlLowLevelClock(bool enable);
        void releaseQuadMode();

        bool tryToDisableCPU();
        bool checkFlags();

        bool insertHidDescHeader(uint32_t ver, int len, long crc);
        bool insertParamHeader(uint32_t ver, int len, long crc);
    public:
        enum class ResetType
        {
            Regular, HwTestMode,
        };

        Plp239FwUpdater(DevHelper* devHelper, RegisterAccessor* regAccr);
        bool reset(ResetType type);
        /** Binary **/
        bool loadFwBin(char const* fwPath);
        void releaseFwBin();
        bool loadParameterBin(char const* path);
        void releaseParameterBin();
        bool loadHidDescBin(char const* path);
        void releaseHidDescBin();
        bool loadUpgradeBin(char const* path);
        void releaseUpgradeBin();
        /** Text **/
        bool loadParameterFile(char const* path);
        bool loadHidDescFile(char const* path);
        /**********/

        int getPid();
        int getFwVersion();
        bool fullyUpgrade();
        void writeFirmware(bool erase = true);
        void writeHidDesc();
        void writeParameter();
		

        uint32_t calCheckSum(byte const * const array, int length);
		int getReadSysRegister(byte bank,byte addr);
		int getReadUserRegister(byte bank,byte addr);
		
    };
}

#endif //__PLP239_FIRMWARE_UPDATER__
