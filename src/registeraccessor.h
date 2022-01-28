#ifndef __REGISTER_ACCESSOR__
#define __REGISTER_ACCESSOR__
#include "type.h"

namespace pixart
{
    class RegisterAccessor
    {
    public:
        virtual ~RegisterAccessor()
        {};
        /**
         * Prepare capability of register accessor.
         * Include reload profile, reset device, etc.
         **/
        virtual void prepare() = 0;
        /**
         * Read register value, return -1 if read failed.
         **/
        virtual byte readRegister(byte bank, byte address) = 0;
		virtual byte readuserRegister(byte bank, byte address) = 0;
        virtual int readRegisters(byte* data, byte bank, byte startAddress,
                uint64_t length) = 0;
        virtual int burstReadRegister(byte* data, byte bank, byte address,
                uint64_t length) = 0;
        virtual void writeRegister(byte bank, byte address, byte value) = 0;
        virtual void writeRegisters(byte bank, byte startAddress, byte* values,
                uint64_t length) = 0;
        virtual void burstWriteRegister(byte bank, byte address, byte* values,
                uint64_t length) = 0;
        virtual byte readInputReport(void) = 0;
	virtual void writeUserRegister(byte bank, byte address, byte value) = 0;
	virtual int readUserRegisters(byte* data, byte bank, byte startAddress,
        uint64_t length) = 0;
    };
}

#endif // __REGISTER_ACCESSOR__
