#ifndef __LINUX_REGISTER_ACCESSOR_pjp255__
#define __LINUX_REGISTER_ACCESSOR_pjp255__
#include "registeraccessor.h"
#include "hid.h"

namespace pixart
{
    class pjp255RegAccr: public RegisterAccessor
    {
    private:
        HidDevice* mHidDevice;

        uint64_t mMaxBurstLength;

    public:
        pjp255RegAccr(HidDevice* hidDevice);
        virtual ~pjp255RegAccr();
        virtual void prepare();
        virtual byte readRegister(byte bank, byte address);
		virtual byte readuserRegister(byte bank, byte address);
        virtual int readRegisters(byte* data, byte bank, byte startAddress,
                uint64_t length);
        virtual int burstReadRegister(byte* data, byte bank, byte address,
                uint64_t length);
        virtual void writeRegister(byte bank, byte address, byte value);
        virtual void writeRegisters(byte bank, byte startAddress, byte* values,
                uint64_t length);
        virtual void burstWriteRegister(byte bank, byte address, byte* values,
                uint64_t length);
        virtual void writeUserRegister(byte bank, byte address, byte value);
	virtual int readUserRegisters(byte* data, byte bank, byte startAddress,
        uint64_t length);
        virtual byte readInputReport(void);
    };
}

#endif //__LINUX_REGISTER_ACCESSOR__
