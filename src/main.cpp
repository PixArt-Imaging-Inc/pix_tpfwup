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
#define VERSION      000003
#define VERSION_STR "v0.0.3"

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
    //add by shawn
    pjp255RegAccr _regAccr(&hiddev);
    pjp255FwUpdater _fwUpdater(&devHelper, &_regAccr);

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
		        else if (IC_type==0x274)
		            res = fwUpdater_.reset(Pjp274FwUpdater::ResetType::HwTestMode);
                else 
                    res = _fwUpdater.reset(pjp255FwUpdater::ResetType::HwTestMode);

                break;
            }
            else if (param == "reset_re")
            {
                printf("=== Reset to Regular Mode + ===\n");
		        if (IC_type==0x239)
                    res = fwUpdater.reset(Plp239FwUpdater::ResetType::Regular);
		        else if (IC_type==0x274)
		            res = fwUpdater_.reset(Pjp274FwUpdater::ResetType::Regular);
                else
                    res = _fwUpdater.reset(pjp255FwUpdater::ResetType::Regular);

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
		        else if (IC_type==0x274)
		            fwVer = fwUpdater_.getFwVersion();
                else 
                    fwVer = _fwUpdater.getFwVersion();
			
                printf("Firmware Version: %04x\n", fwVer);
                break;
            }
	        else if (param == "get_frame")
            {
		        //int fwVer;
                if (IC_type==0x239)
		            ;//fwVer = fwUpdater.getFwVersion();
		        else if (IC_type==0x274)
		            fwUpdater_.ReadFrameData();
                else 
                    _fwUpdater.ReadFrameData();
			
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
		        else if (IC_type==0x274)
		            value = fwUpdater_.getReadSysRegister(bank,addr);
                else
                    value = _fwUpdater.getReadSysRegister(bank,addr);

		        printf("sys bank= %02x, addr= %02x, vaule %04x\n",bank,addr,value);
	        }
	        else if (param == "read_user_bank")
	        {
		        int bank =  atoi(argv[++i]);
		        int addr =  atoi(argv[++i]);
		        int value;
		        if (IC_type==0x239)
		            value = fwUpdater.getReadUserRegister(bank,addr);
		        else if (IC_type==0x274)
		            value = fwUpdater_.getReadUserRegister(bank,addr);			
                else
                    value = _fwUpdater.getReadUserRegister(bank,addr);		

		        printf("user bank= %02x, addr= %02x, vaule %02x\n",bank,addr,value);				
	        }
 	        else if (param == "write_sys_bank")
	        {
		        int bank =  atoi(argv[++i]);
		        int addr =  atoi(argv[++i]);
		        int value =  atoi(argv[++i]);;
		        if (IC_type==0x239)
		            ;//value = fwUpdater.getReadSysRegister(bank,addr);
		        else if (IC_type==0x274)
		            fwUpdater_.writeRegister(bank,addr,value);
                else
                   _fwUpdater.writeRegister(bank,addr,value);

		        printf("write sys bank= %02x, addr= %02x, vaule %04x\n",bank,addr,value);
	        }
	        else if (param == "write_user_bank")
	        {
		        int bank =  atoi(argv[++i]);
		        int addr =  atoi(argv[++i]);
		        int value =  atoi(argv[++i]);;
		        if (IC_type==0x239)
		            ;//value = fwUpdater.getReadUserRegister(bank,addr);
		        else if (IC_type==0x274)
		            fwUpdater_.writeUserRegister(bank,addr,value);			
                else
                    _fwUpdater.writeUserRegister(bank,addr,value);	

		        printf("write user bank= %02x, addr= %02x, vaule %02x\n",bank,addr,value);				
	      }
 	     else if (param == "read_sys_bank_batch")
	     {
		    int bank =  atoi(argv[++i]);
		    int len =  atoi(argv[++i]);
		    bool AutoRead =  atoi(argv[++i]);
		    if (IC_type==0x239)
		        ;//value = fwUpdater.getReadSysRegister(bank,addr);
		    else if (IC_type==0x274)
		        fwUpdater_.ReadBatchSysRegister(bank,len,AutoRead);
            else 
                _fwUpdater.ReadBatchSysRegister(bank,len,AutoRead);

		    //printf("sys bank= %02x, addr= %02x, vaule %04x\n",bank,addr,value);
	     }
	     else if (param == "read_user_bank_batch")
	     {
		    int bank =  atoi(argv[++i]);
		    int len =  atoi(argv[++i]);
		    bool AutoRead =  atoi(argv[++i]);
		    if (IC_type==0x239)
		        ;//value = fwUpdater.getReadUserRegister(bank,addr);
		    else if (IC_type==0x274)
		        fwUpdater_.ReadBatchUserRegister(bank,len,AutoRead);			
            else 
                _fwUpdater.ReadBatchUserRegister(bank,len,AutoRead);

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
                res = fwUpdater.reset(Plp239FwUpdater::ResetType::Regular);
                if (!res) printf("\tReset failed.\n");
		    }
		    else if (IC_type==0x274)
		    {
		        res = fwUpdater_.loadFwBin(fwPath.c_str());
		        printf("Read firmware file, res = %d\n", res);
		        fwUpdater_.writeFirmware();
		        fwUpdater_.releaseFwBin();	
                res = fwUpdater_.reset(Pjp274FwUpdater::ResetType::Regular);
                if (!res) printf("\tReset failed.\n");					
		    }
            else 
            {
                res = _fwUpdater.loadFwBin(fwPath.c_str());
		        printf("Read firmware file, res = %d\n", res);
		        _fwUpdater.writeFirmware();
                //_fwUpdater.get_calchecksum(false);
		        _fwUpdater.releaseFwBin();		
                res = _fwUpdater.reset(pjp255FwUpdater::ResetType::Regular);	
                if (!res) printf("\tReset failed.\n");
                //_fwUpdater.GetChipCodeCCRC(false);
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
            /*
            else if (IC_type==0x274)
            {
                bool res = fwUpdater_.loadParameterFile(path.c_str());
			    printf("Read parameter file, res = %d\n", res);
                fwUpdater_.writeParameter();
			    fwUpdater_.releaseParameterBin();
            }
            else 
            { //IC_type255 
                bool res = _fwUpdater.loadParameterFile(path.c_str());
			    printf("Read parameter file, res = %d\n", res);
                _fwUpdater.writeParameter();
			    _fwUpdater.releaseParameterBin();
            }
            */
            break;
        }
        else if (param=="get_crc")
        {
            if ((i + 1) >= argc)
            {
                printf("Please provide which crc parameter you want.\n");
                res = 0;
                break;
            }    
            string option = argv[++i]; 
            pjp255FlashCtrlr::CRCType type=pjp255FlashCtrlr::CRCType::FW_CRC;

            if  (option=="1")
                type=pjp255FlashCtrlr::CRCType::PAR_CRC;
            else if (option=="2")
                type=pjp255FlashCtrlr::CRCType::DEF_FW_CRC;
            else if (option=="3")
                 type=pjp255FlashCtrlr::CRCType::DEF_PAR_CRC;

            if (IC_type==0x255) _fwUpdater.GetChipCRC(type);
        }    
        else if (param == "up")
        {
            if ((i + 1) >= argc)
            {
                printf("Please provide firmware file path.\n");
                res = 0;
                break;
            }

            /*Compal patch*/
            string fwPath = argv[++i];
            if (IC_type==0x274)
		    {
		        res = fwUpdater_.loadFwBin(fwPath.c_str());
		        printf("Read firmware file, res = %d\n", res);
		        fwUpdater_.writeFirmware();
		        fwUpdater_.releaseFwBin();	
                res = fwUpdater_.reset(Pjp274FwUpdater::ResetType::Regular);
                if (!res) printf("\tReset failed.\n");
                break;				
		    }
            /*Compal patch*/

            printf("=== Reset to HW Mode ===\n");

		    if (IC_type==0x239)
                res = fwUpdater.reset(Plp239FwUpdater::ResetType::HwTestMode);
		    else if (IC_type==0x274)
		        res = fwUpdater_.reset(Pjp274FwUpdater::ResetType::HwTestMode);
            else 
                res = _fwUpdater.reset(pjp255FwUpdater::ResetType::HwTestMode);


            if (!res)
            {
                printf("\tReset failed.\n");
                break;
            }
            printf("=== Start upgrade ===\n");
            string path = argv[i + 1];

		    if (IC_type==0x239)
                res = fwUpdater.loadUpgradeBin(path.c_str());
		    else if (IC_type==0x274)
		        res = fwUpdater_.loadUpgradeBin(path.c_str());	
            else 
                res = _fwUpdater.loadUpgradeBin(path.c_str());	
             

            printf("Read upgrade file, res = %d\n", res);
            if (!res)
            {
                printf("\tFailed to read upgrade file.\n");
                break;
            }
        
		    if (IC_type==0x239)
                res = fwUpdater.fullyUpgrade();
		    else if (IC_type==0x274)
		        res = fwUpdater_.fullyUpgrade();	
            else 
            {
                res = _fwUpdater.fullyUpgrade();
                _fwUpdater.releaseFwBin();
                _fwUpdater.releaseParameterBin();
            }
		    if (!res)
            {
                printf("\tFailed to upgrade.\n");
                break;
            }
            printf("=== Reset to Regular Mode ===\n");

		    if (IC_type==0x239)
                res = fwUpdater.reset(Plp239FwUpdater::ResetType::Regular);
		    else if (IC_type==0x274)		                        
                res = fwUpdater_.reset(Pjp274FwUpdater::ResetType::Regular);	
            else
                res = _fwUpdater.reset(pjp255FwUpdater::ResetType::Regular);

            if (!res)
            {
                    printf("\tReset failed.\n");
                    break;
            }

            if (IC_type==0x255)
            {
                printf("=== The related information reports after bin file updated===\n");
                _fwUpdater.GetChipCRC(pjp255FlashCtrlr::CRCType::FW_CRC);    
                _fwUpdater.GetChipCRC(pjp255FlashCtrlr::CRCType::PAR_CRC); 
                _fwUpdater.GetChipCRC(pjp255FlashCtrlr::CRCType::DEF_FW_CRC); 
                _fwUpdater.GetChipCRC(pjp255FlashCtrlr::CRCType::DEF_PAR_CRC); 
                byte ret_data = _fwUpdater.getHidFwVersion();
                printf("The firmware version is %d\n",ret_data);
                ret_data = _fwUpdater.getHidParversion();
                printf("The parameter version is %d\n",ret_data);
            }
        }
        }
    }
    // release
    hiddev.close();
    return 1;
}

