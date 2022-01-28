#ifndef __HID__
#define __HID__
#include <stdint.h>
#include <string>

#include <linux/hidraw.h>
#include <fcntl.h>
#include <sys/types.h>

#include <vector>
#include <memory>

#include "type.h"

namespace pixart
{
    class ReportDescriptor;
    class HidDevice
    {
    private:
        int mDevHandle;
        struct hidraw_report_descriptor mRptDescRaw;
        std::shared_ptr<ReportDescriptor> mRptDesc;

    public:
        ~HidDevice();
        bool open(char *hidrawPath, int flags = O_RDWR );
        void close();
        std::shared_ptr<ReportDescriptor> getReportDescriptor();
        std::string getRawName();
        bool getRawInfo(hidraw_devinfo* rawInfo);
        bool setFeature(byte reportId, byte *data, int length);
        bool getFeature(byte reportId, byte *data, int *length, int maxLength);
	bool Read(void);
    };

    class Item
    {
    protected:
        uint8_t * mBuf;
        uint16_t mSize;
    public:
        Item(uint8_t* buf, uint16_t size);
        virtual ~Item();
        virtual bool isLongItem() const = 0;
        virtual uint8_t* getData() = 0;
        uint16_t getDataSize()
        {
            return mSize - (isLongItem() ? 3 : 1);
        }
        ;
        virtual std::string toString() = 0;
    };

    enum ShortItemType
    {
        Main = 0, Global, Local, Reserved, Invalid,
    };

    enum ShortItemMainTag
    {
        MAIN_TAG_INPUT = 0x08,
        MAIN_TAG_OUTPUT,
        MAIN_TAG_COLLECTION,
        MAIN_TAG_FEATURE,
        MAIN_TAG_END_COLLECTION,
        MAIN_TAG_UNKNOWN,
    };

    enum ShortItemGlobalTag
    {
        GLOBAL_TAG_USAGE_PAGE,
        GLOBAL_TAG_LOGICAL_MINIMUM,
        GLOBAL_TAG_LOGICAL_MAXIMUM,
        GLOBAL_TAG_PHYSICAL_MINIMUM,
        GLOBAL_TAG_PHYSICAL_MAXIMUM,
        GLOBAL_TAG_UNIT_EXPONENT,
        GLOBAL_TAG_UNIT,
        GLOBAL_TAG_REPORT_SIZE,
        GLOBAL_TAG_REPORT_ID,
        GLOBAL_TAG_REPORT_COUNT,
        GLOBAL_TAG_PUSH,
        GLOBAL_TAG_POP,
        GLOBAL_TAG_UNKNOWN,
    };

    enum ShortItemLocalTag
    {
        LOCAL_TAG_USAGE,
        LOCAL_TAG_USAGE_MINIMUM,
        LOCAL_TAG_USAGE_MAXIMUM,
        LOCAL_TAG_DESIGNATOR_INDEX,
        LOCAL_TAG_DESIGNATOR_MINIMUM,
        LOCAL_TAG_DESIGNATOR_MAXIMUM,
        LOCAL_TAG_INVALID, /**< Specification has
         a hole here */
        LOCAL_TAG_STRING_INDEX,
        LOCAL_TAG_STRING_MINIMUM,
        LOCAL_TAG_STRING_MAXIMUM,
        LOCAL_TAG_DELIMITER
    };

    class ShortItem: public Item
    {
        uint8_t mType;
        uint8_t mTag;
    public:
        ShortItem(uint8_t * buf, uint16_t length);
        virtual ~ShortItem();
        virtual bool isLongItem() const
        {
            return false;
        }
        ;
        virtual uint8_t* getData();

        int getTag() const;
        ShortItemType getType() const;

        virtual std::string toString();
    };

    /**
     * TODO Our HID don't include any LongItem, so this class is
     * not complete yet.
     */
    class LongItem: public Item
    {
    public:
        LongItem(uint8_t * buf, uint16_t length);
        virtual ~LongItem();
        virtual bool isLongItem() const
        {
            return true;
        }
        ;
        virtual uint8_t* getData();

        virtual std::string toString();
    };

    class ReportDescriptor
    {
    private:
        void parser(uint8_t * buf, int length);
    public:
        std::vector<Item*> mItems;

        ReportDescriptor(hidraw_report_descriptor& rptDescRaw);
        ~ReportDescriptor();
        void clear();
        std::string toString();
    };

}
#endif //__HID__
