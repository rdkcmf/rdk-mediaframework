/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2011 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

/**
 * @file This file contains the functions whose work is to get the info about
 * AV interface, Tuner, DVI/HDMI, video format etc and copy them in their respective
 * Info table.
 */


#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

/* pfc support file */
#include <unistd.h>
#include <list>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>

#include "cmhash.h"
#include "rmf_error.h"
#include "core_events.h"
 #include "snmpmanager.h"
 #include "bits_cdl.h"
#include <pthread.h>
/***Start : Snmp related headers**********/
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <signal.h>
#include<string.h>
#include "vl_ocStbHost_GetData.h"
#include "AvInterfaceGetdata.h"
#include "snmpManagerIf.h"
#include "tablevividlogicmib.h"
#include "vlEnv.h"
//TO get the IP and MAC adress
#include "QAMSrcDiag.h"
#include "ipcclient_impl.h"
#include <sys/types.h>
#include <sys/socket.h>

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

#define MAX_CERTIFICATION_INFO 30

#define MAX_ROWS 15

//#include "ocStbHostMibModule.h"

//#include "ocStbHostMibModuleConfig.h" /* #include "ocStbHostMibModuleConfig.h" for scalar data later it has to change */

extern "C" unsigned char vl_isFeatureEnabled(unsigned char *featureString);
static char vlg_szMpeosFeature1394[]        = {"FEATURE.1394.SUPPORT"};
/***************************************** TABLES ******************************************************/

#define vlStrCpy(pDest, pSrc, nDestCapacity)            \
            strcpy(pDest, pSrc)

#define vlMemCpy(pDest, pSrc, nCount, nDestCapacity)            \
            memcpy(pDest, pSrc, nCount)

#ifdef __cplusplus
extern "C"
{
    rmf_Error vlMpeosSetOCAPApplicationInfoTable(int nApps, unsigned char* data,int len)
    {
          //printf("Inside %s \n",__FUNCTION__);
          unsigned char *s = (unsigned char *)data;
          SimpleBitstream b((unsigned char*)data, len);
          
          VLMPEOS_SNMPHOSTSoftwareApplicationInfoTable_t swapplication[nApps];
          
          for (int i=0; i<nApps;i++)
          {
          swapplication[i].vl_ocStbHostSoftwareApplicationInfoIndex = b.get_bits(32);
          //printf("App Index=%ld\n", swapplication[i].vl_ocStbHostSoftwareApplicationInfoIndex);
          
          swapplication[i].vl_ocStbHostSoftwareOrganizationId = b.get_bits(32);
          //printf("ORGID=%d\n", swapplication[i].vl_ocStbHostSoftwareOrganizationId);

          swapplication[i].vl_ocStbHostSoftwareApplicationId = b.get_bits(32);
          //printf("AppID=%d\n", swapplication[i].vl_ocStbHostSoftwareApplicationId);
          
          int appNameLen = b.get_bits(32);
          //printf("Application Name length=%d\n", appNameLen);
          
          if(appNameLen > 0)
          {
            char* name = NULL;
			rmf_osal_memAllocP(RMF_OSAL_MEM_POD, appNameLen+1, (void **)&name);
			memset(name,0,appNameLen+1);
			
            
            if(name)
            {
              vlStrCpy(name, (char*)&s[b.get_pos()/8],appNameLen+1);
              vlStrCpy(swapplication[i].vl_ocStbHostSoftwareAppNameString, name, sizeof(swapplication[i].vl_ocStbHostSoftwareAppNameString));
              //printf("AppName=%s\n", swapplication[i].vl_ocStbHostSoftwareAppNameString);
              rmf_osal_memFreeP(RMF_OSAL_MEM_POD,name);
            }
          }
          b.skip_bits(appNameLen*8);
          
          int appVerLen = b.get_bits(32);
          //printf("Application version length=%d\n", appVerLen);
          
          if(appVerLen > 0)
          {
            char* name = NULL;

			rmf_osal_memAllocP(RMF_OSAL_MEM_POD, appVerLen+1, (void **)&name);
			memset(name,0,appVerLen+1);
            
            if(name)
            {
              vlStrCpy(name, (char*)&s[b.get_pos()/8],appVerLen+1);
              vlStrCpy(swapplication[i].vl_ocStbHostSoftwareAppVersionNumber,name, sizeof(swapplication[i].vl_ocStbHostSoftwareAppVersionNumber));
              //printf("App Version=%s\n", swapplication[i].vl_ocStbHostSoftwareAppVersionNumber);
              rmf_osal_memFreeP(RMF_OSAL_MEM_POD,name);
            }
          }
          b.skip_bits(appVerLen*8);
          
          int appStatus = b.get_bits(32);
          swapplication[i].vl_ocStbHostSoftwareStatus = (VLMPEOS_SoftwareAPP_status)appStatus;
          //printf("Application Status=%d\n", swapplication[i].vl_ocStbHostSoftwareStatus);
          int swSigLen = b.get_bits(32);
          //printf("swSigLen length=%d\n", swSigLen);
          if(swSigLen > 0)
          {
            char* name = NULL;

			rmf_osal_memAllocP(RMF_OSAL_MEM_POD, swSigLen+1, (void **)&name);
			memset(name,0,swSigLen+1);
            
            if(name)
            {
              vlStrCpy(name, (char*)&s[b.get_pos()/8],swSigLen+1);
              vlStrCpy(swapplication[i].vl_ocStbHostSoftwareApplicationSigStatus, name, sizeof(swapplication[i].vl_ocStbHostSoftwareApplicationSigStatus));
              //printf("Stattus=%s\n", swapplication[i].vl_ocStbHostSoftwareApplicationSigStatus);
               rmf_osal_memFreeP(RMF_OSAL_MEM_POD,name);
            }
          }
              
          b.skip_bits(swSigLen*8);
          }
          
          if(nApps > 0)
          {
             return vlMpeosSetocStbHostSwApplicationInfoTable(nApps,swapplication);
          }
          
          return RMF_SUCCESS;
    }

    void vlMpeosSetHostUserSettingsPreferedLanguage(char * userSettingsPreferredLanguage)
    {
        //printf("Inside %s and userSettingsPreferredLanguage=%s \n",__FUNCTION__,userSettingsPreferredLanguage);
        vlMpeosSetocStbHostUserSettingsPreferedLanguage(userSettingsPreferredLanguage);
    }
    
    void vlMpeosSetJvmInfo(int totalHeapSize,int totalAvailSize, int jvmLiveObjects,int jvmDeadObjects)
    {
        //printf("Inside %s totalHeapSize =%d, totalAvailSize = %d, jvmLiveObjects = %d, jvmDeadObjects =%d \n",__FUNCTION__,totalHeapSize,totalAvailSize, jvmLiveObjects,jvmDeadObjects);
        VLMPEOS_SNMP_SJVMinfo_t *pJvmInfo = new VLMPEOS_SNMP_SJVMinfo_t();
        pJvmInfo->JVMHeapSize = totalHeapSize;
        pJvmInfo->JVMAvailHeap = totalAvailSize;
        pJvmInfo->JVMLiveObjects = jvmLiveObjects;
        pJvmInfo->JVMDeadObjects = jvmDeadObjects;    
        vlMpeosSetocStbHostJvmInfo(pJvmInfo);    
        delete pJvmInfo;
    }
#endif //__cplusplus
}
/** Global Sturcture for the storing the Intences */
SNMPocStbHostAVInterfaceTable_t vlg_AVInterfaceTable[SNMP_MAX_ROWS];
//memset(vlg_AVInterfaceTable, 0, (SNMP_MAX_ROWS*s sizeof(SNMPocStbHostAVInterfaceTable_t)));

int vlg_number_rows = 0;
Avinterface avobj; // Avinterface calss obj
/**
 * @brief This function is used to get the AV interface information like AV Interface Type,
 * Description, Status etc and update these info to Interface table.
 *
 * @param[out] table_data Table where the interface data is to be retrieved.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int vl_ocStbHostAVInterfaceTable_getdata(netsnmp_tdata * table_data)
{

    bool bAvIntTableUpdatedNow = false;
    memset(vlg_AVInterfaceTable, 0, sizeof(vlg_AVInterfaceTable));
    if (0 == avobj.vl_getHostAVInterfaceTable(vlg_AVInterfaceTable, &vlg_number_rows, bAvIntTableUpdatedNow))
    {

        SNMP_DEBUGPRINT("ERROR:: avobj.vl_getHostAVInterfaceTable");
        return 0;
    }

    if (! bAvIntTableUpdatedNow)
    {
        SNMP_DEBUGPRINT("vl_ocStbHostAVInterfaceTable_getdata : Table not updated now.. Returning without making createEntry calls\n");
        return 1;
    }

     //SNMP_DEBUGPRINT("\n Avinterface :: vl_getHostAVInterfaceTable ::  COLLECTING THE DATA::::::::::::: \n" );
    int ixmrows,ixrow;


    /* copy and replace will be don inside this copy_all_data function */
    ocStbHostAVInterfaceTable_copy_allData(table_data, vlg_AVInterfaceTable, vlg_number_rows);

//                vlg_AVInterfaceTable[ixrow].vl_ocStbHostAVInterfaceType,
//                 vlg_AVInterfaceTable[ixrow].vl_ocStbHostAVInterfaceDesc,
//                 vlg_AVInterfaceTable[ixrow].vl_ocStbHostAVInterfaceStatus
    return 1;//Mamidi:042209
}

/**
 * @brief This function copies AV interface information from passed input parameter
 * to the Interface table.
 *
 * @param[in] vlg_AVInterfaceTable Struct pointer where interface data is present.
 * @param[in] nrows Number of rows to be copied to inteface table.
 * @param[out] table_data Table where interface data is to be stored.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int
        ocStbHostAVInterfaceTable_copy_allData(netsnmp_tdata * table_data, SNMPocStbHostAVInterfaceTable_t* vlg_AVInterfaceTable, int nrows)
{

    //bool replace_table = false;
//    netsnmp_tdata_row *temprow;
    int ixrow ;
    //For second time collecting the data ,exiting data to be free by the below method
//    if(temprow = netsnmp_tdata_row_first(table_data))
    if(netsnmp_tdata_row_first(table_data))
    {
      //  replace_table = true;
        Table_free(table_data);
    }

    for(ixrow = 0 ; ixrow <nrows; ixrow++ )
    {
        struct ocStbHostAVInterfaceTable_entry *entry;
        netsnmp_tdata_row *row;
        entry = SNMP_MALLOC_TYPEDEF(struct ocStbHostAVInterfaceTable_entry);
        if (!entry)
            return 0;

        row = netsnmp_tdata_create_row();
        if (!row) {
            SNMP_FREE(entry);
            return 0;
        }
        row->data = entry;
     /** + 1 is done so that the table index starts with 1.
        //In SNMP, Index 0 is used for scalars only. Tables start with Index 1; */
        entry->ocStbHostAVInterfaceIndex = ixrow + 1;

    //SNMP_DEBUGPRINT("\n Row numbner =%d  \n", ixrow);
    // entry->ocStbHostAVInterfaceType = (oid*)vlMalloc(sizeof(oid)*ocStbHostAVInterfaceDesc_len);
        /** entry->ocStbHostAVInterfaceIndex is the instance index of the device */
    //SNMP_DEBUGPRINT("\nocStbHostAVInterfaceType===== =0x%x \n",vlg_AVInterfaceTable[ixrow].vl_ocStbHostAVInterfaceType);
    //ocStbHostAVInterfaceType = 0x0;
    //entry->ocStbHostAVInterfaceIndex = ixrow; // using ixrow for the table index
        memset(entry->ocStbHostAVInterfaceType,0,MAX_IOD_LENGTH );

        entry->ocStbHostAVInterfaceType_len = (sizeof(avTableId[vlg_AVInterfaceTable[ixrow].vl_ocStbHostAVInterfaceType])/(sizeof(apioid)));
    //  SNMP_DEBUGPRINT("\nocStbHostAVInterfaceType===== 1\n");
        memcpy(entry->ocStbHostAVInterfaceType, avTableId[vlg_AVInterfaceTable[ixrow].vl_ocStbHostAVInterfaceType],
                 entry->ocStbHostAVInterfaceType_len * sizeof(oid));

        entry->ocStbHostAVInterfaceDesc_len = (strlen(vlg_AVInterfaceTable[ixrow].vl_ocStbHostAVInterfaceDesc));     //entry->ocStbHostAVInterfaceDesc_len = strlen(ocStbHostAVInterfaceDesc);
        //SNMP_DEBUGPRINT("\nocStbHostAVInterfaceType===== 3\n");
        memcpy(entry->ocStbHostAVInterfaceDesc, vlg_AVInterfaceTable[ixrow].vl_ocStbHostAVInterfaceDesc,
                 entry->ocStbHostAVInterfaceDesc_len);
         //SNMP_DEBUGPRINT("\nentry->ocStbHostAVInterfaceDesc %s\n",entry->ocStbHostAVInterfaceDesc);

        entry->ocStbHostAVInterfaceStatus =  vlg_AVInterfaceTable[ixrow].vl_ocStbHostAVInterfaceStatus; /*OCSTBHOSTAVINTERFACESTATUS_ENABLED;// Status of the Avinterface always enable*/

        netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,
                                    &(entry->ocStbHostAVInterfaceIndex),
                                      sizeof(entry->ocStbHostAVInterfaceIndex));

        netsnmp_tdata_add_row(table_data, row);
    //netsnmp_tdata_copy_row(temprow,row);
    }
    return 1;

}

 SNMPocStbHostInBandTunerTable_t InBandTunerTable_obj;//[SNMP_MAX_ROWS];

/**
 * @brief This function is used to get the info about in-band tuners such as interface type,  
 * Interleaver, BER, Bandwidth, map/dac Id etc and updates these info to in-band tuner table.
 *
 * @param[out] table_data Table where info about in-band tuners are stored.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
 int vl_ocStbHostInBandTunerTable_getdata(netsnmp_tdata *table_data)
{


   /**
    vlGet_ocStbHostInBandTunerTableData :: to get the number of rows and data into the structure InBandTunerTable_obj
     */  //int number_rows;

    u_long tunerAvInterfaceIx = 0;
    int ixAvIntTable = 0;
    int ntimereplace = 0;
    bool InBandTunerTableUpdatedNow = false;
   // SNMP_DEBUGPRINT("\n vl_getHostInBandTunerTable ::  COLLECTING THE DATA::::::::::::: \n" );

    //Avinterface obj;
    bool replace_table = false;

    if(netsnmp_tdata_row_first(table_data))
    {
        replace_table = true;
    }


    for (ixAvIntTable = 0; ixAvIntTable < vlg_number_rows; ixAvIntTable++)
    {

       // SNMP_DEBUGPRINT("vlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceType = %d \n",vlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceType);
      //  SNMP_DEBUGPRINT("vlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceIndex = %d",
      //     vlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.tunerIndex);

        /**Fetching the TunerIndex which is corresponding to the Avindex list::
        Comparing the avType_t and Tunerindex(plugindex) with the avinterface global list and making the "tunerAvInterfaceIx" as a tuner-Table index*/

        if( vlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceType == ocStbHostScte40FatRx /*TUNER TYPE*/)
        {

            tunerAvInterfaceIx = ixAvIntTable + 1; // AVInterface Table Index starts at 1
                        //== tunerResourceIndex)



            //vlMemSet(&InBandTunerTable_obj, 0, sizeof(InBandTunerTable_obj), sizeof(InBandTunerTable_obj));
            memset(&InBandTunerTable_obj, 0, sizeof(InBandTunerTable_obj));
//    InBandTunerTable_obj = (SNMPocStbHostInBandTunerTable_t*)vlMalloc(sizeof(SNMPocStbHostInBandTunerTable_t));

            if (0 == avobj.vl_getHostInBandTunerTable(&InBandTunerTable_obj, InBandTunerTableUpdatedNow, vlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.tunerIndex/*tunerplugAVindex*/))
            {

                SNMP_DEBUGPRINT("ERROR:: avobj.vl_getHostInBandTunerTable");
                return 0;
            }

            if(replace_table)
            {
                replace_table = false;
                Table_free(table_data);

            }
            ocStbHostInBandTunerTable_copy_allData(table_data,  tunerAvInterfaceIx, &InBandTunerTable_obj);
                //free(InBandTunerTable_obj);
        } //if condition

    }//for loop
    return 1;
}

/**
 * @brief This function copies in-band interface information from passed input parameter
 * to the In-Band Tuner table.
 *
 * @param[in] InBandTunerTable_obj Struct pointer where in-band interface data is present.
 * @param[in] tunerAvInterfaceIx Index number of tuner interface
 * @param[out] table_data Table where interface data is to be stored.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int ocStbHostInBandTunerTable_copy_allData(netsnmp_tdata * table_data, int tunerAvInterfaceIx ,SNMPocStbHostInBandTunerTable_t* InBandTunerTable_obj)
{


    struct ocStbHostInBandTunerTable_entry *entry;
    netsnmp_tdata_row *row;
    entry = SNMP_MALLOC_TYPEDEF(struct ocStbHostInBandTunerTable_entry);
    if (!entry)
        return 0; //NULL;

    row = netsnmp_tdata_create_row();
    if (!row) {
        SNMP_FREE(entry);
        return 0;//NULL;
    }

    //SNMP_DEBUGPRINT("\n ocStbHostInBandTunerTable_copy_allData ::  vlg_number_rows  =%d  \n", vlg_number_rows);

    row->data = entry;
    SNMP_DEBUGPRINT("\n1::: entry->vl_ocStbHostInBandTunerModulationMode = %d \n entry->ocStbHostInBandTunerModulationMode %d \n",InBandTunerTable_obj->vl_ocStbHostInBandTunerModulationMode ,entry->ocStbHostInBandTunerModulationMode );
    entry->ocStbHostAVInterfaceIndex = tunerAvInterfaceIx;

    entry->ocStbHostInBandTunerModulationMode =InBandTunerTable_obj->vl_ocStbHostInBandTunerModulationMode;

    entry->ocStbHostInBandTunerFrequency = InBandTunerTable_obj->vl_ocStbHostInBandTunerFrequency;

    entry->ocStbHostInBandTunerInterleaver = InBandTunerTable_obj->vl_ocStbHostInBandTunerInterleaver;
    entry->ocStbHostInBandTunerPower = InBandTunerTable_obj->vl_ocStbHostInBandTunerPower;
    entry->old_ocStbHostInBandTunerPower = InBandTunerTable_obj->vl_old_ocStbHostInBandTunerPower;
    entry->ocStbHostInBandTunerAGCValue = InBandTunerTable_obj->vl_ocStbHostInBandTunerAGCValue;
    entry->ocStbHostInBandTunerSNRValue = InBandTunerTable_obj->vl_ocStbHostInBandTunerSNRValue;
    entry->ocStbHostInBandTunerUnerroreds = InBandTunerTable_obj->vl_ocStbHostInBandTunerUnerroreds;
    entry->ocStbHostInBandTunerCorrecteds = InBandTunerTable_obj->vl_ocStbHostInBandTunerCorrecteds;
    entry->ocStbHostInBandTunerUncorrectables = InBandTunerTable_obj->vl_ocStbHostInBandTunerUncorrectables;
    entry->ocStbHostInBandTunerCarrierLockLost = InBandTunerTable_obj->vl_ocStbHostInBandTunerCarrierLockLost;
    entry->ocStbHostInBandTunerPCRErrors = InBandTunerTable_obj->vl_ocStbHostInBandTunerPCRErrors;
    entry->ocStbHostInBandTunerPTSErrors = InBandTunerTable_obj->vl_ocStbHostInBandTunerPTSErrors;
    entry->ocStbHostInBandTunerState         =  InBandTunerTable_obj->vl_ocStbHostInBandTunerState;
    entry->ocStbHostInBandTunerBER           = InBandTunerTable_obj->vl_ocStbHostInBandTunerBER;
    entry->ocStbHostInBandTunerSecsSinceLock = InBandTunerTable_obj->vl_ocStbHostInBandTunerSecsSinceLock;
    entry->ocStbHostInBandTunerEqGain        = InBandTunerTable_obj->vl_ocStbHostInBandTunerEqGain;
    entry->ocStbHostInBandTunerMainTapCoeff  = InBandTunerTable_obj->vl_ocStbHostInBandTunerMainTapCoeff;
    entry->ocStbHostInBandTunerTotalTuneCount = InBandTunerTable_obj->vl_ocStbHostInBandTunerTotalTuneCount;
    entry->ocStbHostInBandTunerTuneFailureCount = InBandTunerTable_obj->vl_ocStbHostInBandTunerTuneFailureCount;
    entry->ocStbHostInBandTunerTuneFailFreq = InBandTunerTable_obj->vl_ocStbHostInBandTunerTuneFailFreq;
    entry->ocStbHostInBandTunerBandwidth = InBandTunerTable_obj->vl_ocStbHostInBandTunerBandwidth;

    entry->ocStbHostInBandTunerChannelMapId = InBandTunerTable_obj->vl_ocStbHostInBandTunerChannelMapId;
    entry->ocStbHostInBandTunerDacId = InBandTunerTable_obj->vl_ocStbHostInBandTunerDacId;
    SNMP_DEBUGPRINT("\n2 ::: entry->vl_ocStbHostInBandTunerModulationMode = %d \n entry->ocStbHostInBandTunerModulationMode %d \n",InBandTunerTable_obj->vl_ocStbHostInBandTunerModulationMode ,entry->ocStbHostInBandTunerModulationMode );


    netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,
                                &(entry->ocStbHostAVInterfaceIndex),
                                  sizeof(entry->ocStbHostAVInterfaceIndex));
    netsnmp_tdata_add_row(table_data, row);
    return 1; //Mamidi:042209
}

SNMPocStbHostDVIHDMITable_t DVIHDMITable_obj;
/**
 * @brief This function is used to get dvi/hdmi data like Output Type, Connection Status,
 * HDCP Status, audio/video Format, FrameRate, Cec Features etc and fill to DVI/HDMI Table.
 *
 * @param[out] table_data Table where dvi/hdmi info has to be stored.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int vl_ocStbHostDVIHDMITable_getdata(netsnmp_tdata *table_data)
{


    bool DVIHDMITableUpdatedNow = false;

    int ixAvIntTable = 0;
    u_long HDMIInterfaceAvInterfaceIx;

    bool replace_table = false;

    if(netsnmp_tdata_row_first(table_data))
    {
        replace_table = true;
    }

    for(ixAvIntTable = 0; ixAvIntTable < vlg_number_rows; ixAvIntTable++)
    {

       // SNMP_DEBUGPRINT("\nvlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceType = Ox%x \n",vlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceType);
       //SNMP_DEBUGPRINT("\nvlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.dispResData.DisplayPortType=%d\n",vlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.dispResData.DisplayPortType);

    //SNMP_DEBUGPRINT("\nxocStbHostHdmiOut 0x%x \t xAV_INTERFACE_TYPE_HDMI 0x% \n",ocStbHostHdmiOut,AV_INTERFACE_TYPE_HDMI);
        if( (vlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceType == ocStbHostHdmiOut ) &&   (vlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.dispResData.DisplayPortType == AV_INTERFACE_TYPE_HDMI))
        {

            HDMIInterfaceAvInterfaceIx = ixAvIntTable + 1; // AVInterface Table Index starts at 1
       //SNMP_DEBUGPRINT("\nixAvIntTable :: %d \tHDMIInterfaceAvInterfaceIx:: %d \n",HDMIInterfaceAvInterfaceIx, ixAvIntTable);

          // Avinterface obj;

          //SNMP_DEBUGPRINT("\n vl_getocStbHostDVIHDMITableData ::  COLLECTING THE DATA:::::::::::::  \n" );

       
//     DVIHDMITable_obj = (SNMPocStbHostDVIHDMITable_t*)vlMalloc(sizeof(SNMPocStbHostDVIHDMITable_t));
            //vlMemSet(&DVIHDMITable_obj,0,sizeof(DVIHDMITable_obj), sizeof(DVIHDMITable_obj));
            memset(&DVIHDMITable_obj,0,sizeof(DVIHDMITable_obj));

            if(0 == avobj.vl_getocStbHostDVIHDMITableData( &DVIHDMITable_obj, DVIHDMITableUpdatedNow,vlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.dispResData.hDisplay,vlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.dispResData.DisplayPortType ))
            {
                SNMP_DEBUGPRINT("ERROR:: vl_getocStbHostDVIHDMITableData");
                return 0;
            }


            if(replace_table)
            {
                replace_table = false;
                Table_free(table_data);
            }

            ocStbHostDVIHDMITable_copy_allData(table_data,  HDMIInterfaceAvInterfaceIx, &DVIHDMITable_obj);

           //free(DVIHDMITable_obj);
        } //if entry of DVIHDMI exits or not

//     if (ixAvIntTable == vlg_number_rows)
//     {
//         //ERROR : we did nt find this tuner entry in AVINterface Table
//         SNMP_DEBUGPRINT("ERROR : we did nt find this tuner entry in AVINterface Table\n");
//         //SNMP_FREE(entry);
//         return 1;//NULL;
//     }


    }//for loop
    return 1;

}

/**
 * @brief This function copies dvi/hdmi information from passed input parameter
 * to the DVI/HDMI table.
 *
 * @param[in] HDMIInterfaceAvInterfaceIx DVI/HDMI interface index.
 * @param[in] DVIHDMITable_obj Struct pointer where dvi/hdmi data is present.
 * @param[out] table_data Table where dvi/hdmi info is retrieved.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int ocStbHostDVIHDMITable_copy_allData(netsnmp_tdata * table_data, int HDMIInterfaceAvInterfaceIx ,SNMPocStbHostDVIHDMITable_t* DVIHDMITable_obj)
{
    struct ocStbHostDVIHDMITable_entry *entry;
    netsnmp_tdata_row *row;
    entry = SNMP_MALLOC_TYPEDEF(struct ocStbHostDVIHDMITable_entry);
    if (!entry)
        return 0;

    row = netsnmp_tdata_create_row();
    if (!row) {
        SNMP_FREE(entry);
        return 0;
    }
    row->data = entry;
    entry->ocStbHostAVInterfaceIndex = HDMIInterfaceAvInterfaceIx;
    entry->ocStbHostDVIHDMIOutputType = DVIHDMITable_obj->vl_ocStbHostDVIHDMIOutputType;
    entry->ocStbHostDVIHDMIConnectionStatus = DVIHDMITable_obj->vl_ocStbHostDVIHDMIConnectionStatus;
    entry->ocStbHostDVIHDMIRepeaterStatus= DVIHDMITable_obj->vl_ocStbHostDVIHDMIRepeaterStatus;
    entry->ocStbHostDVIHDMIVideoXmissionStatus = DVIHDMITable_obj->vl_ocStbHostDVIHDMIVideoXmissionStatus;
    entry->ocStbHostDVIHDMIHDCPStatus = DVIHDMITable_obj->vl_ocStbHostDVIHDMIHDCPStatus;
    entry->ocStbHostDVIHDMIVideoMuteStatus = DVIHDMITable_obj->vl_ocStbHostDVIHDMIVideoMuteStatus;
    entry->ocStbHostDVIHDMIOutputFormat = DVIHDMITable_obj->vl_ocStbHostDVIHDMIOutputFormat;
    entry->ocStbHostDVIHDMIAspectRatio = DVIHDMITable_obj->vl_ocStbHostDVIHDMIAspectRatio;
    entry->ocStbHostDVIHDMIHostDeviceHDCPStatus = DVIHDMITable_obj->vl_ocStbHostDVIHDMIHostDeviceHDCPStatus;
    entry->ocStbHostDVIHDMIAudioFormat = DVIHDMITable_obj->vl_ocStbHostDVIHDMIAudioFormat;
    entry->ocStbHostDVIHDMIAudioSampleRate = DVIHDMITable_obj->vl_ocStbHostDVIHDMIAudioSampleRate;
    entry->ocStbHostDVIHDMIAudioChannelCount = DVIHDMITable_obj->vl_ocStbHostDVIHDMIAudioChannelCount;
    entry->ocStbHostDVIHDMIAudioMuteStatus = DVIHDMITable_obj->vl_ocStbHostDVIHDMIAudioMuteStatus;
    entry->ocStbHostDVIHDMIAudioSampleSize = DVIHDMITable_obj->vl_ocStbHostDVIHDMIAudioSampleSize;
    entry->ocStbHostDVIHDMIColorSpace = DVIHDMITable_obj->vl_ocStbHostDVIHDMIColorSpace;
    entry->ocStbHostDVIHDMIFrameRate  = DVIHDMITable_obj->vl_ocStbHostDVIHDMIFrameRate;

    entry->ocStbHostDVIHDMIAttachedDeviceType = DVIHDMITable_obj->vl_ocStbHostDVIHDMIAttachedDeviceType;
    entry->ocStbHostDVIHDMIEdid_len = DVIHDMITable_obj->vl_ocStbHostDVIHDMIEdid_len;
    vlMemCpy(entry->ocStbHostDVIHDMIEdid,  DVIHDMITable_obj->vl_ocStbHostDVIHDMIEdid,
             entry->ocStbHostDVIHDMIEdid_len,
             sizeof(entry->ocStbHostDVIHDMIEdid));
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP","%s : returning %d bytes of EDID data\n", __FUNCTION__, entry->ocStbHostDVIHDMIEdid_len);
    entry->ocStbHostDVIHDMILipSyncDelay = DVIHDMITable_obj->vl_ocStbHostDVIHDMILipSyncDelay;
    entry->ocStbHostDVIHDMICecFeatures_len =  DVIHDMITable_obj->vl_ocStbHostDVIHDMICecFeatures_len;
    vlMemCpy(entry->ocStbHostDVIHDMICecFeatures,  DVIHDMITable_obj->vl_ocStbHostDVIHDMICecFeatures,
          entry->ocStbHostDVIHDMICecFeatures_len,
         CHRMAX);
///test to check the out-put
//     entry->ocStbHostDVIHDMICecFeatures_len = 16;//16bit
//     char temp[16] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
//     vlMemCpy(entry->ocStbHostDVIHDMICecFeatures, temp,
//            entry->ocStbHostDVIHDMICecFeatures_len,
//           CHRMAX);
///MIB output :: OC-STB-HOST-MIB::ocStbHostDVIHDMICecFeatures.4 = "01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01"

    entry->ocStbHostDVIHDMIFeatures_len = DVIHDMITable_obj->vl_ocStbHostDVIHDMIFeatures_len;
    vlMemCpy(entry->ocStbHostDVIHDMIFeatures,  DVIHDMITable_obj->vl_ocStbHostDVIHDMIFeatures,
             entry->ocStbHostDVIHDMIFeatures_len,
             CHRMAX);
    entry->ocStbHostDVIHDMIMaxDeviceCount = DVIHDMITable_obj->vl_ocStbHostDVIHDMIMaxDeviceCount;
    
    entry->ocStbHostDVIHDMIPreferredVideoFormat =
            DVIHDMITable_obj->vl_ocStbHostDVIHDMIPreferredVideoFormat;
    entry->ocStbHostDVIHDMIEdidVersion_len =
            DVIHDMITable_obj->vl_ocStbHostDVIHDMIEdidVersion_len;
    vlMemCpy(entry->ocStbHostDVIHDMIEdidVersion,
             DVIHDMITable_obj->vl_ocStbHostDVIHDMIEdidVersion,
             entry->ocStbHostDVIHDMIEdidVersion_len,
             CHRMAX);
    entry->ocStbHostDVIHDMI3DCompatibilityControl =
            DVIHDMITable_obj->vl_ocStbHostDVIHDMI3DCompatibilityControl;
    entry->ocStbHostDVIHDMI3DCompatibilityMsgDisplay =
            DVIHDMITable_obj->vl_ocStbHostDVIHDMI3DCompatibilityMsgDisplay;
    netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,
                                &(entry->ocStbHostAVInterfaceIndex),
                                  sizeof(entry->ocStbHostAVInterfaceIndex));
    netsnmp_tdata_add_row(table_data, row);

    return 1;//Mamidi:042209

}

/**
 * @brief This function is used to get dvi/hdmi info like Video Format, Supported 3D Structures,
 * Active 3D Structure and fills to DVI/HDMI video format Table.
 *
 * @param[out] table_data Table where info about dvi/hdmi video format is retrieved.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int vl_ocStbHostDVIHDMIAvailableVideoFormatTable_getdata(netsnmp_tdata* table_data)
{
    bool replace_table = false;
    int nfmype = 0;

    if( netsnmp_tdata_row_first(table_data))
    {
        replace_table = true;
    }
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP","\n %s----- %d start \n",__FUNCTION__, __LINE__);
    SNMPocStbHostDVIHDMIVideoFormatTable_t vedeofmt[MAX_ROWS];
    //vlMemSet(vedeofmt, 0 , sizeof(vedeofmt), sizeof(vedeofmt));
    memset(vedeofmt, 0 , sizeof(vedeofmt));

    if( 0 == avobj.vlGet_ocStbHostDVIHDMIVideoFormatTableData(vedeofmt, &nfmype))
    {
        SNMP_DEBUGPRINT("ERROR ::vlGet_ocStbHostSystemMemoryReportTableData");
           // return 0;
    }
    if(nfmype != 0)
    {
        for(int nmem = 0; nmem < nfmype; nmem++)
        {
            if(replace_table)
            {
                replace_table = false;
                Table_free(table_data);
            }

            ocStbHostDVIHDMIAvailableVideoFormatTable__createEntry_allData(table_data, &vedeofmt[nmem], nmem);
        }
    }
    else
    {
        /** Default Intilization is don if no entries are there , this is for presen's of Table */
        Table_free(table_data);

        SNMPocStbHostDVIHDMIVideoFormatTable_t vedeofmtobj;
        //vlMemSet(&vedeofmtobj, 0 , sizeof(vedeofmtobj),  sizeof(vedeofmtobj));
        memset(&vedeofmtobj, 0 , sizeof(vedeofmtobj));
        ocStbHostDVIHDMIAvailableVideoFormatTable__createEntry_allData(table_data,&vedeofmtobj, 0);
    }
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP","\n %s----- %d \n",__FUNCTION__, __LINE__);
    return 1;
}

/**
 * @brief This function is used to create and copy entries for dvi/hdmi information
 * from passed input parameter to the DVI/HDMI video format table.
 *
 * @param[in] DVIHDMIAVideoFormatIndex DVI/HDMI video format index.
 * @param[in] DVIHDMIVideoFormatTable_obj Struct pointer where dvi/hdmi video format data is present.
 * @param[out] table_data Table where dvi/hdmi video format info is stored.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int ocStbHostDVIHDMIAvailableVideoFormatTable__createEntry_allData(netsnmp_tdata * table_data,  SNMPocStbHostDVIHDMIVideoFormatTable_t* DVIHDMIVideoFormatTable_obj, int DVIHDMIAVideoFormatIndex)
{
    struct ocStbHostDVIHDMIAvailableVideoFormatTable_entry *entry;
    netsnmp_tdata_row *row;
    entry = SNMP_MALLOC_TYPEDEF(struct ocStbHostDVIHDMIAvailableVideoFormatTable_entry);
    if (!entry)
        return 0;

    row = netsnmp_tdata_create_row();
    if (!row) {
        SNMP_FREE(entry);
        return 0;
    }
    row->data = entry;
    entry->ocStbHostDVIHDMIAvailableVideoFormatIndex = DVIHDMIAVideoFormatIndex +1;
    // +1 is for index purpose
    entry->ocStbHostDVIHDMIAvailableVideoFormat = DVIHDMIVideoFormatTable_obj->vl_ocStbHostDVIHDMIAvailableVideoFormat;
    entry->ocStbHostDVIHDMISupported3DStructures_len = DVIHDMIVideoFormatTable_obj->vl_ocStbHostDVIHDMISupported3DStructures_len;
    vlMemCpy(entry->ocStbHostDVIHDMISupported3DStructures,
             DVIHDMIVideoFormatTable_obj->vl_ocStbHostDVIHDMISupported3DStructures,
             entry->ocStbHostDVIHDMISupported3DStructures_len,
             CHRMAX);
    entry->ocStbHostDVIHDMIActive3DStructure = DVIHDMIVideoFormatTable_obj->vl_ocStbHostDVIHDMIActive3DStructure;
    netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,
                                &(entry->ocStbHostDVIHDMIAvailableVideoFormatIndex),
                                  sizeof(entry->ocStbHostDVIHDMIAvailableVideoFormatIndex));
    netsnmp_tdata_add_row(table_data, row);

    return 1;
}

SNMPocStbHostComponentVideoTable_t ComponentVideoTable_obj;
/**
 * @brief This function is used to get component video info like AV interface index, video
 * constrained status, output format, aspect ratio etc and fill these info in Component Video Table.
 *
 * @param[out] table_data Table where component video info is retrieved.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int vl_ocStbHostComponentVideoTable_getdata(netsnmp_tdata *table_data)
{

    bool ComponentVideoTableUpdatedNow = false;

    bool replace_table = false;

    if(netsnmp_tdata_row_first(table_data))
    {
        replace_table = true;
    }
    int ixAvIntTable;
    u_long ComponentVideoAvInterfaceIx ;

    for(ixAvIntTable = 0; ixAvIntTable < vlg_number_rows; ixAvIntTable++)
    {

     //  SNMP_DEBUGPRINT("\nvlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceType = Ox%x \n",vlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceType);
    //   SNMP_DEBUGPRINT("\nvlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.dispResData.DisplayPortType=%d\n",vlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.dispResData.DisplayPortType);

    //SNMP_DEBUGPRINT("\nxocStbHostHdmiOut 0x%x \t xAV_INTERFACE_TYPE_HDMI 0x% \n",ocStbHostHdmiOut,AV_INTERFACE_TYPE_HDMI);
        if( (vlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceType == ocStbHostHdComponentOut ) &&   (vlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.dispResData.DisplayPortType == AV_INTERFACE_TYPE_COMPONENT ))
        {

            ComponentVideoAvInterfaceIx = ixAvIntTable + 1; // AVInterface Table Index starts at 1
         //SNMP_DEBUGPRINT("\n ixAvIntTable :: %d \t omponentVideoAvInterfaceIx:: %d \n", ixAvIntTable, ComponentVideoAvInterfaceIx);

         
//          ComponentVideoTable_obj = (SNMPocStbHostComponentVideoTable_t*)vlMalloc(sizeof(SNMPocStbHostComponentVideoTable_t));

     //Avinterface obj;

            //vlMemSet(&ComponentVideoTable_obj, 0, sizeof(ComponentVideoTable_obj), sizeof(ComponentVideoTable_obj));
            memset(&ComponentVideoTable_obj, 0, sizeof(ComponentVideoTable_obj));
            if(0 == avobj.vl_getocStbHostComponentVideoTableData( &ComponentVideoTable_obj,  ComponentVideoTableUpdatedNow, vlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.dispResData.hDisplay,vlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.dispResData.DisplayPortType ))
            {
                SNMP_DEBUGPRINT("ERROR:: vlGet_ocStbHostComponentVideoTableData");
                return 0;
            }

            if(replace_table)
            {
                replace_table = false;
                Table_free(table_data);
            }

            ComponentVideoTable_copy_allData(table_data,  ComponentVideoAvInterfaceIx, &ComponentVideoTable_obj);


        //free(ComponentVideoTable_obj);
        } //if checks weather Videoentry exits or not


//     if (ixAvIntTable == vlg_number_rows)
//     {
//         //ERROR : we did nt find this tuner entry in AVINterface Table
//         SNMP_DEBUGPRINT("ERROR : we did nt find ocStbHostComponentVideoTableData in AVINterface Table\n");
//         //SNMP_FREE(entry);
//         return 0;//NULL;
//     }

    }//for loop
    return 1;
}

/**
 * @brief This function copies component video information from passed input parameter
 * to the Component video table.
 *
 * @param[in] ComponentVideoAvInterfaceIx Component video interface index.
 * @param[in] ComponentVideoTable_obj Struct pointer where component video data is present.
 * @param[out] table_data Table where component video info is retrieved.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int ComponentVideoTable_copy_allData(netsnmp_tdata * table_data,int  ComponentVideoAvInterfaceIx,SNMPocStbHostComponentVideoTable_t* ComponentVideoTable_obj)
{

    struct ocStbHostComponentVideoTable_entry *entry;
    netsnmp_tdata_row *row;
    entry = SNMP_MALLOC_TYPEDEF(struct ocStbHostComponentVideoTable_entry);
    if (!entry)
        return 0;

    row = netsnmp_tdata_create_row();
    if (!row) {
        SNMP_FREE(entry);
        return 0;
    }
    row->data = entry;
    entry->ocStbHostAVInterfaceIndex = ComponentVideoAvInterfaceIx;
    entry->ocStbHostComponentVideoConstrainedStatus = ComponentVideoTable_obj->vl_ocStbHostComponentVideoConstrainedStatus;
    entry->ocStbHostComponentOutputFormat = ComponentVideoTable_obj->vl_ocStbHostComponentOutputFormat;
    entry->ocStbHostComponentAspectRatio = ComponentVideoTable_obj->vl_ocStbHostComponentAspectRatio;
    entry->ocStbHostComponentVideoMuteStatus = ComponentVideoTable_obj->vl_ocStbHostComponentVideoMuteStatus;


    netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,
                                &(entry->ocStbHostAVInterfaceIndex),
                                  sizeof(entry->ocStbHostAVInterfaceIndex));
    netsnmp_tdata_add_row(table_data, row);
    return 1; //Mamidi:042209

}

SNMPocStbHostRFChannelOutTable_t RFChannelOutTable_obj;
/**
 * @brief This function is used to get RF Channel info like Channel Out type, 
 * audio/video mute status etc and fill to RF out table.
 *
 * @param[out] table_data Table where RF out info is stored.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int vl_ocStbHostRFChannelOutTable_getdata(netsnmp_tdata *table_data)
{


    bool RFChannelOutTableTableUpdatedNow = false;
    bool replace_table = false;

    if(netsnmp_tdata_row_first(table_data))
    {
        replace_table = true;
    }
    int ixAvIntTable;
    u_long RFChannelOutAvInterfaceIx;

    for(ixAvIntTable = 0; ixAvIntTable < vlg_number_rows; ixAvIntTable++)
    {

      //  SNMP_DEBUGPRINT("\nvlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceType = Ox%x \n",vlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceType);
     //  SNMP_DEBUGPRINT("\nvlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.dispResData.DisplayPortType=%d\n",vlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.dispResData.DisplayPortType);

        if( (vlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceType == ocStbHostRFOutCh ) &&   (vlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.dispResData.DisplayPortType == AV_INTERFACE_TYPE_RF ))
        {

            RFChannelOutAvInterfaceIx = ixAvIntTable + 1; // AVInterface Table Index starts at 1
       // SNMP_DEBUGPRINT("\nixAvIntTable :: %d \tHDMIInterfaceAvInterfaceIx:: %d \n",RFChannelOutAvInterfaceIx, ixAvIntTable);


        
//         RFChannelOutTable_obj = (SNMPocStbHostRFChannelOutTable_t*)vlMalloc(sizeof(SNMPocStbHostRFChannelOutTable_t));
            //vlMemSet(&RFChannelOutTable_obj, 0, sizeof(RFChannelOutTable_obj), sizeof(RFChannelOutTable_obj));
            memset(&RFChannelOutTable_obj, 0, sizeof(RFChannelOutTable_obj));
    //Avinterface obj;
            if(0 == avobj.vl_getocStbHostRFChannelOutTableData( &RFChannelOutTable_obj, RFChannelOutTableTableUpdatedNow,vlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.dispResData.hDisplay,vlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.dispResData.DisplayPortType ))
            {
                SNMP_DEBUGPRINT("ERROR:: avobj.vlGet_ocStbHostRFChannelOutTableData");
                return 0;
            }

            if(replace_table)
            {
                replace_table = false;
                Table_free(table_data);
            }

            RFChannelOutTable_copy_allData(table_data,  RFChannelOutAvInterfaceIx, &RFChannelOutTable_obj);


        //free(RFChannelOutTable_obj);

        } //if check'a and call's the moudle

//     if (ixAvIntTable == vlg_number_rows)
//     {
//         //ERROR : we did nt find this tuner entry in AVINterface Table
//         SNMP_DEBUGPRINT("ERROR : we did nt find this tuner entry in AVINterface Table\n");
//         //SNMP_FREE(entry);
//         return 1;//NULL;
//     }

    }//for loop
    return 1;
}

/**
 * @brief This function copies information about RF channel from passed input parameter
 * to the RF out table.
 *
 * @param[in] RFChannelOutAvInterfaceIx RF out interface index.
 * @param[in] RFChannelOutTable_obj Struct pointer where RF out data is present.
 * @param[out] table_data Table where info about RF out is retrieved.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int RFChannelOutTable_copy_allData(netsnmp_tdata* table_data, int RFChannelOutAvInterfaceIx, SNMPocStbHostRFChannelOutTable_t* RFChannelOutTable_obj)
{

    struct ocStbHostRFChannelOutTable_entry *entry;
    netsnmp_tdata_row *row;
    entry = SNMP_MALLOC_TYPEDEF(struct ocStbHostRFChannelOutTable_entry);
    if (!entry)
        return 0;

    row = netsnmp_tdata_create_row();
    if (!row) {
        SNMP_FREE(entry);
        return 0;
    }
    // SNMP_DEBUGPRINT("\n ERROR:: DEG :: ocStbHostRFChannelOutTable_createEntry_allData :: \n");
    row->data = entry;
    entry->ocStbHostAVInterfaceIndex = RFChannelOutAvInterfaceIx;
    entry->ocStbHostRFChannelOut = RFChannelOutTable_obj->vl_ocStbHostRFChannelOut;
    entry->ocStbHostRFChannelOutAudioMuteStatus = RFChannelOutTable_obj->vl_ocStbHostRFChannelOutAudioMuteStatus;
    entry->ocStbHostRFChannelOutVideoMuteStatus = RFChannelOutTable_obj->vl_ocStbHostRFChannelOutVideoMuteStatus;

    netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,
                                &(entry->ocStbHostAVInterfaceIndex),
                                  sizeof(entry->ocStbHostAVInterfaceIndex));
    netsnmp_tdata_add_row(table_data, row);
    return 1;//Mamidi:042209
}

SNMPocStbHostAnalogVideoTable_t AnalogVideoTable_obj;
/**
 * @brief This function is used to get Analog video info like video Protection Status etc
 * and fill the Analog video table.
 *
 * @param[out] table_data Table where analog video info is retrieved.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int vl_ocStbHostAnalogVideoTable_getdata(netsnmp_tdata *table_data)
{

    int nrows,ixmrows;
    int ixrow;

    bool AnalogVideoTableUpdatedNow = false;
    bool replace_table = false;

    if(netsnmp_tdata_row_first(table_data))
    {
        replace_table = true;
    }
    int nitemreplace = 0;
    int ixAvIntTable;
    u_long AnalogVideoAvInterfaceIx;

    for(ixAvIntTable = 0; ixAvIntTable < vlg_number_rows; ixAvIntTable++)
    {
        //SNMP_DEBUGPRINT("\n ::::::::: vl_ocStbHostAnalogVideoTable_getdata :::::::::: \n ");
        //SNMP_DEBUGPRINT("\nvlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceType = Ox%x \n",vlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceType);
       //SNMP_DEBUGPRINT("\nvlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.dispResData.DisplayPortType=%d\n",vlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.dispResData.DisplayPortType);

    //SNMP_DEBUGPRINT("\nxocStbHostHdmiOut 0x%x \t xAV_INTERFACE_TYPE_HDMI 0x% \n",ocStbHostHdmiOut,AV_INTERFACE_TYPE_HDMI);
        if(( (vlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceType ==  ocStbHostBbVideoOut) || (vlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceType ==  ocStbHostHdComponentOut) || (vlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceType ==  ocStbHostRFOutCh) || (vlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceType ==  ocStbHostSVideoOut)) &&   ((vlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.dispResData.DisplayPortType == AV_INTERFACE_TYPE_COMPOSITE ) ||
              (vlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.dispResData.DisplayPortType ==  AV_INTERFACE_TYPE_SVIDEO ) || (vlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.dispResData.DisplayPortType == AV_INTERFACE_TYPE_RF) || (vlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.dispResData.DisplayPortType == AV_INTERFACE_TYPE_COMPONENT)))
        {

            AnalogVideoAvInterfaceIx = ixAvIntTable + 1; // AVInterface Table Index starts at 1

     //SNMP_DEBUGPRINT("\n ixAvIntTable :: %d \t nalogVideoAvInterfaceIx:: %d \n", ixAvIntTable, AnalogVideoAvInterfaceIx);
         
//   AnalogVideoTable_obj = (SNMPocStbHostAnalogVideoTable_t*)vlMalloc(sizeof(SNMPocStbHostAnalogVideoTable_t));
            //
            //vlMemSet(&AnalogVideoTable_obj,0,sizeof(AnalogVideoTable_obj), sizeof(AnalogVideoTable_obj));
            memset(&AnalogVideoTable_obj,0,sizeof(AnalogVideoTable_obj));
    //Avinterface obj;
            if(0 == avobj.vlGet_ocStbHostAnalogVideoTable(&AnalogVideoTable_obj, AnalogVideoTableUpdatedNow,vlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.dispResData.hDisplay,vlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.dispResData.DisplayPortType ))
            {
                SNMP_DEBUGPRINT("ERROR:: vlGet_ocStbHostAnalogVideoTable");
                return 0;
            }

            if(replace_table)
            {

                replace_table = false;
                Table_free(table_data);
            }

            AnalogVideoTable_copy_allData(table_data, AnalogVideoAvInterfaceIx, &AnalogVideoTable_obj);

             //free(AnalogVideoTable_obj);

        } //if checks weather AnalogVideo entry exits or not

//     if (ixAvIntTable == vlg_number_rows)
//     {
//         //ERROR : we did nt find this tuner entry in AVINterface Table
//         SNMP_DEBUGPRINT("ERROR : we did nt find this tuner entry in AVINterface Table\n");
//         //SNMP_FREE(entry);
//         return 1;//NULL;
//     }

    }//for loop

    return 1;
}

/**
 * @brief This function is used to get analog video data from passed input paramenter
 * and copies to analog video table.
 *
 * @param[in] AnalogVideoAvInterfaceIx Analog video interface index.
 * @param[in] AnalogVideoTable_obj Struct pointer where Analog video data is present.
 * @param[out] table_data Table where info about Analog video is stored.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int AnalogVideoTable_copy_allData(netsnmp_tdata* table_data, int AnalogVideoAvInterfaceIx, SNMPocStbHostAnalogVideoTable_t* AnalogVideoTable_obj)
{

    struct ocStbHostAnalogVideoTable_entry *entry;
    netsnmp_tdata_row *row;
    //SNMP_DEBUGPRINT("\n :: AnalogVideoTable_copy_allData :: \n");
    entry = SNMP_MALLOC_TYPEDEF(struct ocStbHostAnalogVideoTable_entry);
    if (!entry)
        return 0;

    row = netsnmp_tdata_create_row();
    if (!row) {
        SNMP_FREE(entry);
        return 0;
    }
    row->data = entry;
    entry->ocStbHostAVInterfaceIndex = AnalogVideoAvInterfaceIx;
    entry->ocStbHostAnalogVideoProtectionStatus =  AnalogVideoTable_obj->vl_ocStbHostAnalogVideoProtectionStatus;

    netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,
                                &(entry->ocStbHostAVInterfaceIndex),
                                  sizeof(entry->ocStbHostAVInterfaceIndex));
    netsnmp_tdata_add_row(table_data, row);

    return 1;
}


// for ProgramStausTable
#define MAX_1394_PGSUPPORT 2
programInfo vlg_programInfo[SNMP_MAX_ROWS];
int vlg_programInfo_nrows = 0;
SNMPocStbHostProgramStatusTable_t ProgramStatusTable_obj;
//SNMPocStbHostProgramStatusTable_t ProgramStatusTable_obj;
//programIEEE1394Info vlg_pgieee1394info[MAX_1394_PGSUPPORT];

/**
 * @brief This function is used to get the info Program Status like Status Index, Program AV Source,
 * AV Destination, Content Source and fills these to Program Status Table.
 *
 * @param[out] table_data Table where Program Status info is retrieved.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int vl_ocStbHostProgramStatusTable_getdata(netsnmp_tdata * table_data)
{

    int TunerPhysicalID;
    int nrows,ixmrows;

    bool replace_table = false;
   //SNMP_DEBUGPRINT("\n:::::::: in vl_get_ocStbHostProgramStatusTable_load::::::: \n");
    if(netsnmp_tdata_row_first(table_data))
    {
        replace_table = true;
    }
    else{}
    int nitemreplace = 0;
    int PgTableindex = 1;
    int ixAvIntTable= 0;
    int pgInfo_rows = 0;
    int vlg_Signal_Status = 0;
    bool ProgramStatusTableUpdatedNow = false;
    int mpgeTableindex = 0;
    SNMPocStbHostProgramStatusTable_t ProgramStatusTable_obj;

    for( int mpegrows = 0; ; mpegrows++)
    {
        struct VideoContentInfo videoContentInfo;
        if(RMF_SUCCESS != IPC_CLIENT_SNMPGetVideoContentInfo(mpegrows, &videoContentInfo))
        {
            break;
        }
        

        int Mpeg2ContentIndex                              = videoContentInfo.TunerIndex; // Order the list based on tuner-id as this stack is incapable of
                                                                                          // delivering more than one content from the same tuner.
        ProgramStatusTable_obj.ProgramStatusIndex = Mpeg2ContentIndex+1;
        ProgramStatusTable_obj.vl_ocStbHostProgramAVSource = (avType_t)Mpeg2ContentIndex;
        ProgramStatusTable_obj.vl_ocStbHostProgramAVDestination = (avType_t)9; // For RDK-Output

        vlMemCpy(ProgramStatusTable_obj.vl_ocStbHostProgramContentSource , Temp_mpeg2entry,
                (sizeof(Temp_mpeg2entry)/(sizeof(apioid))) * sizeof(oid),
                        MAX_IOD_LENGTH * sizeof(oid));
        /*MPEG2 OID types unkonw*/
        vlMemCpy( ProgramStatusTable_obj.vl_ocStbHostProgramContentDestination, Temp_mpeg2entry,
                (sizeof(Temp_mpeg2entry)/(sizeof(apioid))) * sizeof(oid),
                        MAX_IOD_LENGTH * sizeof(oid));
        ProgramStatusTable_obj.vl_ocStbHostProgramContentSource_len = (sizeof(Temp_mpeg2entry)/(sizeof(apioid)));
        ProgramStatusTable_obj.vl_ocStbHostProgramContentDestination_len = (sizeof(Temp_mpeg2entry)/(sizeof(apioid)));
        
        int iContentIndexPos = 17;
        ProgramStatusTable_obj.vl_ocStbHostProgramContentSource[iContentIndexPos]       = Mpeg2ContentIndex+1;
        ProgramStatusTable_obj.vl_ocStbHostProgramContentDestination[iContentIndexPos]  = Mpeg2ContentIndex+1;
        
        int iContentTypePos = 14;
        
        switch(videoContentInfo.containerFormat)
        {
            case VideoContainerFormat_mpeg2:
            {
                ProgramStatusTable_obj.vl_ocStbHostProgramContentSource[iContentTypePos] = 3;
                ProgramStatusTable_obj.vl_ocStbHostProgramContentDestination[iContentTypePos] = 3;
            }
            break;
            
            case VideoContainerFormat_mpeg4:
            {
                ProgramStatusTable_obj.vl_ocStbHostProgramContentSource[iContentTypePos] = 5;
                ProgramStatusTable_obj.vl_ocStbHostProgramContentDestination[iContentTypePos] = 5;
            }
            break;
            
            case VideoContainerFormat_vc1:
            {
                ProgramStatusTable_obj.vl_ocStbHostProgramContentSource[iContentTypePos] = 6;
                ProgramStatusTable_obj.vl_ocStbHostProgramContentDestination[iContentTypePos] = 6;
            }
            break;
            
            default:
            {
            }
            break;
        }
        
        if(replace_table)
        {
            replace_table = false;
            Table_free(table_data);
        }
        ocStbHostProgramStatusTable_copy_allData(table_data, &ProgramStatusTable_obj);
        ++mpgeTableindex;
    }//for loop
    if(0 == mpgeTableindex)
    {
        SNMP_DEBUGPRINT("\n ERROR:: vlGet_ocStbHostProgramStatusTableData \n");
        /* ProgramStatusTable_obj = (SNMPocStbHostProgramStatusTable_t*)vlMalloc(sizeof(SNMPocStbHostProgramStatusTable_t));*/   
        //vlMemSet(&ProgramStatusTable_obj,0,sizeof(ProgramStatusTable_obj), sizeof(ProgramStatusTable_obj));
        memset(&ProgramStatusTable_obj,0,sizeof(ProgramStatusTable_obj));
        if(replace_table)
        {
            replace_table = false;
            Table_free(table_data);
        }
        ProgramStatusTable_obj.ProgramStatusIndex = 1;
        /*ocStbHostProgramStatusTable_DeafultIntilize*/
        ocStbHostProgramStatusTable_copy_allData(table_data, &ProgramStatusTable_obj);
    }
    return 1;
}

#if 0
/* if nothing is exit's then some default intilize it is used */
int ocStbHostProgramStatusTable_DeafultIntilize(netsnmp_tdata* table_data,SNMPocStbHostProgramStatusTable_t* ProgramStatusTable_obj)
{

/* we have to replace the table to avoid the crash for over lap of table */
    struct ocStbHostProgramStatusTable_entry *entry;
    netsnmp_tdata_row *row;
    entry = SNMP_MALLOC_TYPEDEF(struct ocStbHostProgramStatusTable_entry);
    if (!entry)
        return 0;

    row = netsnmp_tdata_create_row();
    if (!row) {
        SNMP_FREE(entry);
        return 0;
}
    row->data = entry;
    entry->ocStbHostProgramIndex = 1;
    //SNMP_DEBUGPRINT("\n ocStbHostProgramStatusTable_DeafultIntilize :: \n" );
    memset(entry->ocStbHostProgramAVSource,0,MAX_IOD_LENGTH);
    memset(entry->ocStbHostProgramAVDestination, 0, MAX_IOD_LENGTH);
    memset(entry->ocStbHostProgramContentSource, 0, MAX_IOD_LENGTH);
    memset(entry->ocStbHostProgramContentDestination, 0, MAX_IOD_LENGTH);

    netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,

                &(entry->ocStbHostProgramIndex),
                    sizeof(entry->ocStbHostProgramIndex));
   netsnmp_tdata_row *temprow;
   if(temprow = netsnmp_tdata_row_first(table_data))
{
          netsnmp_tdata_replace_row(table_data, temprow, row);
}
   else
{
    netsnmp_tdata_add_row(table_data, row);
}

    return 1;
}
#endif //if 0

/**
 * @brief This function copies info about Program Status from passed input parameter
 * to Program Status Table.
 *
 * @param[in] ProgramStatusTable_obj Struct pointer where Program Status data is present.
 * @param[out] table_data Table where info about Program Status is retrieved.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int ocStbHostProgramStatusTable_copy_allData(netsnmp_tdata* table_data,SNMPocStbHostProgramStatusTable_t* ProgramStatusTable_obj)
{
    //int i;
    struct ocStbHostProgramStatusTable_entry *entry;
    netsnmp_tdata_row *row;
    entry = SNMP_MALLOC_TYPEDEF(struct ocStbHostProgramStatusTable_entry);
    if (!entry)
        return 0;

    row = netsnmp_tdata_create_row();
    if (!row) {
        SNMP_FREE(entry);
        return 0;
    }
    row->data = entry;

 // SNMP_DEBUGPRINT("\n ocStbHostProgramStatusTable_copy_allData :: \n" );
/*    vlMemSet(entry->ocStbHostProgramAVSource,0,MAX_IOD_LENGTH,MAX_IOD_LENGTH);
    vlMemSet(entry->ocStbHostProgramAVDestination, 0, MAX_IOD_LENGTH,MAX_IOD_LENGTH);
    vlMemSet(entry->ocStbHostProgramContentSource, 0, MAX_IOD_LENGTH,MAX_IOD_LENGTH);
    vlMemSet(entry->ocStbHostProgramContentDestination, 0, MAX_IOD_LENGTH,MAX_IOD_LENGTH);*/

    memset(entry->ocStbHostProgramAVSource,0,MAX_IOD_LENGTH);
    memset(entry->ocStbHostProgramAVDestination, 0, MAX_IOD_LENGTH);
    memset(entry->ocStbHostProgramContentSource, 0, MAX_IOD_LENGTH);
    memset(entry->ocStbHostProgramContentDestination, 0, MAX_IOD_LENGTH);

    entry->ocStbHostProgramIndex = ProgramStatusTable_obj->ProgramStatusIndex;
    entry->ocStbHostProgramAVSource_len = (sizeof(avTableRowPointers[ProgramStatusTable_obj->vl_ocStbHostProgramAVSource])/(sizeof(apioid)));
    entry->ocStbHostProgramAVDestination_len =(sizeof(avTableRowPointers[ProgramStatusTable_obj->vl_ocStbHostProgramAVDestination])/(sizeof(apioid)));

   //(sizeof(avTableRowPointers[ProgramStatusTable_obj->vl_ocStbHostProgramContentSource  ])/(sizeof(apioid)));
    //=(sizeof(avTableRowPointers[ProgramStatusTable_obj->vl_ocStbHostProgramContentDestination    ])/(sizeof(apioid)));

    vlMemCpy(entry->ocStbHostProgramAVSource, avTableRowPointers[ProgramStatusTable_obj->vl_ocStbHostProgramAVSource],
             entry->ocStbHostProgramAVSource_len * sizeof(oid),
                     MAX_IOD_LENGTH * sizeof(oid));
    vlMemCpy(entry->ocStbHostProgramAVDestination, avTableRowPointers[ProgramStatusTable_obj->vl_ocStbHostProgramAVDestination],
             entry->ocStbHostProgramAVDestination_len * sizeof(oid),
                     MAX_IOD_LENGTH * sizeof(oid));
  //at present pointing the ocStbHostMpeg2ContentTable Table Entry point
    entry->ocStbHostProgramContentSource_len = ProgramStatusTable_obj->vl_ocStbHostProgramContentSource_len;
    entry->ocStbHostProgramContentDestination_len = ProgramStatusTable_obj->vl_ocStbHostProgramContentDestination_len;
    
    vlMemCpy(entry->ocStbHostProgramContentSource , ProgramStatusTable_obj->vl_ocStbHostProgramContentSource,
             entry->ocStbHostProgramContentSource_len * sizeof(oid),
                     MAX_IOD_LENGTH * sizeof(oid));
    vlMemCpy(entry->ocStbHostProgramContentDestination , ProgramStatusTable_obj->vl_ocStbHostProgramContentDestination,
             entry->ocStbHostProgramContentDestination_len * sizeof(oid),
                     MAX_IOD_LENGTH * sizeof(oid));
 // entry->ocStbHostProgramContentSource = 0; // not know mpeg2 type OID
 // entry->ocStbHostProgramContentDestination = 0; // not know mpeg2 type OID

    netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,
                                &(entry->ocStbHostProgramIndex),
                                  sizeof(entry->ocStbHostProgramIndex));
    netsnmp_tdata_add_row(table_data, row);
    return 1;

}

SNMPocStbHostMpeg2ContentTable_t ProgramMPEG2_obj;
/**
 * @brief This function is used to get the MPEG2 content data like Program Number, audio/video PID,
 * CCI Value, lock status (CIT, EPN, PCR), Format etc and stores these info to Content table.
 *
 * @param[out] table_data Table where info about MPEG2 content is retrieved and stored.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int vl_ocStbHostMpeg2ContentTable_getdata(netsnmp_tdata *table_data)
{
    bool Mpeg2ContentTableUpdatedNow = false;
    bool ProgramStatusTableUpdatedNow = false;
    bool replace_table = false;

    if(netsnmp_tdata_row_first(table_data))
    {
        replace_table = true;
    }

    int nitemreplace = 0;
    int mpgeTableindex = 0;
    int Signal_statsu = 0;
    // Avinterface obj;

    //vlMemSet(vlg_programInfo, 0, sizeof(vlg_programInfo),sizeof(vlg_programInfo));
    memset(vlg_programInfo, 0, sizeof(vlg_programInfo));

    for( int mpegrows = 0; ; mpegrows++)
    {
        struct VideoContentInfo videoContentInfo;
        if(RMF_SUCCESS != IPC_CLIENT_SNMPGetVideoContentInfo(mpegrows, &videoContentInfo))
        {
            break;
        }
        
        if(VideoContainerFormat_mpeg2 == videoContentInfo.containerFormat)
        {
            //ProgramMPEG2_obj.Mpeg2ContentIndex                            = videoContentInfo.ContentIndex; // Order the list based on tuner-id as this stack is incapable of
            //                                                                                               // delivering more than one content from the same tuner.
            ProgramMPEG2_obj.Mpeg2ContentIndex                              = videoContentInfo.TunerIndex+1; // Order the list based on tuner-id as this stack is incapable of
                                                                                                             // delivering more than one content from the same tuner.
            ProgramMPEG2_obj.vl_ocStbHostMpeg2ContentProgramNumber          = videoContentInfo.ProgramNumber;
            ProgramMPEG2_obj.vl_ocStbHostMpeg2ContentTransportStreamID      = videoContentInfo.TransportStreamID;
            ProgramMPEG2_obj.vl_ocStbHostMpeg2ContentTotalStreams           = videoContentInfo.TotalStreams;
            ProgramMPEG2_obj.vl_ocStbHostMpeg2ContentSelectedVideoPID       = videoContentInfo.SelectedVideoPID;
            ProgramMPEG2_obj.vl_ocStbHostMpeg2ContentSelectedAudioPID       = videoContentInfo.SelectedAudioPID;
            ProgramMPEG2_obj.vl_ocStbHostMpeg2ContentOtherAudioPIDs         = (Status_t)videoContentInfo.OtherAudioPIDs;
            ProgramMPEG2_obj.vl_ocStbHostMpeg2ContentCCIValue               = (Mpeg2ContentCCIValue_t)videoContentInfo.CCIValue;
            ProgramMPEG2_obj.vl_ocStbHostMpeg2ContentAPSValue               = (Mpeg2ContentAPSValue_t)videoContentInfo.APSValue;
            ProgramMPEG2_obj.vl_ocStbHostMpeg2ContentCITStatus              = (Status_t)videoContentInfo.CITStatus;
            ProgramMPEG2_obj.vl_ocStbHostMpeg2ContentBroadcastFlagStatus    = (Status_t)videoContentInfo.BroadcastFlagStatus;
            ProgramMPEG2_obj.vl_ocStbHostMpeg2ContentEPNStatus              = (Status_t)videoContentInfo.EPNStatus;
            ProgramMPEG2_obj.vl_ocStbHostMpeg2ContentPCRPID                 = videoContentInfo.PCRPID;
            ProgramMPEG2_obj.vl_ocStbHostMpeg2ContentPCRLockStatus          = (Mpeg2ContentPCRLockStatus_t)videoContentInfo.PCRLockStatus;
            ProgramMPEG2_obj.vl_ocStbHostMpeg2ContentDecoderPTS             = videoContentInfo.DecoderPTS;
            ProgramMPEG2_obj.vl_ocStbHostMpeg2ContentDiscontinuities        = videoContentInfo.Discontinuities;
            ProgramMPEG2_obj.vl_ocStbHostMpeg2ContentPktErrors              = videoContentInfo.PktErrors;
            ProgramMPEG2_obj.vl_ocStbHostMpeg2ContentPipelineErrors         = videoContentInfo.PipelineErrors;
            ProgramMPEG2_obj.vl_ocStbHostMpeg2ContentDecoderRestarts        = videoContentInfo.DecoderRestarts;
            ProgramMPEG2_obj.vl_ocStbHostMpeg2ContainerFormat               = (OCSTBHOSTMPEG2CONTAINERFORMAT_t)videoContentInfo.containerFormat;
            
            if(replace_table)
            {
                replace_table = false;
                Table_free(table_data);
            }
            ocStbHostMpeg2ContentTable_createEntry_allData(table_data, &ProgramMPEG2_obj, ProgramMPEG2_obj.Mpeg2ContentIndex);
            ++mpgeTableindex;
        }
    }//for loop
    if(0 == mpgeTableindex)
    {
        SNMPocStbHostMpeg2ContentTable_t ProgramMPEG2_obj;
        memset(&ProgramMPEG2_obj, 0, sizeof(ProgramMPEG2_obj));
        ocStbHostMpeg2ContentTable_Default_Intilize(table_data, &ProgramMPEG2_obj);
    }
    return 1;
}

/**
 * @brief This function is used to initialize Mpeg2 table with default data such
 * as Index, Audio/Video PID, CCI Value, APS Value, CIT & other status and fill
 * these default data to Content Table.
 *
 * @param[in] ProgramMPEG2_obj Struct pointer where Mpeg2 Content data is present.
 * @param[out] table_data Table where Mpeg2 Content info is retrieved and stored.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int ocStbHostMpeg2ContentTable_Default_Intilize(netsnmp_tdata * table_data, SNMPocStbHostMpeg2ContentTable_t *ProgramMPEG2_obj)
{

    int mpegtableindex = 1;
    struct ocStbHostMpeg2ContentTable_entry *entry;
    netsnmp_tdata_row *row;

    entry = SNMP_MALLOC_TYPEDEF(struct ocStbHostMpeg2ContentTable_entry);
    if (!entry)
        return 0;

    row = netsnmp_tdata_create_row();
    if (!row) {
        SNMP_FREE(entry);
        return 0;
    }
    row->data = entry;
    entry->ocStbHostMpeg2ContentIndex = mpegtableindex;
    entry->ocStbHostMpeg2ContentProgramNumber = ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentProgramNumber;
    entry->ocStbHostMpeg2ContentTransportStreamID = ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentTransportStreamID;
    entry->ocStbHostMpeg2ContentTotalStreams = ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentTotalStreams;
    entry->ocStbHostMpeg2ContentSelectedVideoPID = ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentSelectedVideoPID;
    entry->ocStbHostMpeg2ContentSelectedAudioPID = ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentSelectedAudioPID;
    entry->ocStbHostMpeg2ContentOtherAudioPIDs = ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentOtherAudioPIDs;
    entry->ocStbHostMpeg2ContentCCIValue = ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentCCIValue;
    entry->ocStbHostMpeg2ContentAPSValue = ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentAPSValue;
    entry->ocStbHostMpeg2ContentCITStatus = ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentCITStatus;
    entry->ocStbHostMpeg2ContentBroadcastFlagStatus = ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentBroadcastFlagStatus;
    entry->ocStbHostMpeg2ContentEPNStatus = ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentEPNStatus;
    entry->ocStbHostMpeg2ContentPCRPID   =  ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentPCRPID;
    entry->ocStbHostMpeg2ContentPCRLockStatus = ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentPCRLockStatus;
    entry->ocStbHostMpeg2ContentDecoderPTS    =ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentDecoderPTS;
    entry->ocStbHostMpeg2ContentDiscontinuities =ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentDiscontinuities;
    entry->ocStbHostMpeg2ContentPktErrors       =  ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentPktErrors;
    entry->ocStbHostMpeg2ContentPipelineErrors  = ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentPipelineErrors;
    entry->ocStbHostMpeg2ContentDecoderRestarts  = ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentDecoderRestarts;
    entry->ocStbHostMpeg2ContainerFormat  = ProgramMPEG2_obj->vl_ocStbHostMpeg2ContainerFormat;

    netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,
                                &(entry->ocStbHostMpeg2ContentIndex),
                                  sizeof(entry->ocStbHostMpeg2ContentIndex));
    netsnmp_tdata_row *temprow;
    if(temprow = netsnmp_tdata_row_first(table_data))
    {

        netsnmp_tdata_replace_row(table_data, temprow, row);
    }
    else
    {
        netsnmp_tdata_add_row(table_data, row);
    }

    return 1;
}

/**
 * @brief This function is used to copy the entry for Mpeg2 Content Table from 
 * from passed input parameter to its content table.
 *
 * @param[in] mpegtableindex Content table index.
 * @param[in] ProgramMPEG2_obj Struct pointer where Mpeg2 Content data is present.
 * @param[out] table_data Table where Mpeg2 Content info is retrieved and stored.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int ocStbHostMpeg2ContentTable_createEntry_allData(netsnmp_tdata * table_data, SNMPocStbHostMpeg2ContentTable_t *ProgramMPEG2_obj, int mpegtableindex)
{

    struct ocStbHostMpeg2ContentTable_entry *entry;
    netsnmp_tdata_row *row;

    entry = SNMP_MALLOC_TYPEDEF(struct ocStbHostMpeg2ContentTable_entry);
    if (!entry)
        return 0;

    row = netsnmp_tdata_create_row();
    if (!row) {
        SNMP_FREE(entry);
        return 0;
    }
    row->data = entry;
    entry->ocStbHostMpeg2ContentIndex = mpegtableindex;
    entry->ocStbHostMpeg2ContentProgramNumber = ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentProgramNumber;
    entry->ocStbHostMpeg2ContentTransportStreamID = ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentTransportStreamID;
    entry->ocStbHostMpeg2ContentTotalStreams = ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentTotalStreams;
    entry->ocStbHostMpeg2ContentSelectedVideoPID = ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentSelectedVideoPID;
    entry->ocStbHostMpeg2ContentSelectedAudioPID = ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentSelectedAudioPID;
    entry->ocStbHostMpeg2ContentOtherAudioPIDs = ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentOtherAudioPIDs;
    entry->ocStbHostMpeg2ContentCCIValue = ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentCCIValue;
    entry->ocStbHostMpeg2ContentAPSValue = ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentAPSValue;
    entry->ocStbHostMpeg2ContentCITStatus = ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentCITStatus;
    entry->ocStbHostMpeg2ContentBroadcastFlagStatus = ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentBroadcastFlagStatus;
    entry->ocStbHostMpeg2ContentEPNStatus = ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentEPNStatus;
    entry->ocStbHostMpeg2ContentPCRPID   =  ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentPCRPID;
    entry->ocStbHostMpeg2ContentPCRLockStatus = ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentPCRLockStatus;
    entry->ocStbHostMpeg2ContentDecoderPTS    =ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentDecoderPTS;
    entry->ocStbHostMpeg2ContentDiscontinuities =ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentDiscontinuities;
    entry->ocStbHostMpeg2ContentPktErrors       =  ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentPktErrors;
    entry->ocStbHostMpeg2ContentPipelineErrors  = ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentPipelineErrors;
    entry->ocStbHostMpeg2ContentDecoderRestarts  = ProgramMPEG2_obj->vl_ocStbHostMpeg2ContentDecoderRestarts;
    entry->ocStbHostMpeg2ContainerFormat  = ProgramMPEG2_obj->vl_ocStbHostMpeg2ContainerFormat;


    netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,
                                &(entry->ocStbHostMpeg2ContentIndex),
                                  sizeof(entry->ocStbHostMpeg2ContentIndex));
    netsnmp_tdata_add_row(table_data, row);
    return 1;
}

SNMPocStbHostMpeg4ContentTable_t ProgramMPEG4_obj;
/**
 * @brief This function is used to get info about mpeg4 content like program number,
 * Audio/Video PID, CCI, APS Value Status etc and using other function call it creates
 * entries for these data in the Mpeg4 Content Table.
 * 
 * @param[out] table_data Table where Mpeg4 Content info is stored. 
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int vl_ocStbHostMpeg4ContentTable_getdata(netsnmp_tdata *table_data)
{
    bool Mpeg4ContentTableUpdatedNow = false;
    bool ProgramStatusTableUpdatedNow = false;
    bool replace_table = false;

    if(netsnmp_tdata_row_first(table_data))
    {
        replace_table = true;
    }

    int nitemreplace = 0;
    int mpgeTableindex = 0;
    int Signal_statsu = 0;
    // Avinterface obj;

    //vlMemSet(vlg_programInfo, 0, sizeof(vlg_programInfo),sizeof(vlg_programInfo));
    memset(vlg_programInfo, 0, sizeof(vlg_programInfo));

    for( int mpegrows = 0; ; mpegrows++)
    {
        struct VideoContentInfo videoContentInfo;
        if(RMF_SUCCESS != IPC_CLIENT_SNMPGetVideoContentInfo(mpegrows, &videoContentInfo))
        {
            break;
        }
        
        if(VideoContainerFormat_mpeg4 == videoContentInfo.containerFormat)
        {
            //ProgramMPEG4_obj.Mpeg4ContentIndex                            = videoContentInfo.ContentIndex; // Order the list based on tuner-id as this stack is incapable of
            //                                                                                               // delivering more than one content from the same tuner.
            ProgramMPEG4_obj.Mpeg4ContentIndex                              = videoContentInfo.TunerIndex+1; // Order the list based on tuner-id as this stack is incapable of
                                                                                                             // delivering more than one content from the same tuner.
            ProgramMPEG4_obj.vl_ocStbHostMpeg4ContentProgramNumber          = videoContentInfo.ProgramNumber;
            ProgramMPEG4_obj.vl_ocStbHostMpeg4ContentTransportStreamID      = videoContentInfo.TransportStreamID;
            ProgramMPEG4_obj.vl_ocStbHostMpeg4ContentTotalStreams           = videoContentInfo.TotalStreams;
            ProgramMPEG4_obj.vl_ocStbHostMpeg4ContentSelectedVideoPID       = videoContentInfo.SelectedVideoPID;
            ProgramMPEG4_obj.vl_ocStbHostMpeg4ContentSelectedAudioPID       = videoContentInfo.SelectedAudioPID;
            ProgramMPEG4_obj.vl_ocStbHostMpeg4ContentOtherAudioPIDs         = (Status_t)videoContentInfo.OtherAudioPIDs;
            ProgramMPEG4_obj.vl_ocStbHostMpeg4ContentCCIValue               = (Mpeg4ContentCCIValue_t)videoContentInfo.CCIValue;
            ProgramMPEG4_obj.vl_ocStbHostMpeg4ContentAPSValue               = (Mpeg4ContentAPSValue_t)videoContentInfo.APSValue;
            ProgramMPEG4_obj.vl_ocStbHostMpeg4ContentCITStatus              = (Status_t)videoContentInfo.CITStatus;
            ProgramMPEG4_obj.vl_ocStbHostMpeg4ContentBroadcastFlagStatus    = (Status_t)videoContentInfo.BroadcastFlagStatus;
            ProgramMPEG4_obj.vl_ocStbHostMpeg4ContentEPNStatus              = (Status_t)videoContentInfo.EPNStatus;
            ProgramMPEG4_obj.vl_ocStbHostMpeg4ContentPCRPID                 = videoContentInfo.PCRPID;
            ProgramMPEG4_obj.vl_ocStbHostMpeg4ContentPCRLockStatus          = (Mpeg4ContentPCRLockStatus_t)videoContentInfo.PCRLockStatus;
            ProgramMPEG4_obj.vl_ocStbHostMpeg4ContentDecoderPTS             = videoContentInfo.DecoderPTS;
            ProgramMPEG4_obj.vl_ocStbHostMpeg4ContentDiscontinuities        = videoContentInfo.Discontinuities;
            ProgramMPEG4_obj.vl_ocStbHostMpeg4ContentPktErrors              = videoContentInfo.PktErrors;
            ProgramMPEG4_obj.vl_ocStbHostMpeg4ContentPipelineErrors         = videoContentInfo.PipelineErrors;
            ProgramMPEG4_obj.vl_ocStbHostMpeg4ContentDecoderRestarts        = videoContentInfo.DecoderRestarts;
            ProgramMPEG4_obj.vl_ocStbHostMpeg4ContainerFormat               = (OCSTBHOSTMPEG4CONTAINERFORMAT_t)videoContentInfo.containerFormat;
            
            if(replace_table)
            {
                replace_table = false;
                Table_free(table_data);
            }
            ocStbHostMpeg4ContentTable_createEntry_allData(table_data, &ProgramMPEG4_obj, ProgramMPEG4_obj.Mpeg4ContentIndex);
            ++mpgeTableindex;
        }
    }//for loop
    if(0 == mpgeTableindex)
    {
        SNMPocStbHostMpeg4ContentTable_t ProgramMPEG4_obj;
        memset(&ProgramMPEG4_obj, 0, sizeof(ProgramMPEG4_obj));
        ocStbHostMpeg4ContentTable_Default_Intilize(table_data, &ProgramMPEG4_obj);
    }
    return 1;
}

/**
 * @brief This function is used to initialize entries of Mpeg4 Content Table like Program Number,
 * Audio/VideoPID, CCI APS Value & Status etc with default values provided by input parameter.
 *
 * @param[in] ProgramMPEG4_obj Struct pointer where Mpeg4 Content Table Default values are present.
 * @param[out] table_data Table where Mpeg4 Content Table entries have to be initialized.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int ocStbHostMpeg4ContentTable_Default_Intilize(netsnmp_tdata * table_data, SNMPocStbHostMpeg4ContentTable_t *ProgramMPEG4_obj)
{

    int mpegtableindex = 1;
    struct ocStbHostMpeg4ContentTable_entry *entry;
    netsnmp_tdata_row *row;

    entry = SNMP_MALLOC_TYPEDEF(struct ocStbHostMpeg4ContentTable_entry);
    if (!entry)
        return 0;

    row = netsnmp_tdata_create_row();
    if (!row) {
        SNMP_FREE(entry);
        return 0;
    }
    row->data = entry;
    entry->ocStbHostMpeg4ContentIndex = mpegtableindex;
    entry->ocStbHostMpeg4ContentProgramNumber = ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentProgramNumber;
    entry->ocStbHostMpeg4ContentTransportStreamID = ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentTransportStreamID;
    entry->ocStbHostMpeg4ContentTotalStreams = ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentTotalStreams;
    entry->ocStbHostMpeg4ContentSelectedVideoPID = ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentSelectedVideoPID;
    entry->ocStbHostMpeg4ContentSelectedAudioPID = ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentSelectedAudioPID;
    entry->ocStbHostMpeg4ContentOtherAudioPIDs = ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentOtherAudioPIDs;
    entry->ocStbHostMpeg4ContentCCIValue = ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentCCIValue;
    entry->ocStbHostMpeg4ContentAPSValue = ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentAPSValue;
    entry->ocStbHostMpeg4ContentCITStatus = ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentCITStatus;
    entry->ocStbHostMpeg4ContentBroadcastFlagStatus = ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentBroadcastFlagStatus;
    entry->ocStbHostMpeg4ContentEPNStatus = ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentEPNStatus;
    entry->ocStbHostMpeg4ContentPCRPID   =  ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentPCRPID;
    entry->ocStbHostMpeg4ContentPCRLockStatus = ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentPCRLockStatus;
    entry->ocStbHostMpeg4ContentDecoderPTS    =ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentDecoderPTS;
    entry->ocStbHostMpeg4ContentDiscontinuities =ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentDiscontinuities;
    entry->ocStbHostMpeg4ContentPktErrors       =  ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentPktErrors;
    entry->ocStbHostMpeg4ContentPipelineErrors  = ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentPipelineErrors;
    entry->ocStbHostMpeg4ContentDecoderRestarts  = ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentDecoderRestarts;
    entry->ocStbHostMpeg4ContainerFormat  = ProgramMPEG4_obj->vl_ocStbHostMpeg4ContainerFormat;

    netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,
                                &(entry->ocStbHostMpeg4ContentIndex),
                                  sizeof(entry->ocStbHostMpeg4ContentIndex));
    netsnmp_tdata_row *temprow;
    if(temprow = netsnmp_tdata_row_first(table_data))
    {

        netsnmp_tdata_replace_row(table_data, temprow, row);
    }
    else
    {
        netsnmp_tdata_add_row(table_data, row);
    }

    return 1;
}

/**
 * @brief This function is used to create entries in Mpeg4 Content Table and copy
 * content from passed input parameter to these entries. 
 *
 * @param[in] mpegtableindex Mpeg4 Content Table index.
 * @param[in] ProgramMPEG4_obj Struct pointer where Content Table entries are present.
 * @param[out] table_data Mpeg4 Content Table where its entries have to be created.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int ocStbHostMpeg4ContentTable_createEntry_allData(netsnmp_tdata * table_data, SNMPocStbHostMpeg4ContentTable_t *ProgramMPEG4_obj, int mpegtableindex)
{

    struct ocStbHostMpeg4ContentTable_entry *entry;
    netsnmp_tdata_row *row;

    entry = SNMP_MALLOC_TYPEDEF(struct ocStbHostMpeg4ContentTable_entry);
    if (!entry)
        return 0;

    row = netsnmp_tdata_create_row();
    if (!row) {
        SNMP_FREE(entry);
        return 0;
    }
    row->data = entry;
    entry->ocStbHostMpeg4ContentIndex = mpegtableindex;
    entry->ocStbHostMpeg4ContentProgramNumber = ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentProgramNumber;
    entry->ocStbHostMpeg4ContentTransportStreamID = ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentTransportStreamID;
    entry->ocStbHostMpeg4ContentTotalStreams = ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentTotalStreams;
    entry->ocStbHostMpeg4ContentSelectedVideoPID = ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentSelectedVideoPID;
    entry->ocStbHostMpeg4ContentSelectedAudioPID = ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentSelectedAudioPID;
    entry->ocStbHostMpeg4ContentOtherAudioPIDs = ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentOtherAudioPIDs;
    entry->ocStbHostMpeg4ContentCCIValue = ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentCCIValue;
    entry->ocStbHostMpeg4ContentAPSValue = ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentAPSValue;
    entry->ocStbHostMpeg4ContentCITStatus = ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentCITStatus;
    entry->ocStbHostMpeg4ContentBroadcastFlagStatus = ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentBroadcastFlagStatus;
    entry->ocStbHostMpeg4ContentEPNStatus = ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentEPNStatus;
    entry->ocStbHostMpeg4ContentPCRPID   =  ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentPCRPID;
    entry->ocStbHostMpeg4ContentPCRLockStatus = ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentPCRLockStatus;
    entry->ocStbHostMpeg4ContentDecoderPTS    =ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentDecoderPTS;
    entry->ocStbHostMpeg4ContentDiscontinuities =ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentDiscontinuities;
    entry->ocStbHostMpeg4ContentPktErrors       =  ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentPktErrors;
    entry->ocStbHostMpeg4ContentPipelineErrors  = ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentPipelineErrors;
    entry->ocStbHostMpeg4ContentDecoderRestarts  = ProgramMPEG4_obj->vl_ocStbHostMpeg4ContentDecoderRestarts;
    entry->ocStbHostMpeg4ContainerFormat  = ProgramMPEG4_obj->vl_ocStbHostMpeg4ContainerFormat;


    netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,
                                &(entry->ocStbHostMpeg4ContentIndex),
                                  sizeof(entry->ocStbHostMpeg4ContentIndex));
    netsnmp_tdata_add_row(table_data, row);
    return 1;
}

SNMPocStbHostVc1ContentTable_t ProgramVC1_obj;
/**
 * @brief This function is used to to get info about VC1 content like program number,
 * content format etc from input parameter and using another function call it creates
 * entries for these data in Vc1 Content Table. 
 *
 * @param[out] table_data Table where entries for Vc1 content is retrieved.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int vl_ocStbHostVc1ContentTable_getdata(netsnmp_tdata *table_data)
{
    bool Vc1ContentTableUpdatedNow = false;
    bool ProgramStatusTableUpdatedNow = false;
    bool replace_table = false;

    if(netsnmp_tdata_row_first(table_data))
    {
        replace_table = true;
    }

    int nitemreplace = 0;
    int mpgeTableindex = 0;
    int Signal_statsu = 0;
    // Avinterface obj;

    //vlMemSet(vlg_programInfo, 0, sizeof(vlg_programInfo),sizeof(vlg_programInfo));
    memset(vlg_programInfo, 0, sizeof(vlg_programInfo));

    for( int mpegrows = 0; ; mpegrows++)
    {
        struct VideoContentInfo videoContentInfo;
        if(RMF_SUCCESS != IPC_CLIENT_SNMPGetVideoContentInfo(mpegrows, &videoContentInfo))
        {
            break;
        }
        
        if(VideoContainerFormat_vc1 == videoContentInfo.containerFormat)
        {
            //ProgramVC1_obj.Vc1ContentIndex                            = videoContentInfo.ContentIndex; // Order the list based on tuner-id as this stack is incapable of
            //                                                                                               // delivering more than one content from the same tuner.
            ProgramVC1_obj.Vc1ContentIndex                              = videoContentInfo.TunerIndex+1; // Order the list based on tuner-id as this stack is incapable of
                                                                                                             // delivering more than one content from the same tuner.
            ProgramVC1_obj.vl_ocStbHostVc1ContentProgramNumber          = videoContentInfo.ProgramNumber;
            ProgramVC1_obj.vl_ocStbHostVc1ContainerFormat               = (OCSTBHOSTVC1CONTAINERFORMAT_t)videoContentInfo.containerFormat;
            
            if(replace_table)
            {
                replace_table = false;
                Table_free(table_data);
            }
            ocStbHostVc1ContentTable_createEntry_allData(table_data, &ProgramVC1_obj, ProgramVC1_obj.Vc1ContentIndex);
            ++mpgeTableindex;
        }
    }//for loop
    if(0 == mpgeTableindex)
    {
        SNMPocStbHostVc1ContentTable_t ProgramVC1_obj;
        memset(&ProgramVC1_obj, 0, sizeof(ProgramVC1_obj));
        ocStbHostVc1ContentTable_Default_Intilize(table_data, &ProgramVC1_obj);
    }
    return 1;
}

/**
 * @brief This function is used to initialize entries of VC1 Content Table like Program Number,
 * formats etc with default values provided by input parameter.
 * 
 * @param[in] ProgramVC1_obj Struct pointer where VC1 entry default data is present.
 * @param[out] table_data Table where VC1 content info is initialized with default values.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int ocStbHostVc1ContentTable_Default_Intilize(netsnmp_tdata * table_data, SNMPocStbHostVc1ContentTable_t *ProgramVC1_obj)
{

    int mpegtableindex = 1;
    struct ocStbHostVc1ContentTable_entry *entry;
    netsnmp_tdata_row *row;

    entry = SNMP_MALLOC_TYPEDEF(struct ocStbHostVc1ContentTable_entry);
    if (!entry)
        return 0;

    row = netsnmp_tdata_create_row();
    if (!row) {
        SNMP_FREE(entry);
        return 0;
    }
    row->data = entry;
    entry->ocStbHostVc1ContentIndex = mpegtableindex;
    entry->ocStbHostVc1ContentProgramNumber = ProgramVC1_obj->vl_ocStbHostVc1ContentProgramNumber;
    entry->ocStbHostVc1ContainerFormat  = ProgramVC1_obj->vl_ocStbHostVc1ContainerFormat;

    netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,
                                &(entry->ocStbHostVc1ContentIndex),
                                  sizeof(entry->ocStbHostVc1ContentIndex));
    netsnmp_tdata_row *temprow;
    if(temprow = netsnmp_tdata_row_first(table_data))
    {

        netsnmp_tdata_replace_row(table_data, temprow, row);
    }
    else
    {
        netsnmp_tdata_add_row(table_data, row);
    }

    return 1;
}

/**
 * @brief This function is used to create the entry in Vc1 Content Table and set the
 * values from passed input parameter.
 *
 * @param[in] mpegtableindex Vc1 content table index.
 * @param[in] ProgramVC1_obj Struct pointer where Vc1 content table entries data is present.
 * @param[out] table_data Table where entries for Vc1 is getting created.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int ocStbHostVc1ContentTable_createEntry_allData(netsnmp_tdata * table_data, SNMPocStbHostVc1ContentTable_t *ProgramVC1_obj, int mpegtableindex)
{

    struct ocStbHostVc1ContentTable_entry *entry;
    netsnmp_tdata_row *row;

    entry = SNMP_MALLOC_TYPEDEF(struct ocStbHostVc1ContentTable_entry);
    if (!entry)
        return 0;

    row = netsnmp_tdata_create_row();
    if (!row) {
        SNMP_FREE(entry);
        return 0;
    }
    row->data = entry;
    entry->ocStbHostVc1ContentIndex = mpegtableindex;
    entry->ocStbHostVc1ContentProgramNumber = ProgramVC1_obj->vl_ocStbHostVc1ContentProgramNumber;
    entry->ocStbHostVc1ContainerFormat  = ProgramVC1_obj->vl_ocStbHostVc1ContainerFormat;


    netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,
                                &(entry->ocStbHostVc1ContentIndex),
                                  sizeof(entry->ocStbHostVc1ContentIndex));
    netsnmp_tdata_add_row(table_data, row);
    return 1;
}

SNMPocStbHostSPDIfTable_t SPDIfTable_obj;
/**
 * @brief This function is used to get the values of SPDIF entries like program number,
 * Audio format, mute status etc and update to SPDIf Table.
 *
 * @param[out] table_data Table where values for SPDIF entries are updated.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int vl_ocStbHostSPDIfTable_getdata(netsnmp_tdata *table_data)
{

    bool SPDIfTableTableUpdatedNow = false;

    bool replace_table = false;

    if(netsnmp_tdata_row_first(table_data))
    {
        replace_table = true;
    }
    int ixAvIntTable;
    u_long SPDIfTableAvInterfaceIx ;

    for(ixAvIntTable = 0; ixAvIntTable < vlg_number_rows; ixAvIntTable++)
    {
     //  SNMP_DEBUGPRINT("\nvlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceType = Ox%x \n",vlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceType);
    //   SNMP_DEBUGPRINT("\nvlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.dispResData.DisplayPortType=%d\n",vlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.dispResData.DisplayPortType);

    //SNMP_DEBUGPRINT("\nxocStbHostHdmiOut 0x%x \t xAV_INTERFACE_TYPE_HDMI 0x% \n",ocStbHostHdmiOut,AV_INTERFACE_TYPE_HDMI);
        if( ( (vlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceType == ocStbHostToslinkSpdifOut ) || (vlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceType == ocStbHostRcaSpdifOut )) &&   (vlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.dispResData.DisplayPortType == AV_INTERFACE_TYPE_SPDIF ))
        {

            SPDIfTableAvInterfaceIx = ixAvIntTable + 1; // AVInterface Table Index starts at 1
            SNMP_DEBUGPRINT("\n ixAvIntTable :: %d \t SPDIfTableAvInterfaceIx:: %d \n", ixAvIntTable, SPDIfTableAvInterfaceIx);

         
            //vlMemSet(&SPDIfTable_obj, 0, sizeof(SPDIfTable_obj), sizeof(SPDIfTable_obj));
            memset(&SPDIfTable_obj, 0, sizeof(SPDIfTable_obj));
         //Default settings when the call fails
            SPDIfTable_obj.vl_ocStbHostSPDIfAudioFormat = OCSTBHOSTSPDIFAUDIOFORMAT_OTHER;
            SPDIfTable_obj.vl_ocStbHostSPDIfAudioMuteStatus = STATUS_FALSE;
            if(0 == avobj.vlGet_ocStbHostSPDIfTableData(&SPDIfTable_obj,  SPDIfTableTableUpdatedNow, vlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.dispResData.hDisplay,vlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.dispResData.DisplayPortType ))
            {
                SNMP_DEBUGPRINT("ERROR:: vlGet_ocStbHostSPDIfTableData");
                return 0;
            }
            if(replace_table)
            {
                replace_table = false;
                Table_free(table_data);
            }

            ocStbHostSPDIfTable_createEntry_allData(table_data, &SPDIfTable_obj,  SPDIfTableAvInterfaceIx);


        //free(ComponentVideoTable_obj);
        } //if check'a and call's the moudle

    }//for loop
    return 1;
}

/**
 * @brief This function is used to create the entries for SPDIF Table and initialize
 * it from passed input parameter.
 *
 * @param[in] spdeftableindex Index for SPDIF content table.
 * @param[in] SPDIfTable_obj Struct pointer where SPDIF data is present.
 * @param[out] table_data Table where entries for SPDIF content is created.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int ocStbHostSPDIfTable_createEntry_allData(netsnmp_tdata * table_data, SNMPocStbHostSPDIfTable_t *SPDIfTable_obj, int spdeftableindex)
{
    struct ocStbHostSPDIfTable_entry *entry;
    SNMP_DEBUGPRINT("\n ocStbHostSPDIfTable_createEntry_allData \n");
    netsnmp_tdata_row *row;
    entry = SNMP_MALLOC_TYPEDEF(struct ocStbHostSPDIfTable_entry);
    if (!entry)
        return 0;

    row = netsnmp_tdata_create_row();
    if (!row) {
        SNMP_FREE(entry);
        return 0;
    }
    row->data = entry;
    entry->ocStbHostAVInterfaceIndex = spdeftableindex;
    entry->ocStbHostSPDIfAudioFormat = SPDIfTable_obj->vl_ocStbHostSPDIfAudioFormat;
    entry->ocStbHostSPDIfAudioMuteStatus = SPDIfTable_obj->vl_ocStbHostSPDIfAudioMuteStatus;

    netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,
                                &(entry->ocStbHostAVInterfaceIndex),
                                  sizeof(entry->ocStbHostAVInterfaceIndex));
    netsnmp_tdata_add_row(table_data, row);
    return 1;
}

/**
 * @brief This function is used to initialize the entries of SPDIF table like program num,
 * format etc with the default values received as an input parameter.
 * ( Currently this function is not required)
 * 
 * @param[in] SPDIfTable_obj Struct pointer where default values for entries of SPDIF table is present.
 * @param[out] table_data Table where entries of SPDIF table is initialized.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int ocStbHostSPDIfTable_Default_Intilize(netsnmp_tdata * table_data, SNMPocStbHostSPDIfTable_t *SPDIfTable_obj)
{

//present not required
    return 1;
}

IEEE1394Table_t Ieee1394obj;
/**
 * @brief This function is used to get the values of IEEE1394 entries like AV Interface type, 
 * Active Nodes, DTCP, Loop, Audio/Video Mute Status etc and update to IEEE1394 Table.
 *
 * @param[out] table_data Table where values for IEEE1394 entries are updated.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int vl_ocStbHostIEEE1394Table_getdata(netsnmp_tdata * table_data)
{

    u_long IEEE1394AvInterfaceIx = 0;
    int ixAvIntTable = 0;
    int ntimereplace = 0;
    bool IEEE1394TableUpdatedNow = false;
    bool replace_table = false;

    if( netsnmp_tdata_row_first(table_data))
    {
        replace_table = true;
    }

  
    //vlMemSet(&Ieee1394obj, 0, sizeof(IEEE1394Table_t), sizeof(IEEE1394Table_t));
    memset(&Ieee1394obj, 0, sizeof(IEEE1394Table_t));
#ifdef USE_1394
        if(vl_isFeatureEnabled((unsigned char *)vlg_szMpeosFeature1394))
        {
        SNMP_DEBUGPRINT("\n vl_ocStbHostIEEE1394Table_getdata::initialize_table_ocStbHostIEEE1394Table \n");
        for (ixAvIntTable = 0; ixAvIntTable < vlg_number_rows; ixAvIntTable++)
        {

            //SNMP_DEBUGPRINT("vlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceType = %d \n",vlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceType);
        //  SNMP_DEBUGPRINT("vlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceIndex = %d",
        //     vlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.tunerIndex);

        //enable or disable makes same routine
            if(vlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceType == ocStbHost1394Out /*&& vlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceStatus == OCSTBHOSTAVINTERFACESTATUS_ENABLED*/)
            {

            IEEE1394AvInterfaceIx = ixAvIntTable + 1; // AVInterface Table Index
            //ptrAVInterfaceData[iNewRes].resourceIdInfo.Ieee1394port

            if(0 == avobj.vlGet_ocStbHostIEEE1394TableData(&Ieee1394obj))
            {
                SNMP_DEBUGPRINT("ERROR:: avobj.vlGet_ocStbHostIEEE1394TableData");

            }
            SNMP_DEBUGPRINT("\n intilization ocStbHostIEEE1394TableData \n");
            if(replace_table)
            {
                replace_table = false;
                Table_free(table_data);
            }
            RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "vlGet_ocStbHostIEEE1394TableData::  %d Ieee1394obj  portNum = %d\n numNodes = %d\n isXmtngData = %d\n dtcpStatus = %d\n usInLoop = %d \nisRoot = %d \nisCycleMaster = %d \nisIRM = %d \n ",__FUNCTION__,__LINE__, Ieee1394obj.vl_ocStbHostIEEE1394ActiveNodes, Ieee1394obj.vl_ocStbHostIEEE1394DataXMission, Ieee1394obj.vl_ocStbHostIEEE1394DTCPStatus, Ieee1394obj.vl_ocStbHostIEEE1394LoopStatus, Ieee1394obj.vl_ocStbHostIEEE1394RootStatus, Ieee1394obj.vl_ocStbHostIEEE1394CycleIsMaster, Ieee1394obj.vl_ocStbHostIEEE1394IRMStatus);
            ocStbHostIEEE1394Table_createEntry_allData(table_data, IEEE1394AvInterfaceIx, &Ieee1394obj);

            //free(InBandTunerTable_obj);
            } //if checks the entry for IEEE1394
        }//for loop
        //Default Intilization
    }
#endif // USE_1394
    return 1;
}

/**
 * @brief This function is used to create the entries for IEEE1394 and update with
 * values recieved from passed input parameter.
 *
 * @param[in] IEEE1394AvInterfaceIx Index for IEEE1394 content table.
 * @param[in] IEEE1394Table_obj Struct pointer where IEEE1394 data is present for entries.
 * @param[out] table_data Table where entries for IEEE1394 content is created and updated.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int ocStbHostIEEE1394Table_createEntry_allData(netsnmp_tdata * table_data, int IEEE1394AvInterfaceIx, IEEE1394Table_t *IEEE1394Table_obj)
{

    struct ocStbHostIEEE1394Table_entry *entry;
    netsnmp_tdata_row *row;
    entry = SNMP_MALLOC_TYPEDEF(struct ocStbHostIEEE1394Table_entry);
    if (!entry)
        return 0;

    row = netsnmp_tdata_create_row();
    if (!row) {
        SNMP_FREE(entry);
        return 0;
    }
    row->data = entry;
    entry->ocStbHostAVInterfaceIndex = IEEE1394AvInterfaceIx;

    entry->ocStbHostIEEE1394ActiveNodes=IEEE1394Table_obj->vl_ocStbHostIEEE1394ActiveNodes;
    entry->ocStbHostIEEE1394DataXMission=IEEE1394Table_obj->vl_ocStbHostIEEE1394DataXMission;
    entry->ocStbHostIEEE1394DTCPStatus=IEEE1394Table_obj->vl_ocStbHostIEEE1394DTCPStatus;
    entry->ocStbHostIEEE1394LoopStatus=IEEE1394Table_obj->vl_ocStbHostIEEE1394LoopStatus;
    entry->ocStbHostIEEE1394RootStatus=IEEE1394Table_obj->vl_ocStbHostIEEE1394RootStatus;
    entry->ocStbHostIEEE1394CycleIsMaster=IEEE1394Table_obj->vl_ocStbHostIEEE1394CycleIsMaster;
    entry->ocStbHostIEEE1394IRMStatus=IEEE1394Table_obj->vl_ocStbHostIEEE1394IRMStatus;
    entry->ocStbHostIEEE1394AudioMuteStatus=IEEE1394Table_obj->vl_ocStbHostIEEE1394AudioMuteStatus;
    entry->ocStbHostIEEE1394VideoMuteStatus=IEEE1394Table_obj->vl_ocStbHostIEEE1394VideoMuteStatus;

    netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,
                                &(entry->ocStbHostAVInterfaceIndex),
                                  sizeof(entry->ocStbHostAVInterfaceIndex));
    netsnmp_tdata_add_row(table_data, row);
    return 1;
}

/**
 * @brief This function is used to initialize the entries of IEEE1394 table like
 * Active Nodes, DTCP, Loop, Audio/Video Mute Status etc with the default values
 * received from passed input parameter.
 * (Currently this function is not needed)
 *
 * @param[in] IEEE1394Table_obj Struct pointer where default values for entries of IEEE1394 table is present.
 * @param[out] table_data Table where entries of SPDIF table is initialized.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int ocStbHostIEEE1394Table_Default_Intilize(netsnmp_tdata * table_data, IEEE1394Table_t *IEEE1394Table_obj)
{
//    if enable and disable status comes make default init
   // NOT need
    return 1;
}

#define PORT_ZERO 0
#define PORT_ONE  1
#define MAX_1394CONNECTDEVICE 64
IEEE1394ConnectedDevices_t Ieee1394CDObj[MAX_1394CONNECTDEVICE];
IEEE1394ConnectedDevices_t Ieee1394CDObjport1;

/**
 * @brief This function is used to get the values of entries for IEEE1394 connected device
 * like AV Interface, Sub-unit Type, Devices Euic etc and update to IEEE1394ConnectedDevices Table.
 *
 * @param[out] table_data Table where values of entries for connected IEEE1394 devices are updated.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int vl_ocStbHostIEEE1394ConnectedDevicesTable_getdata(netsnmp_tdata * table_data)
{

    u_long IEEE1394ConnectedDevicesAvInterfaceIx = 0;
    u_long IEEE1394ConnectedDevicesIndex = 0;
    int ixActivenodes = 0;
    int nitemreplace = 0;
    int numberofnodes = 0;
    int NumberofDvConnected = 0;
    int port2_connID = 0;
    int connID = 0;
    int numberofTotalDevices = 0;
    bool IEEE1394ConnectedDevicesUpdatedNow = false;
    bool replace_table = false;

    if(netsnmp_tdata_row_first(table_data))
    {
        replace_table = true;
    }

#ifdef USE_1394
        if(vl_isFeatureEnabled((unsigned char *)vlg_szMpeosFeature1394))
        {
        for (int ixAvIntTable = 0; ixAvIntTable < vlg_number_rows; ixAvIntTable++)
        {

        // SNMP_DEBUGPRINT("vlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceType = %d \n",vlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceType);
        //  SNMP_DEBUGPRINT("vlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceIndex = %d",
        //     vlg_AVInterfaceTable[ixAvIntTable].resourceIdInfo.tunerIndex);


            if(vlg_AVInterfaceTable[ixAvIntTable].vl_ocStbHostAVInterfaceType == ocStbHost1394Out )
            {
            IEEE1394ConnectedDevicesAvInterfaceIx = ixAvIntTable + 1; // AVInterface Table Index

                numberofTotalDevices =0;
                //vlMemSet(Ieee1394CDObj, 0, sizeof(Ieee1394CDObj), sizeof(Ieee1394CDObj));
                vlMemSet(Ieee1394CDObj, 0, sizeof(Ieee1394CDObj));
                if(0 == avobj.vlGet_ocStbHostIEEE1394ConnectedDevicesTableData(Ieee1394CDObj, &NumberofDvConnected))
                {
                SNMP_DEBUGPRINT("ERROR:: avobj.vlGet_ocStbHostIEEE1394ConnectedDevicesTableData");
                NumberofDvConnected = 0;
                }
                /*Default Intilization will be don if no active nodes are there*/
                SNMP_DEBUGPRINT("\n vl_ocStbHostIEEE1394ConnectedDevicesTable_getdata :: Pnumber of Device connected   pNumDevices= %d \n", NumberofDvConnected);
                if(NumberofDvConnected == 0)
                {
                //fill the table with zeros as it is default intilization

        
                //vlMemSet(&Ieee1394CDObjport1, 0,sizeof(Ieee1394CDObjport1),sizeof(Ieee1394CDObjport1));
                memset(&Ieee1394CDObjport1, 0,sizeof(Ieee1394CDObjport1));

                if(replace_table)
                {
        
                    replace_table = false;
                    Table_free(table_data);
                }

                Ieee1394CDObjport1.vl_ocStbHostIEEE1394ConnectedDevicesSubUnitType =   SUBUNITTYPE_OTHER;
                vlMemCpy(Ieee1394CDObjport1.vl_ocStbHostIEEE1394ConnectedDevicesEui64, "00:00:00:00:00:00:00:00",strlen("00:00:00:00:00:00:00:00") , CHRMAX);
                Ieee1394CDObjport1.vl_ocStbHostIEEE1394ConnectedDevicesADSourceSelectSupport = ADSOURCESELECTSUPPORT_FALSE;

                ocStbHostIEEE1394ConnectedDevicesTable_createEntry_allData(table_data, &Ieee1394CDObjport1, IEEE1394ConnectedDevicesAvInterfaceIx, 0);
                numberofTotalDevices++;

                }
                else
                {
                for(connID = 0; connID < NumberofDvConnected; connID++)
                {


                    if(replace_table)
                    {

                    replace_table = false;
                    Table_free(table_data);
                    }

                    ocStbHostIEEE1394ConnectedDevicesTable_createEntry_allData(table_data, &Ieee1394CDObj[connID], IEEE1394ConnectedDevicesAvInterfaceIx, connID);
                    numberofTotalDevices++;
            //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.SNMP", "in port 1 detail filling %d =  numberofTotalDevices = %d \n", connID, numberofTotalDevices);
                }//for loop checks the number of connected devices

                }

            }//avinterface check

        } //for loop
    }
#endif // USE_1394
   return 1;
}

/**
 * @brief This function is used to copy values of entries from passed input parameter
 * to the IEEE1394 Connected Devices Table.
 *
 * @param[in] Ieeetableindex Index for entries in Ieee table.
 * @param[in] ConnectedIndex Index for the connected device.
 * @param[in] IEEE1394CDTable_obj Struct pointer where values of entries for IEEE connected 
 * devices are present.
 * @param[out] table_data Table where Ieee device entry info is updated.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int ocStbHostIEEE1394ConnectedDevicesTable_createEntry_allData(netsnmp_tdata * table_data, IEEE1394ConnectedDevices_t *IEEE1394CDTable_obj, int Ieeetableindex, int ConnectedIndex )
{

    struct ocStbHostIEEE1394ConnectedDevicesTable_entry *entry;
    netsnmp_tdata_row *row;
    entry = SNMP_MALLOC_TYPEDEF(struct ocStbHostIEEE1394ConnectedDevicesTable_entry);
    if (!entry)
        return 0;

    row = netsnmp_tdata_create_row();
    if (!row) {
        SNMP_FREE(entry);
        return 0;
    }
    row->data = entry;
    entry->ocStbHostIEEE1394ConnectedDevicesIndex = ConnectedIndex + 1;
    entry->ocStbHostIEEE1394ConnectedDevicesAVInterfaceIndex= Ieeetableindex;
    entry->ocStbHostIEEE1394ConnectedDevicesSubUnitType=IEEE1394CDTable_obj->vl_ocStbHostIEEE1394ConnectedDevicesSubUnitType;
    entry->ocStbHostIEEE1394ConnectedDevicesEui64_len = strlen(IEEE1394CDTable_obj->vl_ocStbHostIEEE1394ConnectedDevicesEui64);

    vlMemCpy(entry->ocStbHostIEEE1394ConnectedDevicesEui64, IEEE1394CDTable_obj->vl_ocStbHostIEEE1394ConnectedDevicesEui64, entry->ocStbHostIEEE1394ConnectedDevicesEui64_len, CHRMAX);

    entry->ocStbHostIEEE1394ConnectedDevicesADSourceSelectSupport =
            IEEE1394CDTable_obj->vl_ocStbHostIEEE1394ConnectedDevicesADSourceSelectSupport;


    netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,
                                &(entry->ocStbHostIEEE1394ConnectedDevicesIndex),
                                    sizeof(entry->ocStbHostIEEE1394ConnectedDevicesIndex));
    netsnmp_tdata_add_row(table_data, row);
    //SNMP_FREE(entry);
    return 1;
}
#if 0
        int ocStbHostIEEE1394ConnectedDevicesTable_Default_Intilize(netsnmp_tdata * table_data, IEEE1394ConnectedDevices_t *IEEE1394CDTable_obj)
        {

        struct ocStbHostIEEE1394ConnectedDevicesTable_entry *entry;
        netsnmp_tdata_row *row;
        entry = SNMP_MALLOC_TYPEDEF(struct ocStbHostIEEE1394ConnectedDevicesTable_entry);
        if (!entry)
        return 0;

        row = netsnmp_tdata_create_row();
        if (!row) {
        SNMP_FREE(entry);
        return 0;
    }
        row->data = entry;
        entry->ocStbHostIEEE1394ConnectedDevicesIndex = 0;
        entry->ocStbHostIEEE1394ConnectedDevicesAVInterfaceIndex= 0;
        entry->ocStbHostIEEE1394ConnectedDevicesSubUnitType=IEEE1394CDTable_obj->vl_ocStbHostIEEE1394ConnectedDevicesSubUnitType;
        entry->ocStbHostIEEE1394ConnectedDevicesEui64_len = strlen(IEEE1394CDTable_obj->vl_ocStbHostIEEE1394ConnectedDevicesEui64);
        vlMemCpy(entry->ocStbHostIEEE1394ConnectedDevicesEui64, IEEE1394CDTable_obj->vl_ocStbHostIEEE1394ConnectedDevicesEui64, entry->ocStbHostIEEE1394ConnectedDevicesEui64_len , CHRMAX);

        entry->ocStbHostIEEE1394ConnectedDevicesADSourceSelectSupport =
        IEEE1394CDTable_obj->vl_ocStbHostIEEE1394ConnectedDevicesADSourceSelectSupport;


        netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,
        &(entry->ocStbHostIEEE1394ConnectedDevicesIndex),
        sizeof(entry->ocStbHostIEEE1394ConnectedDevicesIndex));

    /*netsnmp_tdata_row *temprow;
        if(temprow = netsnmp_tdata_row_first(table_data))
        {
        netsnmp_tdata_replace_row(table_data, temprow, row);
    }
        else{*/
        netsnmp_tdata_add_row(table_data, row);
     //}
        return 1;
    }

#endif //if 0

SNMPocStbHostSystemTempTable_t sysTemperature[MAX_ROWS];
/**
 * @brief This function is used to get the values of entries for System Temperature Table
 * like Description len, Last Update, Max Value etc and update to SystemTempTable Table.
 *
 * @param[out] table_data Table where values of entries for System Temperature are updated.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int vl_ocStbHostSystemTempTable_getdata(netsnmp_tdata * table_data)
{
    bool replace_table = false;
    int nitems = 0;
    int nitemreplace = 0;

    if(netsnmp_tdata_row_first(table_data))
    {
        replace_table = true;
    }

    //vlMemSet(sysTemperature, 0 , sizeof(sysTemperature), sizeof(sysTemperature));
    memset(sysTemperature, 0 , sizeof(sysTemperature));

//    if( 0 == avobj.vlGet_ocStbHostSystemTempTableData(sysTemperature, &nitems));
    avobj.vlGet_ocStbHostSystemTempTableData(sysTemperature, &nitems);
#if 0
    {
        SNMP_DEBUGPRINT("ERROR ::vlGet_ocStbHostCCAppInfoTableData");
    // return 0;
    }
#endif

    if(nitems != 0)
    {
        for(int ntemp = 0; ntemp < nitems; ntemp++)
        {
            if(replace_table)
            {
                replace_table = false;
                Table_free(table_data);
            }

            ocStbHostSystemTempTable_createEntry_allData(table_data, &sysTemperature[ntemp], ntemp);


        }//for loop for the nitems

    }
    else
    {
        /** Default Intilization is don if no entries are there , this is for presen's of Table */
        Table_free(table_data);

        SNMPocStbHostSystemTempTable_t sysTemperature;
        //vlMemSet(&sysTemperature, 0 , sizeof(sysTemperature), sizeof(sysTemperature));
        memset(&sysTemperature, 0 , sizeof(sysTemperature));
        ocStbHostSystemTempTable_createEntry_allData(table_data,&sysTemperature, 0);
    }

    return 1;
}

/**
 * @brief This function is used to update entries for System Temperature Table
 * from the the values passed as an argument.
 *
 * @param[in] SystemTempTableindex Index of entry of System Temperature Table.
 * @param[in] SystemTempTableInfolist Struct pointer where values of entries of system temperature are present.
 * @param[out] table_data Table where system temperature entries are updated.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
int ocStbHostSystemTempTable_createEntry_allData(netsnmp_tdata * table_data, SNMPocStbHostSystemTempTable_t *SystemTempTableInfolist, int SystemTempTableindex)
{
    struct ocStbHostSystemTempTable_entry *entry;
    netsnmp_tdata_row *row;
    entry = SNMP_MALLOC_TYPEDEF(struct ocStbHostSystemTempTable_entry);
    if (!entry)
        return 0;

    row = netsnmp_tdata_create_row();
    if (!row) {
        SNMP_FREE(entry);
        return 0;
    }
    row->data = entry;
    entry->hrDeviceIndex = 1;//hrdevice pre implemented
    entry->ocStbHostSystemTempIndex = SystemTempTableindex+1; //for index purpose
    entry->ocStbHostSystemTempDescr_len = SystemTempTableInfolist->vl_ocStbHostSystemTempDescr_len;
    vlMemCpy(entry->ocStbHostSystemTempDescr, SystemTempTableInfolist->vl_ocStbHostSystemTempDescr, entry->ocStbHostSystemTempDescr_len, CHRMAX);
    entry->ocStbHostSystemTempValue = SystemTempTableInfolist->vl_ocStbHostSystemTempValue;
    entry->ocStbHostSystemTempLastUpdate = SystemTempTableInfolist->vl_ocStbHostSystemTempLastUpdate;
    entry->ocStbHostSystemTempMaxValue = SystemTempTableInfolist->vl_ocStbHostSystemTempMaxValue;
    netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,
                                &(entry->hrDeviceIndex),
                                    sizeof(entry->hrDeviceIndex));
    netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,
                                &(entry->ocStbHostSystemTempIndex),
                                    sizeof(entry->ocStbHostSystemTempIndex));
    netsnmp_tdata_add_row(table_data, row);
    return 1;

}

    SNMPocStbHostSystemHomeNetworkTable_t syshomeNW[MAX_ROWS];
/**
 * @brief This function is used to get the values of the entries for Network table
 * like max num of Network Clients, DRM Status, Client Mac Address, ClientIpAddress etc.,
 * and create and fill these entries.
 *
 * @param[out] table_data Table where values of entries for Home Network Table are updated.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
    int vl_ocStbHostSystemHomeNetworkTable_getdata(netsnmp_tdata * table_data)
    {
        bool replace_table = false;
        int nclients = 0;
        int nitemreplace = 0;

        if(netsnmp_tdata_row_first(table_data))
        {
            replace_table = true;
        }
    
        //vlMemSet(syshomeNW, 0 , sizeof(syshomeNW),sizeof(syshomeNW));
        memset(syshomeNW, 0 , sizeof(syshomeNW));
//        if( 0 == avobj.vlGet_ocStbHostSystemHomeNetworkTableData(syshomeNW,  &nclients));
        avobj.vlGet_ocStbHostSystemHomeNetworkTableData(syshomeNW,  &nclients);
#if 0
        {
            SNMP_DEBUGPRINT("ERROR ::vlGet_ocStbHostSystemHomeNetworkTableData ");

        }
#endif
        if(nclients != 0)
        {
            for(int ncl = 0; ncl <nclients; ncl++)
            {
                if(replace_table)
                {
                    replace_table = false;
                    Table_free(table_data);
                }

                ocStbHostSystemHomeNetworkTable_createEntry_allData(table_data,&syshomeNW[ncl], ncl);


            }

        }
        else
        {
            /** Default Intilization is don if no entries are there , this is for presen's of Table */
            Table_free(table_data);

            SNMPocStbHostSystemHomeNetworkTable_t syshomeNW;
            //vlMemSet(&syshomeNW, 0 , sizeof(syshomeNW), sizeof(syshomeNW));
            memset(&syshomeNW, 0 , sizeof(syshomeNW));
            ocStbHostSystemHomeNetworkTable_createEntry_allData(table_data,&syshomeNW, 0);
        }
        return 1;
    }

/**
 * @brief This function is used to copy values of entries from the passed input
 * parameter to the Network Table.
 *
 * @param[in] HomeNetworkindex Index for Home Network.
 * @param[in] HomeNetworkInfolist Struct pointer where info about Home network is present.
 * @param[out] table_data Table where Home Network entries are updated.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
        int ocStbHostSystemHomeNetworkTable_createEntry_allData(netsnmp_tdata * table_data, SNMPocStbHostSystemHomeNetworkTable_t *HomeNetworkInfolist, int HomeNetworkindex)
        {

            struct ocStbHostSystemHomeNetworkTable_entry *entry;
            netsnmp_tdata_row *row;
            entry = SNMP_MALLOC_TYPEDEF(struct ocStbHostSystemHomeNetworkTable_entry);
            if (!entry)
                return 0;

            row = netsnmp_tdata_create_row();
            if (!row) {
                SNMP_FREE(entry);
                return 0;
            }
            row->data = entry;
            entry->ocStbHostSystemHomeNetworkIndex = HomeNetworkindex +1; //for index purpose
            entry->ocStbHostSystemHomeNetworkMaxClients = HomeNetworkInfolist->vl_ocStbHostSystemHomeNetworkMaxClients;
            entry->ocStbHostSystemHomeNetworkHostDRMStatus =
                    HomeNetworkInfolist->vl_ocStbHostSystemHomeNetworkHostDRMStatus;
            entry->ocStbHostSystemHomeNetworkConnectedClients =
                    HomeNetworkInfolist->vl_ocStbHostSystemHomeNetworkConnectedClients;

            vlMemCpy( entry->ocStbHostSystemHomeNetworkClientMacAddress,
                      HomeNetworkInfolist->vl_ocStbHostSystemHomeNetworkClientMacAddress,  MAC_MAX_SIZE, CHRMAX );

            entry->ocStbHostSystemHomeNetworkClientMacAddress_len =
                    HomeNetworkInfolist->vl_ocStbHostSystemHomeNetworkClientMacAddress_len;

            vlMemCpy(entry->ocStbHostSystemHomeNetworkClientIpAddress,
                     HomeNetworkInfolist->vl_ocStbHostSystemHomeNetworkClientIpAddress,MAC_MAX_SIZE, CHRMAX);
            entry->ocStbHostSystemHomeNetworkClientIpAddress_len =
                    HomeNetworkInfolist->vl_ocStbHostSystemHomeNetworkClientIpAddress_len;
            entry->ocStbHostSystemHomeNetworkClientDRMStatus =
                    HomeNetworkInfolist->vl_ocStbHostSystemHomeNetworkClientDRMStatus;

            netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,
                                        &(entry->ocStbHostSystemHomeNetworkIndex),
                                          sizeof(entry->ocStbHostSystemHomeNetworkIndex));
            netsnmp_tdata_add_row(table_data, row);
            return 1;
        }

/**
 * @brief This function is used to get the values of the entries for System Memory
 * Report like Memory Type, Memory Size  etc. and create and fill these entries.
 * System Memory Report Table
 *
 * @param[out] table_data Table where values of entries for Home Network Table are updated.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
        int vl_ocStbHostSystemMemoryReportTable_getdata(netsnmp_tdata * table_data)
        {
            bool replace_table = false;
            int nmemtype = 0;
            int nitemreplace = 0;

            if(netsnmp_tdata_row_first(table_data))
            {
                replace_table = true;
            }
            SNMPocStbHostSystemMemoryReportTable_t sysMemoryRep[MAX_ROWS];
            //vlMemSet(sysMemoryRep, 0 , sizeof(sysMemoryRep), sizeof(sysMemoryRep));
            memset(sysMemoryRep, 0 , sizeof(sysMemoryRep));

            if( 0 == avobj.vlGet_ocStbHostSystemMemoryReportTableData(sysMemoryRep, &nmemtype))
            {
                SNMP_DEBUGPRINT("ERROR ::vlGet_ocStbHostSystemMemoryReportTableData");
           // return 0;
            }
            if(nmemtype != 0)
            {
                for(int nmem = 0; nmem < nmemtype; nmem++)
                {
                    if(replace_table)
                    {
                        replace_table = false;
                        Table_free(table_data);
                    }

                    ocStbHostSystemMemoryReportTable_createEntry_allData(table_data, &sysMemoryRep[nmem], nmem);


                }

            }
            else
            {
                /** Default Intilization is don if no entries are there , this is for presen's of Table */
                Table_free(table_data);

                SNMPocStbHostSystemMemoryReportTable_t sysMemoryRep;
                //vlMemSet(&sysMemoryRep, 0 , sizeof(sysMemoryRep),  sizeof(sysMemoryRep));
                memset(&sysMemoryRep, 0 , sizeof(sysMemoryRep));
                ocStbHostSystemMemoryReportTable_createEntry_allData(table_data,&sysMemoryRep, 0);
            }

            return 1;
        }

/**
 * @brief This function is used to copy values of entries from the passed input
 * parameter to the Memory Report Table.
 *
 * @param[in] HomeNetworkindex Index for Home Network.
 * @param[in] SystemMemoryInfolist Struct pointer where info about system memory is present.
 * @param[out] table_data Table where Memory report entries are updated.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
        int ocStbHostSystemMemoryReportTable_createEntry_allData(netsnmp_tdata * table_data, SNMPocStbHostSystemMemoryReportTable_t *SystemMemoryInfolist, int SystemMemoryindex)
        {

            struct ocStbHostSystemMemoryReportTable_entry *entry;
            netsnmp_tdata_row *row;
            entry = SNMP_MALLOC_TYPEDEF(struct ocStbHostSystemMemoryReportTable_entry);
            if (!entry)
                return 0;

            row = netsnmp_tdata_create_row();
            if (!row) {
                SNMP_FREE(entry);
                return 0;
            }
            row->data = entry;
            entry->ocStbHostSystemMemoryReportIndex = SystemMemoryindex +1;
    // +1 is for index purpose
            entry->ocStbHostSystemMemoryReportMemoryType = SystemMemoryInfolist->vl_ocStbHostSystemMemoryReportMemoryType;
            entry->ocStbHostSystemMemoryReportMemorySize =
                    SystemMemoryInfolist->vl_ocStbHostSystemMemoryReportMemorySize;
            netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,
                                        &(entry->ocStbHostSystemMemoryReportIndex),
                                          sizeof(entry->ocStbHostSystemMemoryReportIndex));
            netsnmp_tdata_add_row(table_data, row);
            return 1;
        }

        SNMPocStbHostCCAppInfoTable_t CCAppInfolist[MAX_ROWS];
        SNMPocStbHostCCAppInfoTable_t CCAppInfolist_empty;

/**
 * @brief This function is used to get the values of the entries for Closed Caption
 * info,like Application Type, Application version  etc. and create and fill these entries.
 * Closed Caption AppInfo Table.
 *
 * @param[out] table_data Table where values of entries for Home Network Table are updated.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
        int vl_ocStbHostCCAppInfoTable_load_getdata(netsnmp_tdata * table_data)
        {

            bool replace_table = false;
            int nitemreplace = 0;
            int nitems = 0;
            if(netsnmp_tdata_row_first(table_data))
            {
                replace_table = true;
            }
     
            //vlMemSet(CCAppInfolist, 0 ,sizeof(CCAppInfolist), sizeof(CCAppInfolist));
            memset(CCAppInfolist, 0 ,sizeof(CCAppInfolist));

//            if( 0 == avobj.vlGet_ocStbHostCCAppInfoTableData(CCAppInfolist,  &nitems));
            avobj.vlGet_ocStbHostCCAppInfoTableData(CCAppInfolist,  &nitems);
#if 0
            {

                SNMP_DEBUGPRINT("ERROR ::vlGet_ocStbHostCCAppInfoTableData");
           // return 0;
            }
#endif

            if(nitems != 0)
            {
                for(int napp = 0; napp < nitems; napp++)
                {
                    if(replace_table)
                    {
                        replace_table = false;
                        Table_free(table_data);
                    }

                    static bool bUseAppInfoTypeIndices = vl_env_get_bool(true, "FEATURE.CARD_MMI.USE_APP_INFO_TYPE_INDICES");
                    if(bUseAppInfoTypeIndices)
                    {
                        ocStbHostCCAppInfoTable_createEntry_allData(table_data, &CCAppInfolist[napp], CCAppInfolist[napp].vl_ocStbHostCCApplicationType);
                    }
                    else
                    {
                        ocStbHostCCAppInfoTable_createEntry_allData(table_data, &CCAppInfolist[napp], napp);
                    }
                }

            }
            else
            {
                /** Default Intilization is don if no entries are there , this is for present of Table */
                Table_free(table_data);

     
                //vlMemSet(&CCAppInfolist_empty, 0 , sizeof(CCAppInfolist_empty), sizeof(CCAppInfolist_empty));
                memset(&CCAppInfolist_empty, 0 , sizeof(CCAppInfolist_empty));
                ocStbHostCCAppInfoTable_createEntry_allData(table_data,&CCAppInfolist_empty, 0);
            }

            return 1;


        }

/**
 * @brief This function is used to copy values of entries from the passed input
 * parameter to the CCApp info Table.
 *
 * @param[in] CCAppindex Index for closed-caption info.
 * @param[in] CCAppInfolist Struct pointer where info about closed caption is present.
 * @param[out] table_data Table where closed caption info entries are updated.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
        int ocStbHostCCAppInfoTable_createEntry_allData(netsnmp_tdata * table_data, SNMPocStbHostCCAppInfoTable_t *CCAppInfolist, int CCAppindex)
        {
            struct ocStbHostCCAppInfoTable_entry *entry;
            netsnmp_tdata_row *row;
            entry = SNMP_MALLOC_TYPEDEF(struct ocStbHostCCAppInfoTable_entry);
            if (!entry)
                return 0;

            row = netsnmp_tdata_create_row();
            if (!row) {
                SNMP_FREE(entry);
                return 0;
            }
            row->data = entry;
            static bool bUseAppInfoTypeIndices = vl_env_get_bool(true, "FEATURE.CARD_MMI.USE_APP_INFO_TYPE_INDICES");
            if(bUseAppInfoTypeIndices)
            {
                entry->ocStbHostCCAppInfoIndex = CCAppindex;
            }
            else
            {
                entry->ocStbHostCCAppInfoIndex = CCAppindex+1;
            }
            entry->ocStbHostCCApplicationType = CCAppInfolist->vl_ocStbHostCCApplicationType;
            entry->ocStbHostCCApplicationName_len = CCAppInfolist->vl_ocStbHostCCApplicationName_len;
            vlMemCpy(entry->ocStbHostCCApplicationName,CCAppInfolist->vl_ocStbHostCCApplicationName, entry->ocStbHostCCApplicationName_len, CHRMAX);

            entry->ocStbHostCCApplicationVersion = CCAppInfolist->vl_ocStbHostCCApplicationVersion;
            entry->ocStbHostCCAppInfoPage_len = CCAppInfolist->vl_ocStbHostCCAppInfoPage_len;
        //vlMemSet(entry->ocStbHostCCAppInfoPage,0, sizeof(entry->ocStbHostCCAppInfoPage),sizeof(entry->ocStbHostCCAppInfoPage) );
        memset(entry->ocStbHostCCAppInfoPage,0, sizeof(entry->ocStbHostCCAppInfoPage));
            vlMemCpy(entry->ocStbHostCCAppInfoPage, CCAppInfolist->vl_ocStbHostCCAppInfoPage,entry->ocStbHostCCAppInfoPage_len, sizeof(entry->ocStbHostCCAppInfoPage));


            netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,
                                        &(entry->ocStbHostCCAppInfoIndex),
                                          sizeof(entry->ocStbHostCCAppInfoIndex));
            netsnmp_tdata_add_row(table_data, row);
            return 1;


        }

/**
 * @brief This function is used to assign the data to the Software Application
 * Info Table for the required number of applications.
 *
 * @param[in] nApplications Number of Applications.
 * @param[out] pAppInfolist Table where Software Application Info is stored.
 *
 * @return On success it returns 1 and on fail it returns 0.
 */
        int vl_ocStbHostSoftwareApplicationInfoTable_setdata(int nApplications, SNMPocStbHostSoftwareApplicationInfoTable_t * pAppInfolist)
        {
            return avobj.vlSet_ocStbHostSoftwareApplicationInfoTable_set_list(nApplications, pAppInfolist);
        }

        SNMPocStbHostSoftwareApplicationInfoTable_t swapplication[MAX_ROWS];
        SNMPocStbHostSoftwareApplicationInfoTable_t swapplication_empty;

/**
 * @brief This function is used to get the information about application like
 * application id, name, version number etc. and fill these info to Application
 * info table.
 *
 * @param[out] table_data Netsnmp table, where application info is retrieved.
 *
 * @return On success it returns 1 and on fail it returns 0.
 */
        int vl_ocStbHostSoftwareApplicationInfoTable_getdata(netsnmp_tdata * table_data)
        {

            bool replace_table = false;
            int nitemreplace = 0;
            int nswappl = 0;
            if(netsnmp_tdata_row_first(table_data))
            {
                replace_table = true;
            }
     
            //vlMemSet(swapplication, 0 ,  sizeof(swapplication) , sizeof(swapplication));
            memset(swapplication, 0 ,  sizeof(swapplication));

            if( 0 == avobj.vlGet_ocStbHostSoftwareApplicationInfoTableData(swapplication,  &nswappl))
            {
                SNMP_DEBUGPRINT("ERROR ::vlGet_ocStbHostSoftwareApplicationInfoTableData");
           // return 0;
            }
            else {
                SNMP_DEBUGPRINT("\n::error::\n");
          //call default intilization
            }
            if(nswappl != 0)
            {
                for(int nsw = 0; nsw < nswappl; nsw++)
                {
                    if(replace_table)
                    {
                        replace_table = false;
                        Table_free(table_data);
                    }
                    ocStbHostSoftwareApplicationInfoTable_createEntry_allData(table_data, &swapplication[nsw], nsw);

                }

            }
            else
            {
                /** Default Intilization is don if no entries are there , this is for present of Table */
                Table_free(table_data);

      
                //vlMemSet(&swapplication_empty, 0 , sizeof(swapplication_empty), sizeof(swapplication_empty));
                memset(&swapplication_empty, 0 , sizeof(swapplication_empty));
                ocStbHostSoftwareApplicationInfoTable_createEntry_allData(table_data,&swapplication_empty, 0);
            }

            return 1;

        }

/**
 * @brief This function is used to copy values of entries from the passed input
 * parameter to the Software application table.
 *
 * @param[in] SAppindex Software application index.
 * @param[in] SAppInfolist Struct pointer where info about software application is present.
 * @param[out] table_data Table where software application entries are updated.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
        int ocStbHostSoftwareApplicationInfoTable_createEntry_allData(netsnmp_tdata* table_data, SNMPocStbHostSoftwareApplicationInfoTable_t *SAppInfolist, int SAppindex)
        {

            struct ocStbHostSoftwareApplicationInfoTable_entry *entry;
            netsnmp_tdata_row *row;
            entry = SNMP_MALLOC_TYPEDEF(struct ocStbHostSoftwareApplicationInfoTable_entry);
            if (!entry)
                return 0;

            row = netsnmp_tdata_create_row();
            if (!row) {
                SNMP_FREE(entry);
                return 0;
            }
            row->data = entry;
            entry->ocStbHostSoftwareApplicationInfoIndex = SAppindex+1;
            entry->ocStbHostSoftwareAppNameString_len = SAppInfolist->vl_ocStbHostSoftwareAppNameString_len;
            vlMemCpy(entry->ocStbHostSoftwareAppNameString, SAppInfolist->vl_ocStbHostSoftwareAppNameString, entry->ocStbHostSoftwareAppNameString_len, CHRMAX);
            entry->ocStbHostSoftwareAppVersionNumber_len = SAppInfolist->vl_ocStbHostSoftwareAppVersionNumber_len;
            vlMemCpy(entry->ocStbHostSoftwareAppVersionNumber, SAppInfolist->vl_ocStbHostSoftwareAppVersionNumber,  entry->ocStbHostSoftwareAppVersionNumber_len, CHRMAX);

            entry->ocStbHostSoftwareStatus = SAppInfolist->vl_ocStbHostSoftwareStatus;
            entry->ocStbHostSoftwareOrganizationId_len = 4;
            vlMemCpy(entry->ocStbHostSoftwareOrganizationId, SAppInfolist->vl_ocStbHostSoftwareOrganizationId, entry->ocStbHostSoftwareOrganizationId_len, SORG_ID);
            entry->ocStbHostSoftwareApplicationId_len = 2;
            vlMemCpy(entry->ocStbHostSoftwareApplicationId, SAppInfolist->vl_ocStbHostSoftwareApplicationId, entry->ocStbHostSoftwareApplicationId_len, SAID);
            entry->ocStbHostSoftwareApplicationSigStatus = SAppInfolist->vl_ocStbHostSoftwareApplicationSigStatus;


            netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,
                                        &(entry->ocStbHostSoftwareApplicationInfoIndex),
                                          sizeof(entry->ocStbHostSoftwareApplicationInfoIndex));
            netsnmp_tdata_add_row(table_data, row);
            return 1;

        }

        SNMPifTable_t ifdata[IFTABLE_MAX_ROW]; //ifdata will act as index for IpNetToPhyscialTable
        SNMPipNetToPhysicalTable_t ipnettophy[IFTABLE_MAX_ROW]; // for ipNetToPhysicalTable content

/**
 * @brief This function is used to get the information of IP like physical address
 * MTU, speed etc. and fill these info to ifTable.
 *
 * @param[out] table_data Netsnmp table, where Interface info is retrieved.
 *
 * @return On success it returns 1 and on fail it returns 0.
 */
        int vl_ifTable_getdata(netsnmp_tdata * table_data)
        {

            bool replace_table = false;
            int nitemreplace = 0;
            int iftabelcheck = 0;
            if(netsnmp_tdata_row_first(table_data))
            {
                replace_table = true;
            }

            if( 0 == avobj.vlGet_ifIpTableData( ifdata, ipnettophy, &iftabelcheck))
            {
                SNMP_DEBUGPRINT("ERROR ::vlGet_ifTableData");
           // return 0;
            }
//     else {
//             SNMP_DEBUGPRINT("\n::error:: vlGet_ifTableData\n");
//     }
            /* As the ifTable will have a fixed entry of two elements */
            for(int count = 0; count < IFTABLE_MAX_ROW; count++)
            {
                if(replace_table)
                {
                    replace_table = false;
                    Table_free(table_data);
                }
                ifTable_createEntry_allData(table_data, &ifdata[count]);
            }
//     }
//      else
//     {
            //       /** Default Intilization is don if no entries are there , this is for present of Table */
//       Table_free(table_data);
            //
//       ifTable_createEntry_allData(table_data, &ifdata[count]);
//     }

            return 1;

        }

/**
 * @brief This function is used to copy values of IP entries from the passed input
 * parameter to the Interface table.
 *
 * @param[in] SifTablelist Struct pointer where data of IP interface is present.
 * @param[out] table_data Table where IP interface entries are updated.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
        int ifTable_createEntry_allData(netsnmp_tdata * table_data, SNMPifTable_t *SifTablelist)
        {

            struct ifTable_entry *entry;
            netsnmp_tdata_row *row;
            entry = SNMP_MALLOC_TYPEDEF(struct ifTable_entry);
            if (!entry)
                return 0;

            row = netsnmp_tdata_create_row();
            if (!row) {
                SNMP_FREE(entry);
                return 0;
            }
            row->data = entry;
    //entry->ifIndex;
            entry->ifIndex = SifTablelist->vl_ifIndex;
    /*
            * Column values
    */

            entry->ifindex =  SifTablelist->vl_ifindex;//Index for IpNetToPhyscialTable
            entry->ifDescr_len = SifTablelist->vl_ifDescr_len;
            vlMemCpy(entry->ifDescr, SifTablelist->vl_ifDescr, entry->ifDescr_len, CHRMAX);
            entry->ifType = SifTablelist->vl_ifType;
            entry->ifMtu = SifTablelist->vl_ifMtu;
            entry->ifSpeed = SifTablelist->vl_ifSpeed;
            entry->ifPhysAddress_len = SifTablelist->vl_ifPhysAddress_len;
            vlMemCpy(entry->ifPhysAddress,
                     SifTablelist->vl_ifPhysAddress,entry->ifPhysAddress_len, CHRMAX);
            entry->ifAdminStatus = SifTablelist->vl_ifAdminStatus;
            entry->old_ifAdminStatus = SifTablelist->vl_old_ifAdminStatus;
            entry->ifOperStatus = SifTablelist->vl_ifOperStatus;
            entry->ifLastChange = SifTablelist->vl_ifLastChange;
            entry->ifInOctets = SifTablelist->vl_ifInOctets;
            entry->ifInUcastPkts = SifTablelist->vl_ifInUcastPkts;
            entry->ifInNUcastPkts = SifTablelist->vl_ifInNUcastPkts;
            entry->ifInDiscards = SifTablelist->vl_ifInDiscards;
            entry->ifInErrors = SifTablelist->vl_ifInErrors;
            entry->ifInUnknownProtos = SifTablelist->vl_ifInUnknownProtos;
            entry->ifOutOctets = SifTablelist->vl_ifOutOctets;
            entry->ifOutUcastPkts = SifTablelist->vl_ifOutUcastPkts;
            entry->ifOutNUcastPkts = SifTablelist->vl_ifOutNUcastPkts;
            entry->ifOutDiscards = SifTablelist->vl_ifOutDiscards;
            entry->ifOutErrors = SifTablelist->vl_ifOutErrors;
            entry->ifOutQLen = SifTablelist->vl_ifOutQLen;
            entry->ifSpecific_len = SifTablelist->vl_ifSpecific_len;
            vlMemCpy(entry->ifSpecific,SifTablelist->vl_ifSpecific, entry->ifSpecific_len, CHRMAX);


            netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,
                                        &(entry->ifIndex),
                                          sizeof(entry->ifIndex));
            netsnmp_tdata_add_row(table_data, row);
            return 1;

        }

/**
 * @brief This function is used to get the values of IP like physical address
 * MTU, speed etc. and fill corresponding entry in Physical Table.
 *
 * @param[out] table_data Netsnmp table, where Interface info is retrieved.
 *
 * @return On success it returns 1 and on fail it returns 0.
 */
        int vl_ipNetToPhysicalTable_getdata(netsnmp_tdata * table_data)
        {
            bool replace_table = false;
            int nitemreplace = 0;
            int iptabelcheck = 0;
            if(netsnmp_tdata_row_first(table_data))
            {
                replace_table = true;
            }

            if( 0 == avobj.vlGet_ifIpTableData( ifdata, ipnettophy, &iptabelcheck))
            {
                SNMP_DEBUGPRINT("ERROR ::vlGet_ifTableData");
           // return 0;
            }
            /* ipnettppyy table date will be collected in vlGet_ifIpTableData( ifdata, ipnettophy, &iftabelcheck)  */
            for(int count = 0; count < IFTABLE_MAX_ROW; count++)
            {
                if(replace_table)
                {
                    replace_table = false;
                    Table_free(table_data);
                }
                ipNetToPhysicalTable_createEntry_allData(table_data, &ipnettophy[count]);
            }
    //}
//      else
//     {
            //       /** Default Intilization is don if no entries are there , this is for present of Table */
//       Table_free(table_data);
            //
//       ipNetToPhysicalTable_createEntry_allData(table_data, &ipnettophy[count]);
//     }

            return 1;
        }

/**
 * @brief This function is used to copy values of IP entries from the passed input
 * parameter to the Physical table.
 *
 * @param[in] SipNetToPhysicallist Struct pointer where data about IP network.
 * @param[out] table_data Table where IP network and physical table entries are updated.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
        int ipNetToPhysicalTable_createEntry_allData(netsnmp_tdata * table_data, SNMPipNetToPhysicalTable_t *SipNetToPhysicallist)
        {
            struct ipNetToPhysicalTable_entry *entry;
            netsnmp_tdata_row *row;
            entry = SNMP_MALLOC_TYPEDEF(struct ipNetToPhysicalTable_entry);
            if (!entry)
                return 0;

            row = netsnmp_tdata_create_row();
            if (!row) {
                SNMP_FREE(entry);
                return 0;
            }
            row->data = entry;
            //vlMemSet(entry,0,sizeof(struct ipNetToPhysicalTable_entry),sizeof(struct ipNetToPhysicalTable_entry));
            memset(entry,0,sizeof(struct ipNetToPhysicalTable_entry));
//#if 0
            entry->ipNetToPhysicalIfIndex = SipNetToPhysicallist->vl_ipNetToPhysicalIfIndex;
    /*
            * Column values
    */
   // SNMP_DEBUGPRINT("\n1::: entry->ipNetToPhysicalIfIndex = %d \t SipNetToPhysicallist->vl_ipNetToPhysicalIfIndex %d \n", entry->ipNetToPhysicalIfIndex,SipNetToPhysicallist->vl_ipNetToPhysicalIfIndex );
            entry->ipNetToPhysicalNetAddressType = SipNetToPhysicallist->vl_ipNetToPhysicalNetAddressType;
//SNMP_DEBUGPRINT("\n1::: entry->ipNetToPhysicalNetAddressType = %d \t SipNetToPhysicallist->vl_ipNetToPhysicalNetAddressType %d \n", entry->ipNetToPhysicalNetAddressType,SipNetToPhysicallist->vl_ipNetToPhysicalNetAddressType );


            entry->ipNetToPhysicalNetAddress_len = SipNetToPhysicallist->vl_ipNetToPhysicalNetAddress_len;

//SNMP_DEBUGPRINT("\n1:::entry->ipNetToPhysicalNetAddress_len = %d \t SipNetToPhysicallist->vl_ipNetToPhysicalNetAddress_len %d \n", entry->ipNetToPhysicalNetAddress_len ,SipNetToPhysicallist->vl_ipNetToPhysicalNetAddress_len);

            vlMemCpy(entry->ipNetToPhysicalNetAddress, SipNetToPhysicallist->vl_ipNetToPhysicalNetAddress, entry->ipNetToPhysicalNetAddress_len, CHRMAX);

//SNMP_DEBUGPRINT("\n1::: entry->ipNetToPhysicalNetAddress = %s \t SipNetToPhysicallist->vl_ipNetToPhysicalNetAddress = %s \n", entry->ipNetToPhysicalNetAddress,SipNetToPhysicallist->vl_ipNetToPhysicalNetAddress);

            entry->ipNetToPhysicalPhysAddress_len = SipNetToPhysicallist->vl_ipNetToPhysicalPhysAddress_len;

//SNMP_DEBUGPRINT("\n1::: entry->ipNetToPhysicalPhysAddress_len  = %d \t SipNetToPhysicallist->vl_ipNetToPhysicalPhysAddress_len %d \n", entry->ipNetToPhysicalPhysAddress_len ,SipNetToPhysicallist->vl_ipNetToPhysicalPhysAddress_len);


            vlMemCpy(entry->ipNetToPhysicalPhysAddress, SipNetToPhysicallist->vl_ipNetToPhysicalPhysAddress, entry->ipNetToPhysicalPhysAddress_len, CHRMAX);

//SNMP_DEBUGPRINT("\n1::: entry->ipNetToPhysicalPhysAddress = %s \t SipNetToPhysicallist->vl_ipNetToPhysicalPhysAddress %s \n", entry->ipNetToPhysicalPhysAddress ,SipNetToPhysicallist->vl_ipNetToPhysicalPhysAddress);

            entry->ipNetToPhysicalLastUpdated = SipNetToPhysicallist->vl_ipNetToPhysicalLastUpdated;

//SNMP_DEBUGPRINT("\n1:::entry->ipNetToPhysicalLastUpdated = %d \t SipNetToPhysicallist->vl_ipNetToPhysicalLastUpdated %d \n", entry->ipNetToPhysicalLastUpdated ,SipNetToPhysicallist->vl_ipNetToPhysicalLastUpdated);

            entry->ipNetToPhysicalType = SipNetToPhysicallist->vl_ipNetToPhysicalType;
//SNMP_DEBUGPRINT("\n1:::entry->ipNetToPhysicalType = %d \t SipNetToPhysicallist->vl_ipNetToPhysicalType %d \n", entry->ipNetToPhysicalType ,SipNetToPhysicallist->vl_ipNetToPhysicalType);
    
            entry->ipNetToPhysicalNetAddressType = SipNetToPhysicallist->vl_ipNetToPhysicalNetAddressType;
//SNMP_DEBUGPRINT("\n1:::entry->ipNetToPhysicalNetAddressType = %d \t SipNetToPhysicallist->vl_ipNetToPhysicalNetAddressType %d \n", entry->ipNetToPhysicalNetAddressType ,SipNetToPhysicallist->vl_ipNetToPhysicalNetAddressType);

            entry->ipNetToPhysicalState = SipNetToPhysicallist->vl_ipNetToPhysicalState;
//SNMP_DEBUGPRINT("\n1:::entry->ipNetToPhysicalState = %d \t SipNetToPhysicallist->vl_ipNetToPhysicalState %d \n", entry->ipNetToPhysicalState ,SipNetToPhysicallist->vl_ipNetToPhysicalState);

            entry->ipNetToPhysicalRowStatus= SipNetToPhysicallist->vl_ipNetToPhysicalRowStatus;
//SNMP_DEBUGPRINT("\n1::: entry->ipNetToPhysicalRowStatus = %d \t SipNetToPhysicallist->vl_ipNetToPhysicalRowStatus %d \n",  entry->ipNetToPhysicalRowStatus ,SipNetToPhysicallist->vl_ipNetToPhysicalRowStatus);
//#endif //if 0
            netsnmp_tdata_row_add_index(row, ASN_INTEGER,
                                        &(entry->ipNetToPhysicalIfIndex),
                                          sizeof(entry->ipNetToPhysicalIfIndex));
            netsnmp_tdata_row_add_index(row, ASN_INTEGER,
                                        &(entry->ipNetToPhysicalNetAddressType),
                                          sizeof(entry->ipNetToPhysicalNetAddressType));
            netsnmp_tdata_row_add_index(row, ASN_OCTET_STR,
                                        entry->ipNetToPhysicalNetAddress,
                                        entry->ipNetToPhysicalNetAddress_len);
            netsnmp_tdata_add_row(table_data, row);
//SNMP_DEBUGPRINT("\n::ipNetToPhysicalTable_createEntry_allData END OF COLLECTION \n");
            return 1;
        }

        vlMCardCertInfo_t Vl_CertificationInfo;

/**
 * @brief This function is used to get the info about HDMI, IEEE1394, OCAP, M-CARD,
 * COMMONDOWNLOAD certification info etc and fill these data to it info table.
 * 
 * @param[out] table_data Table where sertification data is stored.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
        int  vl_VividlogiccertificationTable_getdata(netsnmp_tdata * table_data)
        {
            bool replace_table = false;
            int nitem = 0;

            if(netsnmp_tdata_row_first(table_data))
            {
                replace_table = true;
            }
            if(  0 == avobj.vl_GetMCertificationInfo(&Vl_CertificationInfo, &nitem))
            {
                SNMP_DEBUGPRINT("ERROR:: Certification info NULL \n");
                return GETDATA_FAILED;
            }
            /* ipnettppyy table date will be collected in vlGet_ifIpTableData( ifdata, ipnettophy, &iftabelcheck)  */
            if( nitem > MAX_CERTIFICATION_INFO)
            {
                nitem = MAX_CERTIFICATION_INFO;
            }
            //for(int count = 0; count < nitem; count++)
            //{
                if(replace_table)
                {
                    replace_table = false;
                    Table_free(table_data);
                }
                Vividlogiccertification_createEntry_allData(table_data,
                        &Vl_CertificationInfo ,nitem);
            //}

            return 1;
        }

/**
 * @brief This function copies all received value of entries for certification details
 * and fill these to the table.
 * 
 * @param[in] count Index number
 * @param[in] pMcardinfo Struct pointer where cerfification data is present.
 * @param[out] table_data Table where all data has to be copied.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
        int Vividlogiccertification_createEntry_allData(netsnmp_tdata
                * table_data, vlMCardCertInfo_t *pMcardinfo , int count){

            struct McardcertificationTable_entry *entry;
            netsnmp_tdata_row *row;
            entry = SNMP_MALLOC_TYPEDEF(struct McardcertificationTable_entry);
            if (!entry)
                return GETDATA_FAILED;

            row = netsnmp_tdata_create_row();
            if (!row) {
                SNMP_FREE(entry);
                return GETDATA_FAILED;
            }
            row->data = entry;
            memset(entry,0,sizeof(struct McardcertificationTable_entry));

            entry->VividlogicMcardInterfaceIndex = count ; // Index should start with 1
        
            entry->VividlogicSzDispString_len = strlen(pMcardinfo->vl_szDispString);
            vlMemCpy(entry->VividlogicSzDispString, pMcardinfo->vl_szDispString,
                     entry->VividlogicSzDispString_len,CARD_SIZE);     
            
            entry->VividlogicMcardSzDeviceCertSubjectName_len = strlen(pMcardinfo->vl_szDeviceCertSubjectName);
            vlMemCpy(entry->VividlogicMcardSzDeviceCertSubjectName, pMcardinfo->vl_szDeviceCertSubjectName, 
                     entry->VividlogicMcardSzDeviceCertSubjectName_len,CARD_SIZE);  
            
            entry->VividlogicMcardSzDeviceCertIssuerName_len = strlen(pMcardinfo->vl_szDeviceCertIssuerName);
            vlMemCpy(entry->VividlogicMcardSzDeviceCertIssuerName , pMcardinfo->vl_szDeviceCertIssuerName, 
                     entry->VividlogicMcardSzDeviceCertIssuerName_len, CARD_SIZE);  
          
            entry->VividlogicMcardSzManCertSubjectName_len = strlen(pMcardinfo->vl_szManCertSubjectName);
            vlMemCpy(entry->VividlogicMcardSzManCertSubjectName , pMcardinfo->vl_szManCertSubjectName, 
                     entry->VividlogicMcardSzManCertSubjectName_len, CARD_SIZE);  
                           
            entry->VividlogicMcardSzManCertIssuerName_len =  strlen(pMcardinfo->vl_szManCertIssuerName);
            vlMemCpy(entry->VividlogicMcardSzManCertIssuerName ,  pMcardinfo->vl_szManCertIssuerName, 
                     entry->VividlogicMcardSzManCertIssuerName_len, CARD_SIZE);  
            
                                           
            entry->VividlogicMcardAcHostId_len =  strlen(pMcardinfo->vl_acHostId);
            vlMemCpy(entry->VividlogicMcardAcHostId  ,  pMcardinfo->vl_acHostId, 
                     entry->VividlogicMcardAcHostId_len, CARD_SIZE);  

		
             vlStrCpy(entry->VividlogicMcardbCertAvailable, pMcardinfo->vl_bCertAvailable,sizeof(pMcardinfo->vl_bCertAvailable));
	     vlStrCpy(entry->VividlogicMcardbCertValid, pMcardinfo->vl_bCertValid,sizeof(pMcardinfo->vl_bCertValid));
             vlStrCpy(entry->VividlogicMcardbVerifiedWithChain, pMcardinfo->vl_bVerifiedWithChain,sizeof(pMcardinfo->vl_bVerifiedWithChain));
             vlStrCpy(entry->VividlogicMcardbIsProduction, pMcardinfo->vl_bIsProduction,sizeof(pMcardinfo->vl_bIsProduction));
           
//printf(" \n DEBUG TEST %s %d \n ", __FUNCTION__ ,  __LINE__);

   // memset(entry->VividlogicCertFullInfo, 0 , sizeof(entry->VividlogicCertFullInfo));


    //strcat(entry->VividlogicCertFullInfo,"\0");
            netsnmp_tdata_row_add_index(row, ASN_UNSIGNED,
                     &(entry->VividlogicMcardInterfaceIndex),
                     sizeof(entry->VividlogicMcardInterfaceIndex));
            netsnmp_tdata_add_row(table_data, row);

             return 1;
        }

/**
 * @brief This function is used to get host serial number.
 *
 * @param[in] nChars Number of characters to be copied.
 * @param[out] SerialNumber String pointer to contain Host serial number.
 *
 * @return Returns 1 (as success) by default.
 */
        unsigned int Sget_ocStbHostSerialNumber(char* SerialNumber, int nChars)
        {
            avobj.vlGet_Serialnum(SerialNumber, nChars);
   //SNMP_DEBUGPRINT("\nSERIAL NUMEBR %s\n",SerialNumber);
   //memcpy(SerialNumber,"TEMP serial number",strlen("TEMP serial number"));
            return 1;

        }

/**
 * @brief This function is used to get host ID number.
 *
 * @param[out] snmpHostID String pointer to contain Host id.
 *
 * @return Returns 1 (as success) by default.
 */
        unsigned int Sget_ocStbHostHostID(char* snmpHostID)
        {

            avobj.vlGet_Hostid(snmpHostID);
      //  memcpy(snmpHostID,"TEMP HOST ID ",strlen("TEMP HOST ID "));
            return 1;
        }

/**
 * @brief This function is used to get host capability like OCHD2 EMBEDDED etc.
 *
 * @param[out] HostCapabilities Struct pointer to contain Host capability.
 *
 * @return Returns 1 (as success) by default.
 */
        unsigned int Sget_ocStbHostCapabilities(HostCapabilities_snmpt* HostCapabilities)
        {
            int hostcaptype;
            avobj.vlGet_Hostcapabilite(&hostcaptype);
            switch(hostcaptype){

                case 1:
                    *HostCapabilities = S_CAPABILITIES_OTHER;
                    break;
                case 2:
                    *HostCapabilities = S_CAPABILITIES_OCHD2;
                    break;
                case 3:
                    *HostCapabilities = S_CAPABILITIES_EMBEDDED;
                    break;
                case 4:
                    *HostCapabilities = S_CAPABILITIES_DCAS;
                    break;
                case 5:
                    *HostCapabilities = S_CAPABILITIES_OCHD21;
                    break;
                case 6:
                    *HostCapabilities = S_CAPABILITIES_BOCR;
                    break;
                case 7:
                    *HostCapabilities = S_CAPABILITIES_OCHD21TC;
                    break;
                default :
                    *HostCapabilities = S_CAPABILITIES_OTHER;
                    break;
            }
//       *HostCapabilities = S_CAPABILITIES_OCHD2;
            return 1;

        }

/**
 * @brief This function is used to get status about AVC support.
 *
 * @param[out] avcSupport Status of AVC support.
 *
 * @return Returns 1 (as success) by default.
 */
        unsigned int Sget_ocStbHostAvcSupport(Status_t* avcSupport)
        {
            avobj.vlGet_ocStbHostAvcSupport(avcSupport);
            return 1;

        }

/**
 * @brief This function is used to get status of EAS message.
 *
 * @param[out] EasobectsInfo Status of EAS meaasage.
 *
 * @return Returns 1 (as success) by default.
 */
        unsigned int Sget_ocStbEasMessageStateInfo( EasInfo_t* EasobectsInfo)
        {

            avobj.vlGet_ocStbEasMessageState(EasobectsInfo);

     /*  EasobectsInfo->EMSCodel = 10;
            EasobectsInfo->EMCCode = 20;
            EasobectsInfo->EMCSCodel = 9;
            */  return 1;
        }

/**
 * @brief This function is used to get info about device software like Firmware
 * Version, Firmware OCAP Version and update the structure.
 *
 * @param[out] DevSoftareInfo Struct pointer where info of device software is saved.
 *
 * @return Returns 1 (as success) by default.
 */
        unsigned int Sget_ocStbHostDeviceSoftware(DeviceSInfo_t * DevSoftareInfo)
        {
            avobj.vlGet_ocStbHostDeviceSoftware(DevSoftareInfo);

            return 1;
        }

/**
 * @brief This function is used to get the status of firmware downloading.
 *
 * @param[out] FirmwareStatus Struct pointer to contain status of firmware downloading.
 *
 * @return Returns 1 (as success) by default.
 */
        unsigned int Sget_ocStbHostFirmwareDownloadStatus(FDownloadStatus_t* FirmwareStatus)
        {

            avobj.vlGet_HostFirmwareDownloadStatus(FirmwareStatus);
            return 1;

        }

/**
 * @brief This function is used to get the status of application signature by 
 * checking last read status and last receive time.
 *
 * @param[out] pswappStaticstatus Struct pointer to contain status of application signature.
 *
 * @return Returns 1 (as success) by default.
 */
        unsigned int Sget_ocStbHostSoftwareApplicationSigstatus(SNMPocStbHostSoftwareApplicationInfoTable_t* pswappStaticstatus)
        {

            avobj.vlGet_ocStbHostSoftwareApplicationSigstatus(pswappStaticstatus);
            return 1;

        }

/**
 * @brief This function is used to get the info about security like CA type, id etc.
 * and update to the structure.
 *
 * @param[out] HostCATobj Struct pointer to contain status of application signature.
 *
 * @return Returns 1 (as success) by default.
 */
        unsigned int Sget_ocStbHostSecurityIdentifier(HostCATInfo_t* HostCATobj)
        {

            avobj.vlGet_HostCaTypeInfo(HostCATobj);


  /*    char secIDtemp[255] = "TEMP CASECURITY ID";
            char CAsIDtemp2[255] = "TEMP CASECURITY ID";

            memcpy(HostCATobj->S_SecurityID ,secIDtemp,strlen(secIDtemp)+1);
            memcpy(HostCATobj->S_CASystemID ,CAsIDtemp2,strlen(CAsIDtemp2)+1);
            HostCATobj->S_HostCAtype = S_CATYPE_CABLECARD;
            */    return 1;
        }
        /**     ocStbHostPowerStatus     */
        unsigned int Sget_ocStbHostPowerInf(HostPowerInfo_t* Hostpowerstatus)
        {
            avobj.vlGet_ocStbHostPower(Hostpowerstatus);

     //Hostpowerstatus->ocStbHostPowerStatus = S_POWERON ;
    // Hostpowerstatus->ocStbHostAcOutletStatus =  S_UNSWITCHED;
            return 1;
        }
        /**     OCSTBHOST_USERSETTINGS     */
        unsigned int Sget_ocStbHostUserSettingsPreferedLanguage(char* UserSettingsPreferedLanguage)
        {
            avobj.vlGet_UserSettingsPreferedLanguage(UserSettingsPreferedLanguage);
      // memcpy(UserSettingsPreferedLanguage ,"ENGLISH",strlen("ENGLISH"));
            return 1;
        }
        unsigned int Sget_ocStbHostCardInfo(CardInfo_t* CardIpInfo)
        {

            avobj.vlGet_ocStbHostCardInfo(CardIpInfo);
       //get_IP_MACadress(CardIpInfo);
       /*memcpy(CardIpInfo->ocStbHostCardMACAddress ,"00-00-0",strlen("00-00-0"));
            memcpy(CardIpInfo->ocStbHostCardIpAddress ,"192.168.1.110",strlen("192.168.1.110"));
            CardIpInfo->ocStbHostCardIpAddressType = S_CARDIPADDRESSTYPE_IPV4;
       */
            return 1;
        }
        unsigned int Sget_ocStbHostCMMIInfo(SCCMIAppinfo_t* CardIpInfo)
        {
  // memset(&CardIpInfo, 0, sizeof(SCCMIAppinfo_t));
            return 1;
        }
        /** OCATBHOST_INFO */
        unsigned int Sget_ocstbHostInfo(SOcstbHOSTInfo_t* HostIpinfo)
        {
            avobj.vlGet_ocstbHostInfo(HostIpinfo);
   // memset(&HostIpinfo, 0,sizeof(SOcstbHOSTInfo_t));
            return 1;
        }
        /** OCATBHOST_REBOOTINFO */
        unsigned int Sget_OcstbHostRebootInfo(SOcstbHostRebootInfo_t* HostRebootInfo)
        {
            avobj.vlGet_OcstbHostReboot(HostRebootInfo);

            return 1;
        }
/**
 * @brief This function is used to get the info about reboot ur reset of host.
 *
 * @return Returns 1 by default for success
 */
        unsigned int SSet_OcstbHostRebootInfo()
        {
            RDK_LOG(RDK_LOG_INFO, "LOG.RDK.SNMP", "\n SSet_OcstbHostRebootInfo:: calling avobj.vlSet_OcstbHostReboot()\n");
            avobj.vlSet_OcstbHostReboot();
            return 1;
        }

        /** OCATBHOST_MEMORYINFO */
        unsigned int Sget_OcstbHostMemoryInfo(SMemoryInfo_t* HostMemoryInfo)
        {
            avobj.vlGet_OcstbHostMemoryInfo(HostMemoryInfo);
            return 1;
        }
        /** OCATBHOST_JVMINFO */
        unsigned int Sget_OcstbHostJVMInfo(SJVMinfo_t* pHostJVMInfo)
        {
            avobj.vlGet_ocStbHostJvmInfo(pHostJVMInfo);
            return 1;
        }
        /** HOSTQpskObjects*/
        unsigned int Sget_ocStbHostQpskObjects(ocStbHostQpsk_t* QpskObj)
        {
  //prasad
            avobj.vlGet_ocStbHostQpsk(QpskObj);

            return 1;
        }
        /** /** ocStbHostSnmpProxyInfo Card Info */
        unsigned int Sget_ocStbHostSnmpProxyCardInfo(SocStbHostSnmpProxy_CardInfo_t * ProxyCardInfo){

            avobj.vlGet_ocStbHostCard_Details(ProxyCardInfo);
            return 1;
        }
        /** SYSTEM INFO*/

/**
 * @brief This function is used to check the DVI/HDMI 3D-Compatibility. This value 
 * is updated in DVI/HDMI Table.
 *
 * @param[out] eCompatibilityType Address where DVI/HDMI 3D-Compatibility info is retrieved.
 *
 * @return On success it returns 1.
 */ 
unsigned int Sset_ocStbHostDVIHDMI3DCompatibilityControl(long eCompatibilityType)
{
    avobj.vl_setocStbHostDVIHDMI3DCompatibilityControl(eCompatibilityType);
    return 1;
}

/**
 * @brief This function is used to display the message about DVI/HDMI 3D-Compatibility.
 *
 * @param[out] eStatus Address where message about DVI/HDMI 3D-Compatibility is stored.
 *
 * @return On success it returns 1, on failure it returns 0.
 */
unsigned int Sset_ocStbHostDVIHDMI3DCompatibilityMsgDisplay(long eStatus)
{
    avobj.vl_setocStbHostDVIHDMI3DCompatibilityMsgDisplay(eStatus);
    return 1;
}
// VL_HOST_SNMP_API_RESULT Sget_SystemInfo(VL_SNMP_SYSTEM_VAR_TYPE systype, void* _Systeminfo)
// {
//        if(vl_SnmpHostSysteminfo(systype, _Systeminfo) == 0)
//         {
//               return VL_HOST_SNMP_API_RESULT_SUCCESS;
//         }
//        else
//         {
        //
//              return VL_HOST_SNMP_API_RESULT_FAILED;
//         }
// }

/**
 * @brief This function is used to free the content of netsnmp table by deleting
 * rows in the table.
 *
 * @param[out] table_data netsnmp table whose content has to be deleted.
 *
 * @return None
 */
        void
                Table_free(netsnmp_tdata * table_data) {
            netsnmp_tdata_row *rowthis;

            while ((rowthis = netsnmp_tdata_row_first(table_data)))
            {
       // netsnmp_tdata_remove_and_delete_row(table_data, rowthis);
                netsnmp_tdata_remove_row(table_data, rowthis);
    //SNMP_FREE(rowthis);
            }

                }

/**
 * @brief This function is used to get the timeout count for the Carousel number.
 *
 * @param[out] pTimeoutCount Carousel Number.
 *
 * @return Returns 1 by default for success.
 */
                unsigned int Sget_ocStbHostOobCarouselTimeoutCount(int * pTimeoutCount)
                {
                    return avobj.vlGet_ocStbHostOobCarouselTimeoutCount(pTimeoutCount);
                }

/**
 * @brief This function is used to get the timeout count for the Carousel number
 * for InBand tuners.
 *
 * @param[out] pTimeoutCount Carousel Number.
 *
 * @return Returns 1 by default for success.
 */
                unsigned int Sget_ocStbHostInbandCarouselTimeoutCount(int * pTimeoutCount)
                {
                    return avobj.vlGet_ocStbHostInbandCarouselTimeoutCount(pTimeoutCount);
                }
               
/**
 * @brief This function is used to get the timeout count for Program Association
 * and Program Map Table (PAT & PMT).
 *
 * @param[out] pat_timeout PAT Timeout count.
 * @param[out] pmt_timeout PMT Time out count.
 *
 * @return Returns 1 by default for success.
 */ 
                unsigned int Sget_ocStbHostPatPmtTimeoutCount(unsigned long *pat_timeout, unsigned long *pmt_timeout)
                {
                    return avobj.vlGet_ocStbHostPatPmtTimeoutCount(pat_timeout, pmt_timeout);
                }

/**
 * @brief This function is used to get the info about Software Application for
 * STB.
 *
 * @param[in] nApps Total number of Applications whose information has to be retrieved..
 * @param[out] pAppInfolist Struct Pointer where info has to be stored.
 *
 * @return Returns 1 by default for success.
 */
                unsigned int Set_ocStbHostSwApplicationInfoTable(int nApps, SNMPHOSTSoftwareApplicationInfoTable_t * pAppInfolist)
                {
                    avobj.vlSet_ocStbHostSwApplicationInfoTable_set_list(nApps, pAppInfolist);
                    return 1;
                }
                rmf_Error vlMpeosSetocStbHostSwApplicationInfoTable(int nApps, VLMPEOS_SNMPHOSTSoftwareApplicationInfoTable_t * pAppInfolist)
                {
                    avobj.vlMpeosSet_ocStbHostSwApplicationInfoTable_set_list(nApps, pAppInfolist);
                    return RMF_SUCCESS;
                }

                rmf_Error vlMpeosSetocStbHostUserSettingsPreferedLanguage(char * pszHostUserSettingsPreferredLanguage)
                {
                    VL_SNMP_VAR_USER_PREFERRED_LANGUAGE userLang;
                    //vlMemSet(&userLang, 0, sizeof(userLang), sizeof(userLang));
                    memset(&userLang, 0, sizeof(userLang));
                    strncpy(userLang.UserSettingsPreferedLanguage, pszHostUserSettingsPreferredLanguage, sizeof(userLang.UserSettingsPreferedLanguage));
                    userLang.UserSettingsPreferedLanguage [ VL_MAX_SNMP_STR_SIZE - 1 ] = 0; 

                    vlSnmpSetHostInfo(VL_SNMP_VAR_TYPE_ocStbHostUserSettingsPreferedLanguage, &userLang);
                    return RMF_SUCCESS;
                }

/**
 * @brief This function is used to get the info about JVM for the host.
 *
 * @param[out] pJvmInfo Struct pointer where JVM info is stored.
 *
 * @return Returns 1 by default for success.
 */
                unsigned int Set_ocStbHostJvmInfo(SJVMinfo_t * pJvmInfo)
                {
                    avobj.vlSet_ocStbHostJvmInfo(pJvmInfo);
                    return 1;
                }
                
                rmf_Error vlMpeosSetocStbHostJvmInfo(VLMPEOS_SNMP_SJVMinfo_t * pJvmInfo)
                {
                    SJVMinfo_t jvmInfo;
                    //vlMemSet(&jvmInfo, 0, sizeof(jvmInfo), sizeof(jvmInfo));
                    memset(&jvmInfo, 0, sizeof(jvmInfo));
                    jvmInfo.JVMAvailHeap    = pJvmInfo->JVMAvailHeap;
                    jvmInfo.JVMDeadObjects  = pJvmInfo->JVMDeadObjects;
                    jvmInfo.JVMHeapSize     = pJvmInfo->JVMHeapSize;
                    jvmInfo.JVMLiveObjects  = pJvmInfo->JVMLiveObjects;
                    avobj.vlSet_ocStbHostJvmInfo(&jvmInfo);
                    return RMF_SUCCESS;
                }
                rmf_Error vlMpeosocStbHostPatPmtTimeoutCount(unsigned long nPatTimeout,unsigned long nPmtTimeout)
                {
                    avobj.vlSet_ocStbHostPatPmtTimeoutCount(nPatTimeout, nPmtTimeout);
                    return RMF_SUCCESS;
                }
                rmf_Error vlMpeosSetocStbHostOobCarouselTimeoutCount(int nTimeoutCount)
                {
                    avobj.vlSet_ocStbHostOobCarouselTimeoutCount(nTimeoutCount);
                    return RMF_SUCCESS;
                }
                rmf_Error vlMpeosSetocStbHostInbandCarouselTimeoutCount(int nTimeoutCount)
                {
                    avobj.vlSet_ocStbHostInbandCarouselTimeoutCount(nTimeoutCount);
                    return RMF_SUCCESS;
                }

