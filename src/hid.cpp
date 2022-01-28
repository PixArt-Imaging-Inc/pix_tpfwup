#include <stdio.h>
#include <string.h>
#include <iostream>
#include <string>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "hid.h"
#include "utilities.h"

#define HIDRD_ITEM_PFX_TAG_MASK   0x0F
#define HIDRD_ITEM_PFX_TAG_SHFT   4

#define HIDRD_ITEM_PFX_TAG_LONG   0xF
#define HIDRD_ITEM_PFX_TYPE_MASK  0x03
#define HIDRD_ITEM_PFX_TYPE_SHFT  2

//#define DEBUG
using namespace std;

namespace pixart
{
    // Utility function
    const char* typeToString(ShortItemType type)
    {
        switch (type)
        {
        case Main:
            return "Main";
        case Global:
            return "Global";
        case Local:
            return "Local";
        case Reserved:
            return "Reserved";
        default:
            return "Invalid";
        }
    }

    const char* mainTagToString(ShortItemMainTag tag)
    {
        switch (tag)
        {
        case MAIN_TAG_INPUT:
            return "MAIN_TAG_INPUT";
        case MAIN_TAG_OUTPUT:
            return "MAIN_TAG_OUTPUT";
        case MAIN_TAG_COLLECTION:
            return "MAIN_TAG_COLLECTION";
        case MAIN_TAG_FEATURE:
            return "MAIN_TAG_FEATURE";
        case MAIN_TAG_END_COLLECTION:
            return "MAIN_TAG_END_COLLECTION";
        default:
            return "Unknown tag!!";
        }
    }

    const char* globalTagToString(ShortItemGlobalTag tag)
    {
        switch (tag)
        {
        case GLOBAL_TAG_USAGE_PAGE:
            return "GLOBAL_TAG_USAGE_PAGE";
        case GLOBAL_TAG_LOGICAL_MINIMUM:
            return "GLOBAL_TAG_LOGICAL_MINIMUM";
        case GLOBAL_TAG_LOGICAL_MAXIMUM:
            return "GLOBAL_TAG_LOGICAL_MAXIMUM";
        case GLOBAL_TAG_PHYSICAL_MINIMUM:
            return "GLOBAL_TAG_PHYSICAL_MINIMUM";
        case GLOBAL_TAG_PHYSICAL_MAXIMUM:
            return "GLOBAL_TAG_PHYSICAL_MAXIMUM";
        case GLOBAL_TAG_UNIT_EXPONENT:
            return "GLOBAL_TAG_UNIT_EXPONENT";
        case GLOBAL_TAG_UNIT:
            return "GLOBAL_TAG_UNIT";
        case GLOBAL_TAG_REPORT_SIZE:
            return "GLOBAL_TAG_REPORT_SIZE";
        case GLOBAL_TAG_REPORT_ID:
            return "GLOBAL_TAG_REPORT_ID";
        case GLOBAL_TAG_REPORT_COUNT:
            return "GLOBAL_TAG_REPORT_COUNT";
        case GLOBAL_TAG_PUSH:
            return "GLOBAL_TAG_PUSH";
        case GLOBAL_TAG_POP:
            return "GLOBAL_TAG_POP";
        default:
            return "Unknown";
        }
    }

    const char* localTagToString(ShortItemLocalTag tag)
    {
        switch (tag)
        {
        case LOCAL_TAG_USAGE:
            return "LOCAL_TAG_USAGE";
        case LOCAL_TAG_USAGE_MINIMUM:
            return "LOCAL_TAG_USAGE_MINIMUM";
        case LOCAL_TAG_USAGE_MAXIMUM:
            return "LOCAL_TAG_USAGE_MAXIMUM";
        case LOCAL_TAG_DESIGNATOR_INDEX:
            return "LOCAL_TAG_DESIGNATOR_INDEX";
        case LOCAL_TAG_DESIGNATOR_MINIMUM:
            return "LOCAL_TAG_DESIGNATOR_MINIMUM";
        case LOCAL_TAG_DESIGNATOR_MAXIMUM:
            return "LOCAL_TAG_DESIGNATOR_MAXIMUM";
        case LOCAL_TAG_INVALID:
            return "LOCAL_TAG_INVALID";
        case LOCAL_TAG_STRING_INDEX:
            return "LOCAL_TAG_STRING_INDEX";
        case LOCAL_TAG_STRING_MINIMUM:
            return "LOCAL_TAG_STRING_MINIMUM";
        case LOCAL_TAG_STRING_MAXIMUM:
            return "LOCAL_TAG_STRING_MAXIMUM";
        case LOCAL_TAG_DELIMITER:
            return "LOCAL_TAG_DELIMITER";
        default:
            return "Unknown";
        }
    }
    // ==========================
    Item::Item(uint8_t* buf, uint16_t size)
    {
        mBuf = new uint8_t[size];
        memcpy(mBuf, buf, size);
        mSize = size;
    }

    Item::~Item()
    {
        delete[] mBuf;
    }

    // Implement ShortItem
    ShortItem::ShortItem(uint8_t * buf, uint16_t length) :
            Item(buf, length)
    {
        mTag = (*buf >> HIDRD_ITEM_PFX_TAG_SHFT) &
        HIDRD_ITEM_PFX_TAG_MASK;
        mType = (*buf >> HIDRD_ITEM_PFX_TYPE_SHFT) &
        HIDRD_ITEM_PFX_TYPE_MASK;
    }

    ShortItem::~ShortItem()
    {
    }

    uint8_t* ShortItem::getData()
    {
        return mBuf + 1;
    }

    int ShortItem::getTag() const
    {
        return mTag;
    }

    ShortItemType ShortItem::getType() const
    {
        if (mType >= ShortItemType::Invalid)
            return ShortItemType::Invalid;
        return (ShortItemType) mType;
    }

    string ShortItem::toString()
    {
        const char * tag;

        switch (getType())
        {
        case Main:
            tag = mainTagToString((ShortItemMainTag) getTag());
            break;
        case Global:
            tag = globalTagToString((ShortItemGlobalTag) getTag());
            break;
        case Local:
            tag = localTagToString((ShortItemLocalTag) getTag());
            break;
        default:
            tag = "Unknown";
        }
        string out = fmt("Type: %6s Tag: %30s   ==> ", typeToString(getType()),
                tag);
        for (int i = 0; i < mSize; i++)
        {
            out += fmt("%02hhx ", mBuf[i]);
            if ((i + 1) % 30 == 0)
                out += "\n";
        }

#ifdef DEBUG
        printf("Type: %6s Tag: %30s   ==> ", typeToString(getType()), tag);

        for (int i = 0; i < mSize; i++)
        {
            printf("%02hhx ", mBuf[i]);
            if ((i+1)%30 == 0)
            puts("\n");
        }
        printf("\n");
#endif //DEBUG
        return out;
    }
    // =====================
    LongItem::LongItem(uint8_t * buf, uint16_t length) :
            Item(buf, length)
    {
    }

    LongItem::~LongItem()
    {
    }

    uint8_t* LongItem::getData()
    {
        return mBuf + 3;
    }

    string LongItem::toString()
    {
        string out = "Type: LongItem    ==> ";
        for (int i = 0; i < mSize; i++)
        {
            out += fmt("%02hhx ", mBuf[i]);
            if ((i + 1) % 30 == 0)
                out += "\n";
        }
        return out;
    }
    // ======================
    ReportDescriptor::ReportDescriptor(hidraw_report_descriptor& rptDescRaw)
    {
        parser(rptDescRaw.value, rptDescRaw.size);
    }

    ReportDescriptor::~ReportDescriptor()
    {
        clear();
    }

    void ReportDescriptor::clear()
    {
        for (vector<Item*>::iterator it = mItems.begin(); it != mItems.end();
                it++)
        {
            delete *it;
        }
        mItems.clear();
    }

    void ReportDescriptor::parser(uint8_t * buf, int length)
    {
        // Get prefix.
        uint8_t prefix = buf[0];
        int size;

        // Check Tag.
        uint8_t tag = (prefix >> HIDRD_ITEM_PFX_TAG_SHFT) &
        HIDRD_ITEM_PFX_TAG_MASK;

        bool isLongItem = tag == HIDRD_ITEM_PFX_TAG_LONG ? true : false;

        Item* item;
        if (isLongItem)
        {
            static const int HIDRD_ITEM_LONG_MIN_SIZE = 3;
            size = HIDRD_ITEM_LONG_MIN_SIZE + buf[1];
            item = new LongItem(buf, size);
        }
        else
        {
            static const int HIDRD_ITEM_PFX_SIZE_4B = 3;
            int prefixSize = prefix & 0x3;

            size = 1 + (prefixSize == HIDRD_ITEM_PFX_SIZE_4B ? 4 : prefixSize);
            uint8_t type = (prefix >> HIDRD_ITEM_PFX_TYPE_SHFT) &
            HIDRD_ITEM_PFX_TYPE_MASK;

            item = new ShortItem(buf, size);
        }

        mItems.push_back(item);
        int newLength = length - size;

        if (newLength >= 0)
            parser(buf + size, newLength);
    }

    string ReportDescriptor::toString()
    {
        string out;
        for (vector<Item*>::iterator it = mItems.begin(); it != mItems.end();
                it++)
        {
            out += (*it)->toString() + "\n";
        }
        return out;
    }
    // ======================
    HidDevice::~HidDevice()
    {
    }

    bool HidDevice::open(char* path, int flags)
    {
        if (mDevHandle != 0)
        {
            ::close(mDevHandle);
        }
        mDevHandle = ::open(path, flags);
        if (mDevHandle < 0)
        {
            perror("Unable to open device");
            return false;
        }
        memset(&mRptDescRaw, 0x0, sizeof(mRptDescRaw));
        return true;
    }

    void HidDevice::close()
    {
        if (mDevHandle <= 0)
            return;
        ::close(mDevHandle);
    }

    shared_ptr<ReportDescriptor> HidDevice::getReportDescriptor()
    {
        // Clear old ReportDesciptor
        mRptDesc = nullptr;
        /* Get Report Descriptor Size */
        int descSize;
        int res = ioctl(mDevHandle, HIDIOCGRDESCSIZE, &descSize);
        if (res < 0)
        {
            perror("HIDIOCGRDESCSIZE");
            return nullptr;
        }
#ifdef DEBUG
        else
        printf("Report Descriptor Size: %d\n", descSize);
#endif //DEBUG

        /* Get Report Descriptor */
        mRptDescRaw.size = descSize;
        res = ioctl(mDevHandle, HIDIOCGRDESC, &mRptDescRaw);
        if (res < 0)
        {
            perror("HIDIOCGRDESC");
            return nullptr;
        }
        else
        {
#ifdef DEBUG
            printf("Report Descriptor:\n");
            for (int i = 0; i < mRptDescRaw.size; i++)
            {
                printf("%02hhx ", mRptDescRaw.value[i]);
                if ((i+1)%30 == 0)
                printf("\n");
            }
            puts("\n");
#endif //DEBUG

            mRptDesc = make_shared<ReportDescriptor>(mRptDescRaw);
        }
        return mRptDesc;
    }

    string HidDevice::getRawName()
    {
        /* Get Raw Name */
        char buf[256];
        int res = ioctl(mDevHandle, HIDIOCGRAWNAME(256), buf);
        if (res < 0)
        {
            perror("HIDIOCGRAWNAME");
            return "";
        }
#ifdef DEBUG
        else
        printf("Raw Name: %s\n", buf);

        /* Get Physical Location */
        res = ioctl(mDevHandle, HIDIOCGRAWPHYS(256), buf);
        if (res < 0)
        {
            perror("HIDIOCGRAWPHYS");
            return false;
        }

        else
        printf("Raw Phys: %s\n", buf);
#endif //DEBUG
        return string(buf);
    }

    bool HidDevice::getRawInfo(hidraw_devinfo* rawInfo)
    {
        if (rawInfo == 0)
            return false;

        memset(rawInfo, 0x0, sizeof(hidraw_devinfo));
        int res = ioctl(mDevHandle, HIDIOCGRAWINFO, rawInfo);
        if (res < 0)
        {
            perror("HIDIOCGRAWINFO failed");
            return false;
        }
#ifdef DEBUG
        else
        {
            printf("RAW Info: \n");
            printf("\tBusType: %d\n", rawInfo->bustype);
            printf("\tvid: 0x%04hx\n", rawInfo->vendor);
            printf("\tpid: 0x%04hx\n", rawInfo->product);
        }
#endif //DEBUG		
        return true;
    }

    bool HidDevice::setFeature(byte reportId, byte *data, int length)
    {
        bool ret;
        byte* buffer = new byte[length + 1];
        memcpy(buffer + 1, data, length);
        buffer[0] = reportId;
#ifdef DEBUG                
        printf("setFeature():\n before: ");
        for (int i = 0; i < length+1; i++)
        {
            printf("%02hhx ", buffer[i]);
            if ((i+1)%30 == 0)
            puts("\n");
        }
        printf("\n");
#endif
        int res = ioctl(mDevHandle, HIDIOCSFEATURE(length + 1), buffer);
        if (res < 0)
        {
            perror("Failed to HIDIOCSFEATURE.");
            ret = false;
        }
        else
        {
            ret = true;
        }

#ifdef DEBUG
        printf(" after : ");
        for (int i = 0; i < res; i++)
        {
            printf("%02hhx ", buffer[i]);
            if ((i+1)%30 == 0)
            puts("\n");
        }
        printf("\n");
#endif

        delete[] buffer;
        return ret;
    }

    bool HidDevice::getFeature(byte reportId, byte *data, int* length,
            int maxLength)
    {
        bool ret;
        byte *buffer = new byte[maxLength];
        memset(buffer, 0xcc, maxLength);
        buffer[0] = reportId; /* Report Number */

#ifdef DEBUG
        printf("getFeature():\n before: ");
        for (int i = 0; i < 4; i++)
        {
            printf("%02hhx ", buffer[i]);
            if ((i+1)%30 == 0)
            puts("\n");
        }
        printf("\n");
#endif

        int res = ioctl(mDevHandle, HIDIOCGFEATURE(maxLength), buffer);

#ifdef DEBUG            
        printf("getFeature(), res: %d\n", res);
        printf(" after : ");
        for (int i = 0; i < res; i++)
        {
            printf("%02hhx ", buffer[i]);
            if ((i+1)%30 == 0)
            puts("\n");
        }
        printf("\n");
#endif

        if (res < 0)
        {
            perror("Failed to HIDIOCGFEATURE");
            ret = false;
        }
        else
        {
            *length = res;

            if (res > 0)
            {
                if (res > maxLength)
                    res = maxLength;
                memcpy(data, buffer, res);
            }
            ret = true;
        }
        delete[] buffer;
        return ret;
    }
    bool HidDevice::Read(void)
    {

	byte* buffer = new byte[20];	
	int ret = read(mDevHandle,buffer,20);
        /*
	if(ret <0)
        {
		perror("read");
	}
	else
	{
		printf("read() read %d bytes:\n\t",ret);
		for(int i=0;i<ret;i++)
                   printf("%hhx",buffer[i]);
		
	} 
        */       
	/*
	printf("\nret=%d",ret);
	for(int i =0; i<32;i++)
        {
	  printf("(i=%d,data=%d)",i,buffer[i]);	
	}
*/
	delete[] buffer;
        return ret;

    }
}
