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
#include "pjp255fwupdater.h"
#include "pjp255flashctrlr.h"
#include "pjp255regaccr.h"

using namespace std;
using namespace pixart;

#define TITLE       "Pixart Touchpad Utility"
#define VERSION_STR "v0.1.4"

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
    Pjp274FwUpdater fwUpdater_274(&devHelper, &regAccr_);

    Plp239RegAccr regAccr(&hiddev);
    Plp239FwUpdater fwUpdater_239(&devHelper, &regAccr);
    //add by shawn
    pjp255RegAccr _regAccr(&hiddev);
    pjp255FwUpdater fwUpdater_255(&devHelper, &_regAccr);

    int IC_type = fwUpdater_274.getICType();
    printf("IC Type: %04x\n",IC_type);

    if (argc > 2)
    {
        for (int i = 2; i < argc; ++i)
        {
            string param = argv[i];
            if (param == "reset_hw")
            {
                printf("=== Reset to HW Test Mode + ===\n");
                if (IC_type == 0x239)
                    res = fwUpdater_239.reset(Plp239FwUpdater::ResetType::HwTestMode);
                else if (IC_type == 0x274)
                    res = fwUpdater_274.reset(Pjp274FwUpdater::ResetType::HwTestMode);
                else if (IC_type == 0x255)
                    res = fwUpdater_255.reset(pjp255FwUpdater::ResetType::HwTestMode);
                else
                    printf("=== Not support %04x ===\n",IC_type);

                break;
            }
            else if (param == "reset_re")
            {
                printf("=== Reset to Regular Mode + ===\n");
		        if (IC_type == 0x239)
                    res = fwUpdater_239.reset(Plp239FwUpdater::ResetType::Regular);
		        else if (IC_type == 0x274)
		            res = fwUpdater_274.reset(Pjp274FwUpdater::ResetType::Regular);
                else if (IC_type == 0x255)
                    res = fwUpdater_255.reset(pjp255FwUpdater::ResetType::Regular);
                else
                    printf("=== Not support %04x ===\n",IC_type);

                break;
            }
            else if (param == "show_rd")
            {
                shared_ptr<ReportDescriptor> rptDesc =
                        hiddev.getReportDescriptor();
                printf("%s", rptDesc->toString().c_str());
                break;
            }
            else if (param == "get_pid")
            {
                int pid = 0;

                if (IC_type == 0x239)
                    pid = fwUpdater_239.getPid();
                else if (IC_type == 0x274)
                    pid = fwUpdater_274.getPid();
                else if (IC_type == 0x255)
                    pid = fwUpdater_255.getPid();
                else
                {
                    printf("=== Not support %04x ===\n",IC_type);
                    break;
                }

                printf("PID: %04x\n", pid);
                break;
            }
            else if (param == "get_fwver")
            {
                int fwVer = 0;
                if (IC_type == 0x239)
                    fwVer = fwUpdater_239.getFwVersion();
                else if (IC_type == 0x274)
                    fwVer = fwUpdater_274.getFwVersion();
                else if (IC_type == 0x255)
                    fwVer = fwUpdater_255.getFwVersion();
                else
                {
                    printf("=== Not support %04x ===\n",IC_type);
                    break;
                }

                printf("Firmware Version: %04x\n", fwVer);
                break;
            }
	        else if (param == "get_frame")
            {
		        //int fwVer;
                if (IC_type == 0x239)
		            printf("=== %04x doesn't support this command===\n",IC_type);
		        else if (IC_type == 0x274)
		            fwUpdater_274.ReadFrameData();
                else if (IC_type == 0x255)
                    fwUpdater_255.ReadFrameData();
                else
                    printf("=== Not support %04x ===\n",IC_type);
			
                printf("Get frame data\n");
                break;
            }
	        else if (param == "read_sys_bank")
	        {
		        int bank = atoi(argv[++i]);
		        int addr = atoi(argv[++i]);
		        int value;
		        if (IC_type == 0x239)
                {
		            value = fwUpdater_239.getReadSysRegister(bank,addr);
                    printf("sys bank= %02x, addr= %02x, vaule %04x\n",bank,addr,value);
                }
		        else if (IC_type == 0x274)
                {
		            value = fwUpdater_274.getReadSysRegister(bank,addr);
                    printf("sys bank= %02x, addr= %02x, vaule %04x\n",bank,addr,value);
                }
                else if (IC_type == 0x255)
                {
                    value = fwUpdater_255.getReadSysRegister(bank,addr);
                    printf("sys bank= %02x, addr= %02x, vaule %04x\n",bank,addr,value);
                }
                else
                    printf("=== Not support %04x ===\n",IC_type);

	        }
	        else if (param == "read_user_bank")
	        {
		        int bank = atoi(argv[++i]);
		        int addr = atoi(argv[++i]);
		        int value;
		        if (IC_type == 0x239)
                {
		            value = fwUpdater_239.getReadUserRegister(bank,addr);
                    printf("user bank= %02x, addr= %02x, vaule %02x\n",bank,addr,value);
                }
		        else if (IC_type == 0x274)
                {
		            value = fwUpdater_274.getReadUserRegister(bank,addr);
                    printf("user bank= %02x, addr= %02x, vaule %02x\n",bank,addr,value);
                }
                else if (IC_type == 0x255)
                {
                    value = fwUpdater_255.getReadUserRegister(bank,addr);
                    printf("user bank= %02x, addr= %02x, vaule %02x\n",bank,addr,value);
                }
                else
                    printf("=== Not support %04x ===\n",IC_type);
	        }
 	        else if (param == "write_sys_bank")
	        {
		        int bank = atoi(argv[++i]);
		        int addr = atoi(argv[++i]);
		        int value = atoi(argv[++i]);;
		        if (IC_type == 0x239)
                    printf("=== %04x doesn't support this command===\n",IC_type);
		        else if (IC_type == 0x274)
		            fwUpdater_274.writeRegister(bank,addr,value);
                else if (IC_type == 0x255)
                   fwUpdater_255.writeRegister(bank,addr,value);
                else
                    printf("=== Not support %04x ===\n",IC_type);

		        printf("write sys bank= %02x, addr= %02x, vaule %04x\n",bank,addr,value);
	        }
	        else if (param == "write_user_bank")
	        {
		        int bank = atoi(argv[++i]);
		        int addr = atoi(argv[++i]);
		        int value = atoi(argv[++i]);;
		        if (IC_type == 0x239)
                    printf("=== %04x doesn't support this command===\n",IC_type);
		        else if (IC_type == 0x274)
		            fwUpdater_274.writeUserRegister(bank,addr,value);			
                else if (IC_type == 0x255)
                    fwUpdater_255.writeUserRegister(bank,addr,value);
                else
                    printf("=== Not support %04x ===\n",IC_type);

		        printf("write user bank= %02x, addr= %02x, vaule %02x\n",bank,addr,value);				
            }
            else if (param == "read_sys_bank_batch")
            {
                int bank = atoi(argv[++i]);
                int len = atoi(argv[++i]);
                bool AutoRead = atoi(argv[++i]);
                if (IC_type == 0x239)
                    printf("=== %04x doesn't support this command===\n",IC_type);
                else if (IC_type == 0x274)
                    fwUpdater_274.ReadBatchSysRegister(bank,len,AutoRead);
                else if (IC_type == 0x255)
                    fwUpdater_255.ReadBatchSysRegister(bank,len,AutoRead);
                else
                    printf("=== Not support %04x ===\n",IC_type);
            }
            else if (param == "read_user_bank_batch")
            {
                int bank = atoi(argv[++i]);
                int len = atoi(argv[++i]);
                bool AutoRead = atoi(argv[++i]);
                if (IC_type == 0x239)
                    printf("=== %04x doesn't support this command===\n",IC_type);
                else if (IC_type == 0x274)
                    fwUpdater_274.ReadBatchUserRegister(bank,len,AutoRead);			
                else if (IC_type == 0x255)
                    fwUpdater_255.ReadBatchUserRegister(bank,len,AutoRead);
                else
                    printf("=== Not support %04x ===\n",IC_type);			
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

                if (IC_type == 0x239)
                {
                    res = fwUpdater_239.loadFwBin(fwPath.c_str());
                    printf("Read firmware file, res = %d\n", res);
                    fwUpdater_239.writeFirmware();
                    fwUpdater_239.releaseFwBin();
                    res = fwUpdater_239.reset(Plp239FwUpdater::ResetType::Regular);
                    if (!res) printf("\tReset failed.\n");
                }
                else if (IC_type == 0x274)
                {
                    res = fwUpdater_274.loadFwBin(fwPath.c_str());
                    printf("Read firmware file, res = %d\n", res);
                    fwUpdater_274.writeFirmware();
                    fwUpdater_274.releaseFwBin();	
                    res = fwUpdater_274.reset(Pjp274FwUpdater::ResetType::Regular);
                    if (!res) printf("\tReset failed.\n");					
                }
                else if (IC_type == 0x255)
                {
                    res = fwUpdater_255.loadFwBin(fwPath.c_str());
                    printf("Read firmware file, res = %d\n", res);
                    fwUpdater_255.writeFirmware();
                    //fwUpdater_255.get_calchecksum(false);
                    fwUpdater_255.releaseFwBin();		
                    res = fwUpdater_255.reset(pjp255FwUpdater::ResetType::Regular);	
                    if (!res) printf("\tReset failed.\n");
                    //fwUpdater_255.GetChipCodeCCRC(false);
                }
                else
                {
                    printf("=== Not support %04x ===\n",IC_type);
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
            
                if (IC_type == 0x239)
                {
                    res = fwUpdater_239.loadHidDescFile(path.c_str());
                    printf("Read firmware file, res = %d\n", res);
                    fwUpdater_239.writeHidDesc();
                    fwUpdater_239.releaseHidDescBin();
                }
                else
                {
                    printf("=== Not support %04x ===\n",IC_type);
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


                if (IC_type == 0x239)
                {
                    bool res = fwUpdater_239.loadParameterFile(path.c_str());
                    printf("Read parameter file, res = %d\n", res);
                    fwUpdater_239.writeParameter();
                    fwUpdater_239.releaseParameterBin();
                }
                else
                {
                    printf("=== Not support %04x ===\n",IC_type);
                }
                /*
                else if (IC_type==0x274)
                {
                    bool res = fwUpdater_274.loadParameterFile(path.c_str());
                    printf("Read parameter file, res = %d\n", res);
                    fwUpdater_274.writeParameter();
                    fwUpdater_274.releaseParameterBin();
                }
                else 
                { //IC_type255 
                    bool res = fwUpdater_255.loadParameterFile(path.c_str());
                    printf("Read parameter file, res = %d\n", res);
                    fwUpdater_255.writeParameter();
                    fwUpdater_255.releaseParameterBin();
                }
                */
                break;
            }
            else if (param == "get_crc")
            {
                if ((i + 1) >= argc)
                {
                    printf("Please provide which crc parameter you want.\n");
                    res = 0;
                    break;
                }    
                string option = argv[++i]; 
                pjp255FlashCtrlr::CRCType type=pjp255FlashCtrlr::CRCType::FW_CRC;

                if  (option == "1")
                    type=pjp255FlashCtrlr::CRCType::PAR_CRC;
                else if (option == "2")
                    type=pjp255FlashCtrlr::CRCType::DEF_FW_CRC;
                else if (option == "3")
                    type=pjp255FlashCtrlr::CRCType::DEF_PAR_CRC;

                if (IC_type == 0x255) fwUpdater_255.GetChipCRC(type);
            }    
            else if (param == "up")
            {
                if ((i + 1) >= argc)
                {
                    printf("Please provide firmware file path.\n");
                    res = 0;
                    break;
                }

                printf("=== Start upgrade ===\n");
                string path = argv[i + 1];

                if (IC_type == 0x239)
                    res = fwUpdater_239.loadUpgradeBin(path.c_str());
                else if (IC_type == 0x274)
                    res = fwUpdater_274.loadUpgradeBin(path.c_str());	
                else if (IC_type == 0x255)
                    res = fwUpdater_255.loadUpgradeBin(path.c_str());
                else
                {
                    printf("=== Not support %04x ===\n",IC_type);	
                    break;
                }
                

                printf("Read upgrade file, res = %d\n", res);
                if (!res)
                {
                    printf("\tFailed to read upgrade file.\n");
                    break;
                }

                printf("=== Reset to hardware mode ===\n");

                if (IC_type == 0x239)
                    res = fwUpdater_239.reset(Plp239FwUpdater::ResetType::HwTestMode);
                else if (IC_type == 0x274)
                    res = fwUpdater_274.reset(Pjp274FwUpdater::ResetType::HwTestMode);
                else if (IC_type == 0x255)
                    res = fwUpdater_255.reset(pjp255FwUpdater::ResetType::HwTestMode);
                else
                {
                    printf("=== Not support %04x ===\n",IC_type);
                    break;
                }


                if (!res)
                {
                    printf("\tReset failed.\n");
                    break;
                }
                
            
                if (IC_type == 0x239)
                    res = fwUpdater_239.fullyUpgrade();
                else if (IC_type == 0x274)
                {
                    res = fwUpdater_274.fullyUpgrade();	
                    fwUpdater_274.releaseFwBin();
                    fwUpdater_274.releaseParameterBin();
                }
                else if (IC_type == 0x255)
                {
                    res = fwUpdater_255.fullyUpgrade();
                    fwUpdater_255.releaseFwBin();
                    fwUpdater_255.releaseParameterBin();
                }
                else
                {
                    printf("=== Not support %04x ===\n",IC_type);
                    break;
                }

                if (!res)
                {
                    printf("\tFailed to upgrade.\n");
                    // Do not break here, keep running until device reset
                }
                printf("=== Reset to Regular Mode ===\n");

                if (IC_type == 0x239)
                    res = fwUpdater_239.reset(Plp239FwUpdater::ResetType::Regular);
                else if (IC_type == 0x274)
                    res = fwUpdater_274.reset(Pjp274FwUpdater::ResetType::Regular);	
                else if (IC_type == 0x255)
                    res = fwUpdater_255.reset(pjp255FwUpdater::ResetType::Regular);
                else
                {
                    printf("=== Not support %04x ===\n",IC_type);
                    break;
                }

                if (!res)
                {
                    printf("\tReset failed.\n");
                    break;
                }

                if (IC_type == 0x255)
                {
                    printf("=== The related information reports after bin file updated===\n");
                    fwUpdater_255.GetChipCRC(pjp255FlashCtrlr::CRCType::FW_CRC);    
                    fwUpdater_255.GetChipCRC(pjp255FlashCtrlr::CRCType::PAR_CRC); 
                    fwUpdater_255.GetChipCRC(pjp255FlashCtrlr::CRCType::DEF_FW_CRC); 
                    fwUpdater_255.GetChipCRC(pjp255FlashCtrlr::CRCType::DEF_PAR_CRC); 
                    int fwVer = fwUpdater_255.getFwVersion();
                    printf("Firmware Version: %04x\n",fwVer);
                }
            }
        }
    }
    // release
    hiddev.close();
    return 1;
}

