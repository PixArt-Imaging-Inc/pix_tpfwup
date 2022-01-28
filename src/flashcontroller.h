#ifndef __FLASHCONTROLLER__
#define __FLASHCONTROLLER__
#include <stdint.h>

#include "type.h"
#include "registeraccessor.h"

namespace pixart
{
    class FlashController
    {
    protected:
        RegisterAccessor* mRegAccessor;

    public:
        FlashController(RegisterAccessor* regAccessor);
        virtual ~FlashController();
        virtual int writeFlash(byte* data, int length, int startPage,
                bool erase) = 0;
        virtual int readFlash(byte** data, int startPage, int pageSize) = 0;
        virtual bool erase(int startPage, int length) = 0;
    };
}

#endif //__FLASHCONTROLLER__