#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>

#include "hid.h"
#include "hiddevhelper.h"
#include "plp239regaccr.h"
#include "plp239flashctrlr.h"
#include "plp239fwupdater.h"
#include "pjp274regaccr.h"
#include "pjp274flashctrlr.h"
#include "pjp274fwupdater.h"

using namespace std;
using namespace pixart;

#define TITLE       "Pixart Touchpad Utility"
#define VERSION      000001
#define VERSION_STR "v0.0.1"

int main(int argc, char **argv)
{
    printf("%s (%s)\n", TITLE, VERSION_STR);
    if (argc < 2)
    {
        printf("Please input HIDRAW path.\n");
        return 0;
    }
    char * path = argv[1];
    int res = 1;
    HidDevice hiddev;
    res = hiddev.open(path);
    if (!res)
    {
        printf("Failed to open hidraw interface.\n");
        return 0;
    }

    HidDevHelper devHelper(&hiddev);
	
    Pjp274RegAccr regAccr_(&hiddev);
    Pjp274FwUpdater fwUpdater_(&devHelper, &regAccr_);
    int IC_type = fwUpdater_.getICType();
    printf("IC Type: %04x\n",IC_type);


    Plp239RegAccr regAccr(&hiddev);
    Plp239FwUpdater fwUpdater(&devHelper, &regAccr);

    if (argc > 2)
    {
        for (int i = 2; i < argc; ++i)
        {
            string param = argv[i];
            if (param == "reset_hw")
            {
                printf("=== Reset to HW Test Mode + ===\n");
		if (IC_type==0x239)
                res = fwUpdater.reset(Plp239FwUpdater::ResetType::HwTestMode);
		else
		res = fwUpdater_.reset(Pjp274FwUpdater::ResetType::HwTestMode);
                break;
            }
            else if (param == "reset_re")
            {
                printf("=== Reset to Regular Mode + ===\n");
		if (IC_type==0x239)
                res = fwUpdater.reset(Plp239FwUpdater::ResetType::Regular);
		else
		res = fwUpdater_.reset(Pjp274FwUpdater::ResetType::Regular);
                break;
            }
            else if (param == "show_rd")
            {
                shared_ptr<ReportDescriptor> rptDesc =
                        hiddev.getReportDescriptor();
                printf("%s", rptDesc->toString().c_str());
                break;
            }
            else if (param == "get_fwver")
            {
		int fwVer;
                if (IC_type==0x239)
		fwVer = fwUpdater.getFwVersion();
		else
		fwVer = fwUpdater_.getFwVersion();
			
                printf("Firmware Version: %04x\n", fwVer);
                break;
            }
	    else if (param == "get_frame")
            {
		int fwVer;
                if (IC_type==0x239)
		;//fwVer = fwUpdater.getFwVersion();
		else
		fwUpdater_.ReadFrameData();
			
                printf("Get frame data\n");
                break;
            }
	    else if (param == "read_sys_bank")
	    {
		int bank =  atoi(argv[++i]);
		int addr =  atoi(argv[++i]);
		int value;
		if (IC_type==0x239)
		value = fwUpdater.getReadSysRegister(bank,addr);
		else
		value = fwUpdater_.getReadSysRegister(bank,addr);
		printf("sys bank= %02x, addr= %02x, vaule %04x\n",bank,addr,value);
	     }
	     else if (param == "read_user_bank")
	     {
		int bank =  atoi(argv[++i]);
		int addr =  atoi(argv[++i]);
		int value;
		if (IC_type==0x239)
		value = fwUpdater.getReadUserRegister(bank,addr);
		else
		value = fwUpdater_.getReadUserRegister(bank,addr);			
		printf("user bank= %02x, addr= %02x, vaule %02x\n",bank,addr,value);				
	      }
 	     else if (param == "write_sys_bank")
	    {
		int bank =  atoi(argv[++i]);
		int addr =  atoi(argv[++i]);
		int value =  atoi(argv[++i]);;
		if (IC_type==0x239)
		;//value = fwUpdater.getReadSysRegister(bank,addr);
		else
		fwUpdater_.writeRegister(bank,addr,value);
		printf("write sys bank= %02x, addr= %02x, vaule %04x\n",bank,addr,value);
	     }
	     else if (param == "write_user_bank")
	     {
		int bank =  atoi(argv[++i]);
		int addr =  atoi(argv[++i]);
		int value =  atoi(argv[++i]);;
		if (IC_type==0x239)
		;//value = fwUpdater.getReadUserRegister(bank,addr);
		else
		fwUpdater_.writeUserRegister(bank,addr,value);			
		printf("write user bank= %02x, addr= %02x, vaule %02x\n",bank,addr,value);				
	      }
 	    else if (param == "read_sys_bank_batch")
	    {
		int bank =  atoi(argv[++i]);
		int len =  atoi(argv[++i]);
		bool AutoRead =  atoi(argv[++i]);
		if (IC_type==0x239)
		;//value = fwUpdater.getReadSysRegister(bank,addr);
		else
		fwUpdater_.ReadBatchSysRegister(bank,len,AutoRead);
		//printf("sys bank= %02x, addr= %02x, vaule %04x\n",bank,addr,value);
	     }
	     else if (param == "read_user_bank_batch")
	     {
		int bank =  atoi(argv[++i]);
		int len =  atoi(argv[++i]);
		bool AutoRead =  atoi(argv[++i]);
		if (IC_type==0x239)
		;//value = fwUpdater.getReadUserRegister(bank,addr);
		else
		fwUpdater_.ReadBatchUserRegister(bank,len,AutoRead);			
		//printf("user bank= %02x, addr= %02x, vaule %02x\n",bank,addr,value);				
	      }
            else if (param == "update_fw")
            {
                if ((i + 1) >= argc)
                {
                    printf("Please provide firmware path.\n");
                    res = 0;
                    break;
                }
                string fwPath = argv[++i];
		if (IC_type==0x239)
		{
		     res = fwUpdater.loadFwBin(fwPath.c_str());
		     printf("Read firmware file, res = %d\n", res);
		     fwUpdater.writeFirmware();
		     fwUpdater.releaseFwBin();
		}
		else
		{
		      res = fwUpdater_.loadFwBin(fwPath.c_str());
		      printf("Read firmware file, res = %d\n", res);
		      fwUpdater_.writeFirmware();
		      fwUpdater_.releaseFwBin();
					
		}
                break;
            }

            else if (param == "update_hid")
            {
                if ((i + 1) >= argc)
                {
                    printf("Please provide hid descriptor file path.\n");
                    res = 0;
                    break;
                }
                string path = argv[++i];
		if (IC_type==0x239)
		{
			res = fwUpdater.loadHidDescFile(path.c_str());
			printf("Read firmware file, res = %d\n", res);
			fwUpdater.writeHidDesc();
			fwUpdater.releaseHidDescBin();
		}
				
                break;
            }
            else if (param == "update_param")
            {
                if ((i + 1) >= argc)
                {
                    printf("Please provide parameter file path.\n");
                    res = 0;
                    break;
                }
                string path = argv[++i];
		if (IC_type==0x239)
		{
			bool res = fwUpdater.loadParameterFile(path.c_str());
			printf("Read parameter file, res = %d\n", res);
			fwUpdater.writeParameter();
			fwUpdater.releaseParameterBin();
		}			
                break;
            }
            else if (param == "up")
            {
                if ((i + 1) >= argc)
                {
                    printf("Please provide firmware file path.\n");
                    res = 0;
                    break;
                }

                printf("=== Reset to HW Mode ===\n");
		if (IC_type==0x239)
                res = fwUpdater.reset(Plp239FwUpdater::ResetType::HwTestMode);
		else
		res = fwUpdater_.reset(Pjp274FwUpdater::ResetType::HwTestMode);
			
                if (!res)
                {
                    printf("\tReset failed.\n");
                    break;
                }
                printf("=== Start upgrade ===\n");
                string path = argv[i + 1];

		if (IC_type==0x239)
                res = fwUpdater.loadUpgradeBin(path.c_str());
		else
		res = fwUpdater_.loadUpgradeBin(path.c_str());	

                printf("Read upgrade file, res = %d\n", res);
                if (!res)
                {
                    printf("\tFailed to read upgrade file.\n");
                    break;
                }

		if (IC_type==0x239)
                res = fwUpdater.fullyUpgrade();
		else
		res = fwUpdater_.fullyUpgrade();	
                
		if (!res)
                {
                    printf("\tFailed to upgrade.\n");
                    break;
                }
                printf("=== Reset to Regular Mode ===\n");

		if (IC_type==0x239)
                res = fwUpdater.reset(Plp239FwUpdater::ResetType::Regular);
		else
		res = fwUpdater_.reset(Pjp274FwUpdater::ResetType::Regular);	
                if (!res)
                {
                    printf("\tReset failed.\n");
                    break;
                }
            }
        }
    }
    // release
    hiddev.close();
    return 1;
}

