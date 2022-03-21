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





#ifndef __CM_API_H__
#define __CM_API_H__

#ifdef GCC4_XXX
#include <vector>
#include <string>
#else
#include <vector.h>
#include <string.h>
#endif
#include "rmf_osal_sync.h"
#include "semaphore.h"
#include "cardUtils.h"
#include "core_events.h"
#include "genfeature.h"
#include "cmevents.h"
#include "ip_types.h"

using namespace std;
//#include "tablemgr.h"  //for TableManagerTableReturnCode


//! Card Manager Interface API
/*! More Detailed description of this file
    The cCardManagerIF class is used by Applications to communicate with the CableCard.
    This has been implemented as a Singleton class so that all the requests to talk with the
    CableCard arrive at the right interface.
    This class handles the following
    - setting the Host's Display Capabilities
    - getting information on the Applications available on the CableCard
    - MMI interface - opening and closing a MMI session and sending a query to get data
      pertaining to a specific URL on the POD.
    - Controlling specific features on the CableCard and Host
    -

    //Note: The OOB PID filtering API interface is through the pfcTableManager interface



*/

//! \enum Card States
/*! Card States enum more detailed description
*/

class    CardManager;
class     cCardManagerIF;
typedef unsigned long IOResourceHandle;


#define MAX_SEC_FILTER_LEN 16 // just to maintain compatibility with inband section filters

typedef enum
{
    CARD_INSERTED,
    CARD_REMOVED,
    CARD_READY,
    CARD_ERROR,
    CARD_INVALID
}CardStatus_t;

typedef enum
{

    CM_CARD_REMOVED,//Set when card is removed
    CM_CARD_INVALID,
    CM_CARD_ERROR,//Set when card error occured
    CM_CARD_INSERTED,//Set as soon as card is inserted
    CM_CARD_READY,// Set when extended channel flow connection is established to receive the OOB tables..
}CMCardStatus_t;

typedef enum
{
    TYPE_CA,
    TYPE_CP,
    TYPE_IP,
    TYPE_DVS167,   //SCTE 55-2
    TYPE_DVS178,   //SCTE 55-1
    TYPE_DSG,
    TYPE_DIAGNOSTIC,
    TYPE_UNDESIGNATED

}PodAppType_t;

typedef enum
{
        EMI0=0,
        EMI1=1,
        APS0=2,
        APS1=4,
        CIT=8
}CCI_t;


typedef enum
{
    UNKNOWN_PATH,
    QAM_IB,
    QPSK_OOB
}UpgradeSrc_t;


typedef enum
{

    PCMCIA_RESET,
    POD_RESET,
    NO_RESET //this value with card_fw_upgrade_complete_event means POD cancelled Fw update

}ResetType_t;


typedef enum
{
    OOB1,
    IB1,
    DOCSIS

}DownloadType_t;



typedef enum
{
    ACK_NO_ERROR,
    INVALID_ID,
    PARAMETER_ERROR

}HostResponse_t;


typedef enum
{
    CM_SERVICE_TYPE_MPEG_SECTION,
    CM_SERVICE_TYPE_IP_U,
    CM_SERVICE_TYPE_IP_M,
    CM_SERVICE_TYPE_DSG,
    CM_SERVICE_TYPE_SOCKET_FLOW,
    CM_SERVICE_TYPE_IPDM,								// IP Direct Multicast
    CM_SERVICE_TYPE_IPDU,								// IP Direct Unicast
    CM_SERVICE_TYPE_INVALID_FLOW_SERVICE_TYPE = -1
}Service_type_t;


typedef enum
{
    FLOW_NOT_STARTED=-1,
    FLOW_CREATED=0x0,
    FLOW_DENIED=0x1,
    INVALID_SERVICE_TYPE=0x2,
    NETWORK_UNAVAILABLE=0x3,
    NETWORK_BUSY=0x4,
    FLOW_CREATION_IN_PROGRESS=0x111, //after APDU is sent
    FLOW_UNINITIALIZED=0x112, //Initial state for any oob filter
    EXTCH_RESOURCE_NOT_OPEN = 0x113,
    EXTCH_NO_RESPONSE = 0x114,
    EXTCH_RESOURCE_DELETE_DENIED = 0x115, // if another filter is waiting on same PID - delete_flow_req is not called
    EXTCH_PID_FLOW_EXISTS = 0x116 // not a error condition, but a existing flow is used for this case
}XFlowStatus_t;

typedef enum
{
    FILE_OK,
    URL_NOT_FOUND,
    URL_ACCESS_DENIED,
    STATUS_INVALID=-1

}ServerReplyStatus_t;


typedef enum
{
    MMI_DIALOG_OK,
    MMI_HOST_BUSY,
    MMI_WRONG_DISPLAY_TYPE,
    MMI_NO_VIDEO_SIGNAL,
    MMI_NO_WINDOWS

}MmiStatus_t;


typedef int HostId_t;
typedef int HardwareId_t;
typedef int UIMode;
typedef int CaSystemId_t;

typedef enum
{
 CM_NO_ERROR,
 CM_ERROR_NO_CABLECARD,
 CM_INVALID_PARAMETER, // if any of the parameters passed to the API are not correct. Eg. if a null pointer is passed
 CM_DATA_NOT_AVAILABLE,
 CM_EXTCH_FLOWS_NOT_AVAILABLE,
 CM_EXTCH_FLOW_GRANTED,
 CM_EXTCH_FLOW_DENIED,
 CM_ERROR_INVALID_SECTION_TYPE,
 CM_ERROR_NO_FLOW_EXISTS,
 CM_ERROR_FILTER_NOT_STARTED,
 CM_SEND_APDU_ERROR,
 CM_BUFFER_NOT_ALLOCATED

}CMStatus_t;

typedef enum
{
    CM_TUNING_DENIED,   //used with Host Control APIS
    CM_TUNING_ALLOWED   //used with Host Control APIS

}Tuner_response_t;


typedef enum
{
    NO_ACTION,
    START_PID_COLLECTION,
    STOP_PID_COLLECTION
}action_t;


typedef enum
{
    CM_IGNORE,
    CM_DO_NOT_USE_DAYLIGHT_SAVINGS,
    CM_USE_DAYLIGHT_SAVINGS

}Daylight_Savings_t;


typedef enum
{
    CM_USE_USER_SETTING,
    CM_SWITCHED,
    CM_UNSWITCHED //always on

}AC_Outlet_t;


typedef enum
{
    CM_FORBIDDEN,
    CM_UNITED_STATES,
    CM_CANADA
}Rating_Region_t;


typedef enum
{
    CM_DO_NOT_RESET_PIN,
    CM_RESET_PARENTAL_CONTROL_PIN,
    CM_RESET_PURCHASE_PIN,
    CM_RESET_BOTH_PINS

}Reset_Pin_t;


typedef enum
{
    CM_UNDEFINED,
    CM_WEB_PORTAL_URL,
    CM_EPG_URL,
    CM_VOD_URL

}Url_Type_t;


typedef enum
{
    CM_FEATURE_ACCEPTED,
    CM_FEATURE_NOT_SUPPORTED,
    CM_FEATURE_INVALID_PARAMETER, //invalid parameter
    CM_FEATURE_DENIED

}Feature_Control_Status_t;


extern "C" char * cExtChannelGetFlowStatusName(XFlowStatus_t status);
extern "C" int vlTransferCcAppInfoPage(const char * pszPage, int nBytes);
extern "C" int vlGetCcAppInfoPage(int iIndex, const char ** ppszPage, int * pnBytes);
extern "C" int rmfStartCableCardAppInfoPageMonitor();
extern "C" int vlMCardGetCardVersionNumberFromMMI(char * pszVersionNumber, int nCapacity);


//! \This function is used to inform the CableCard of the Host's Display Capability.
//! \ The get functions return the current/default values and the set is used to change them

class cDisplayCapabilities
{

public:
    cDisplayCapabilities();

    ~cDisplayCapabilities();

    void    SetRows(int num_rows);

    void     SetColumns(int num_columns);

    void    SetScrolling(int vert, int hor);

    void    SetDisplayTypeSupport( int multi_window_cap);

     void   SetDataEntrySupport(int data_entry_cap);


     void    SetHTMLSupport(int htmlSupport,int link,int form,int table,int list, int image);




    int   GetRows();

    int     GetColumns();

    void    GetScrolling(unsigned char &vert, unsigned char &hor);


    int    GetDisplayTypeSupport();

     int   GetDataEntrySupport();


     void    GetHTMLSupport(unsigned char &htmlSupport,unsigned char &link,unsigned char &form,unsigned char &table,unsigned char &list, unsigned int &image);

      void    GetHTMLSupport(unsigned char &htmlSupport);
private:

    int    DisplayRows; // No of rows the Host device can support.  If the host
                // supports more than 255, set this value equal to 0xFF
    int    DisplayColumns; // No of cols the Host device can support. If the Host
                // supports more than 255, set this value equal to 0xFF
    int    verticalScrolling; // If the Host supports vertical scrolling.
                  // Default value = 0
                  // 0x00 vertical scrolling not supported
                  // 0x01 Vertical scrolling supported
                  // 0x02-0xFF reserved
    int    horizontal_scrolling;    // If the Host supprots horizontal scrolling.
                  // Default value = 0
                  // 0x00 horizontal scrolling not supported
                  // 0x01 horizontal scrolling supported
                  // 0x02-0xFF reserved

    int    display_type_support; // Window capability of the Host.
                  // 0x00 Full screen. Host supports full screen for MMI
                      //      screens
                  // 0x01 overlay. Host supports Overlay Windows for MMI
                  //       Screens
                  // 0x02-0x6F Multiple Windows. Host supports multiple
                  //      simultaneous MMI windows. The value equals the
                  //      max. no. of simultaneous open windows the host can
                  //       support.

    int    data_entry_support; // Defines the preferred data entry capability of the
                    // Host.
                    // 0x00 None
                    // 0x01 Last/Next
                    // 0x02 Numeric Pad
                    // 0x03 Alpha keyboard with mouse
                    // 0x04-0xFF reserved
    int     HTML_Support;       // HTML support capability of the Host. All Hosts SHALL
                    // support a min. baseline HTML profile.
                    // 0x00 Baseline profile
                    // 0x01 Custom profile
                    // 0x02 HTML 3.2
                    // 0x03 XHTML 1.0
                    // 0x04-0xFF reserved.
    int    link_support;       // Whether Host can support single or multiple links.
                    // 0x00 one link
                    // 0x01 Multiple links
                    // 0x02-0xFF reserved.
    int    form_support;        // Form support capability of the Host
                    // 0x00 None
                    // 0x01 HTML 3.2 w/o POST method
                    // 0x02 HTML 3.2
                     // 0x03-0xFF reserved
    int    table_support;        // Table support capability of the Host
                    // 0x00 None
                    // 0x01 HTML 3.2
                     // 0x02-0xFF reserved
    int    list_support;        // List support capability of the Host
                    // 0x00 None
                    // 0x01 HTML 3.2 w/o descriptive lists
                    // 0x02 HTML 3.2
                     // 0x03-0xFF reserved
    int    image_support;         // Image support capability of the Host
                    // 0x00 None
                    // 0x01 HTML 3.2 PNG picture under RGB w/o resizing
                    // 0x02 HTML 3.2
                     // 0x03-0xFF reserved
};


//! /This functions gives information on the applications present in the CableCard.
//! / It gives the name of the application,, URL, versionNumber and Manufacturer ID
class cApplicationInfoList
{
public:
    friend class cAppInfo;
    cApplicationInfoList(uint16_t urlLength, uint16_t nameLength);
    cApplicationInfoList();
    ~cApplicationInfoList();


// each of these functions returns CM_ERROR_NO_CABLECARD  until card is inserted and app_info_cnf is received
    uint16_t   getURLLength();
    CMStatus_t getURL(uint8_t *); //callers responsibility to ensure that the buffer size == urlLength
    uint16_t   getNameLength();
    CMStatus_t getName(uint8_t *);//callers responsibility to ensure that the buffer size == nameLength
    PodAppType_t     getType();
    int getVersionNumber();
    CMStatus_t  getManufacturerID();


    //Following used by CardManager to populate the private data members of this class
    void setURLLength(uint16_t length);
    void setURL(uint8_t []);
    void setNameLength(uint16_t length);




private:

    //void ProcessAppInfoCnf(uint8_t *); //populates the URL,Name, version# and type for this app

    uint8_t        *URL;
    uint16_t    urlLength; //in bytes
    uint8_t        *Name;
    uint16_t    nameLength;
    int        version;   //app version #
    int        type;

};






class cRfOutput : public cGenericFeatureBaseClass
{

public:

    cRfOutput();
    cRfOutput(uint8_t, uint8_t);
    cRfOutput(uint8_t, uint8_t, uint8_t *);
    ~cRfOutput();



    uint8_t            rf_outputChannel;
    uint8_t            rf_outputChannelUI;

};








class    cFeatureControl
{
public:

    cFeatureControl();
    ~cFeatureControl();
    void getHostFeatureList(uint8_t &);  //complete list
    void getHostParam(int featureID, uint8_t &); //returns the values pertaining to this feature ID
    bool setHostParam(int featureID, uint8_t &); //used to update the features by sending the APDU to CableCard //
    void BuildUpdatedFeatureList(uint8_t featureList); //list of updated features
    void BuildAllFeaturesList(uint8_t featureList); //list of all supported features
     void NotifyOCAP(); //OCAP App seems to need to approve any feature change requested by the Host or POD

    cRfOutput    fc_rfOutputObj;



private:

    uint8_t       * hostFeatureList;  // feature list supported by this host device.
    uint8_t       * cardFeatureList;  // feature list provided by the card device.

//see CCIF2.0 for the Feature IDs and related definitions

/*
    uint8_t            rf_outputChannel;
    uint8_t            rf_outputChannelUI;

    uint8_t            p_c_pinLength;
    uint8_t            *p_c_pinData;

    struct
    {
    uint8_t        reserved; //4bits
    uint16_t    maj_ch_num; //10 bits
    uint16_t    min_ch_num; //10 bits

    }p_c_channel;
    uint8_t            p_c_factory_reset;
    uint16_t        p_c_channel_count;
    struct p_c_channel    *p_c_channel_value[];

    uint8_t            purchase_pin_length;
    uint8_t            *purchase_pin_chr;

    uint16_t        time_zone_offset; //two's complement integer offset, in number of minutes from UTC.

    Daylight_Savings_t    daylight_savings_control;

    AC_Outlet_t        ac_outlet_control;

    uint32_t        language_control;

    Rating_Region_t        rating_region_setting;

    Reset_Pin_t        reset_pin_control;

    uint8_t            number_of_urls;

    struct
    {
        uint8_t    url_type;
        uint8_t url_length;
        uint8_t    *url;

    }cable_url;

    cable_url        *cable_url_array[];

    uint8_t            state_code;
    uint8_t            county_subdivision;
    uint16_t            county_code;
*/
    //CreatePodFeatureParams(event);
    //CreateCardFeatureList(event);
};




///This is applicable only for Native app case, as OCAP sends the APDUs directly
class  cMMIControl
{ //open_mmi_req , close_mmi_req, server_reply are sent as Events
friend class cMMI; //temp
public:
    cMMIControl();

    cMMIControl(uint8_t *urlStr,uint16_t length );
    ~cMMIControl();
    void openMMICnf(int dialogNum, int status, int ltsid);  //! Sends response to a open_mmi_req event
    void closeMMICnf(int dialogNum, int ltsid ); //! Sends response to a close_mmi_req event or Host can send this in response to the user
                            //closing the MMI window

    int serverQuery(int dialogNum, uint8_t *url, uint16_t urlLength, uint8_t *header, uint16_t headerLength);
    void setUrl(uint8_t *ptr, uint16_t len);
    uint16_t getUrlLength();
    uint8_t *getUrl();
    int    getDialogNum();

private:
    uint8_t    *url;
    uint16_t urlLength;
    int    ltsid;
    int    dialogNum;
    int     streamType;

};


// changed: Aug-06-2007: typedef for XChan_cb changed.
typedef  int (*XChan_cb) (void *obj, void *buf, int len, unsigned long * retValue);
#define MAX_SEC_FILTER_LEN 16

class cPodPresenceEventListener;

//! The caller creates a new cExtChannelFlow object, pushes it into the list
//! calling RequestNewFlow sends the APDU to CableCard
class    cExtChannelFlow
{

public:
friend class cExtChannelThread;

    cExtChannelFlow();
    cExtChannelFlow(cCardManagerIF *pCMInterface);
    ~cExtChannelFlow();
    void SetServiceType(Service_type_t);
    //! Sets the service type for this extended channel flow

    Service_type_t GetServiceType(void);
    //! Gets the service type of this extended channel flow

    int  GetPID();
    //! Get the PID related to this flow - returns -1 if it is not a MPEG_SECTION flow

    void SetPID(int);
    //! Sets the PID for the flow

    uint32_t GetFlowID();
    void    ResetFlowID();
    uint8_t     GetFlowsRemaining();
    uint8_t     GetStatus();
    void     SetMacAddress(uint8_t *addr);
    int32_t GetIPAddress();
    VL_IP_IF_PARAMS GetIpuParams();
    void     SetOptionByteLength(uint8_t);
    void     SetOptionBytes(uint8_t *data);// if length is not set before this call using SetOptionByteLength , return a error
    void    SendIPData(uint8_t *data, uint32_t length, uint32_t flowID);
    
    int  RequestExtChannelFlow(); // adds this object to the extChannelList in cCardManagerIF
                                   // status of adding this flow is sent through a event

    static void vlRequestExtChannelFlowTask(void * arg);

    static int RequestExtChannelFlowWithRetries(cPodPresenceEventListener * pPodPresenceEventListener,
                                                Service_type_t eServiceType,
                                                unsigned long nRetries = -1);
                                    // adds this object to the extChannelList in cCardManagerIF
                                    // status of adding this flow is sent through a event
                                    // starts a new thread
                                   
    int  DeleteExtChannelFlow(); // deletes this object from the extChannelList in cCardManagerIF
                                   // status of deleting this flow is sent through a event

    int StartFiltering(void); //Ext Ch data pertaining to this flowID is ignored until StartFiltering
    int StopFiltering();
    static void ReadData(uint8_t *extendedData, uint32_t dataLen);

    // changed: Aug-06-2007: type of cb_fn changed.
    XChan_cb    cb_fn; //if callback is null, event mechanism is used to convey data availability
    void        *cb_obj; // a object used with the callback

    /**
* Filter values.
 */
      uint8_t     filter_value[MAX_SEC_FILTER_LEN];


/**
 * Used to define matching and non-matching bits that need to be 'matched'.
 */
    uint8_t     maskandmode[MAX_SEC_FILTER_LEN];

/**
 * Used to define matching and non-matching bits that need to be 'matched'.
 */
    uint8_t     maskandnotmode[MAX_SEC_FILTER_LEN];

    int         numBuffs;
    int         max_section_size;
    uint8_t        *buffer_array;// for cases where a buffer_array is provided by the caller who is setting the filter params
    void            * is_buffer_free_cb;
    uint8_t        *defaultBuffer;

    enum { INVALID_FLOW_ID = -1};

private:

    Service_type_t    serviceType;
    static uint8_t    flows_Remaining;
    XFlowStatus_t    status;
    int        flowID;

    VL_IP_IF_PARAMS ipu_flow_params;
    uint32_t    pid;
    uint8_t        optionBytesLength;
    uint8_t        *optionBytes;
    action_t    action;
    uint8_t        *dataPtr;
    uint32_t    dataLength;

    cCardManagerIF  *pCMIF;
    unsigned long   m_nRetries;
    cPodPresenceEventListener * m_pPodPresenceEventListener;


};





//Here is how the singleton class is to be used
    //cCardManagerIF *pCM = cCardManagerIF->Instance();
    // ......

typedef unsigned int (RawAPDU_cb) (uint8_t *apduData, uint32_t dataLen);    //if callback is null, event mechanism is used to convey data availability

//Added by Mamidi for SNMP Purpose


/*
 * enums  ocStbHostCAType
 */

typedef enum
{
    CATYPE_OTHER    =    1,
    CATYPE_EMBEDDED    =    2,
    CATYPE_CABLECARD    =    3,
    CATYPE_DCAS    =    4,
}ocStbHostCAType;


typedef struct
{
     unsigned char SecurityID[5];
     unsigned short CASystemID;
    ocStbHostCAType  HostCAtype;
}HostCATypeInfo_t;
#define VL_CARD_STR_SIZE 512
typedef struct vlCableCardCertInfo
{
    char szDispString[VL_CARD_STR_SIZE];  // String" Displays "M-card Host Certificate Info"
    char szDeviceCertSubjectName[VL_CARD_STR_SIZE];// subject string
    char szDeviceCertIssuerName[VL_CARD_STR_SIZE];// Issuer string
    char szManCertSubjectName[VL_CARD_STR_SIZE];// subject string
    char szManCertIssuerName[VL_CARD_STR_SIZE];// Issuer string
    unsigned char acHostId[5]; // Host ID of the device
    unsigned char bCertAvailable;// 1 if certificates are not copied
    unsigned char bCertValid; // 1 if certificates are valid
    unsigned char bVerifiedWithChain;// 1 if cert chain is verified successfully
    unsigned char bIsProduction;// 1 for production keys
}vlCableCardCertInfo_t;

#define VL_MAX_CC_APPS              10  // Max 10 cable card pages
#define VL_MAX_CC_APP_SUB_PAGES     20  // Max 20 cable card sub-pages

typedef struct vlCableCardApp
{
   unsigned char AppType;// Application type
   unsigned short VerNum;// Application version
   unsigned char AppNameLen;// Application name length
   unsigned char AppName[256];// Applcation name array of max 256
 //  unsigned char AppUrlLen; // Application Url length
   int AppUrlLen; // Application Url length
   unsigned char AppUrl[256];//Application Url array of max 256
 // unsigned char *AppUrl;//Application Url array of max 256
    int nSubPages;

}vlCableCardApp_t;

typedef struct vlCableCardAppInfo
{
    unsigned short CardManufactureId;// 2 byte cable card Manufacture Id
    unsigned short CardVersion; //  2 buy cable card version number
    unsigned char CardMAC[6];
    unsigned char CardSerialNumberLen;// Cable card Serial number length
    unsigned char CardSerialNumber[256]; // Cable card serial number array of 256 max
    unsigned char CardRootOidLen;// Cable card Root OID length
    unsigned char CardRootOid[256]; // Cable card Root OID array max length 256
    unsigned char CardNumberOfApps;// number of cable card applications
    vlCableCardApp_t Apps[12];// Max 64 cable card applications
}vlCableCardAppInfo_t;

typedef struct vlCableCardVersionInfo
{
    char szCardVersion[256]; //  2 buy cable card version number
}vlCableCardVersionInfo_t;

typedef struct vlCableCardIdBndStsOobMode
{

    unsigned char CableCardId[32]; // Cable card Id string... of 17 bytes
    int CableCardBindingStatus;// 1 unkonw, 2 invalid cert, 3 other failures, 4 bound
    int OobMessMode; // 1 scte55, 2 dsg, other 3
}vlCableCardIdBndStsOobMode_t;
typedef struct vlCableCardId
{

    unsigned char CableCardId[32]; // Cable card Id string... of 17 bytes
    unsigned char CableCardIdBytes[8]; // Cable card Id byte array... of 8 bytes
}vlCableCardId_t;

typedef struct vlCableCardDetails
{
    unsigned short CardManufactureId;// 2 byte cable card Manufacture Id
    unsigned short CardVersion; //  2 buy cable card version number
    unsigned char CardMAC[6];
    unsigned char CardSerialNumberLen;// Cable card Serial number length
    unsigned char CardSerialNumber[256]; // Cable card serial number array of 256 max

}vlCableCardDetails_t;


int GetCableCardCertInfo(vlCableCardCertInfo_t *pCardCertInfo);
class cCardManagerIF
{



public :
    typedef enum
    {
        CM_IF_CA_RES_ALLOC_SUCCESS,
        CM_IF_CA_RES_BUSY_WITH_LTSID_PRG,
        CM_IF_CA_RES_ALLOC_FAILED_ALL_RES_BUSY,
        CM_IF_CA_RES_DEALLOC_SUCCESS,
        CM_IF_CA_RES_DEALLOC_FAILED,
        CM_IF_CA_RES_NO_CABLE_CARD,
    }CM_IF_CA_RES_STATUS;

    static cCardManagerIF*  getInstance();
    cCardManagerIF*     deleteInstance();

    CardManager *            CMInstance;

    //APPLICATION_INFO
    cDisplayCapabilities    displayCapabilities;


    vector<cApplicationInfoList *> AppInfoList;    //initialized when a app_info_cnf is received.
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
//
    cFeatureControl    featureControl;

    vector<cMMIControl *>    mmiFlowList;

    //vector<cExtChannelFlow *> extChannelList;
    //pfcSemaphore         extChSem;
    static rmf_osal_Mutex         extChMutex;
    static sem_t    extChSem;

    unsigned char GetCableCardState(); //returns CARD_REMOVED if card not present and CARD_INSERTED if card is present
    //GetCableCardFeatures (INPUT pList: min size of 32 bytes,INPUT pSize pass the size of pList in bytes
    //OUTPUT pSize: pass the number of Card Feature params received from the card.
//    int GetCableCardFeatures(unsigned char *pList, unsigned char *pSize);
    void SetDisplayCapabilities();

    void SetUIMode(UIMode mode);//used to set the UI Mode to be either Native or OCAP

    void AssumeOCAPHandlers(bool);
    void AssumeCaAPDUHandlers(bool);

    /* If a app takes over MMI handling it needs to call this function */
    /* Otherwise, the cardmanager itself provides some minimum handling */
    void AssumeMMIHandler(bool);
    /* The app needs to call this if it is no longer going to handle mmi*/
    void ReleaseMMIHandler(bool);

    bool IsMMIhandlerPresent();

    RawAPDU_cb            *cb_fn; //if callback is null, event mechanism is used to convey data availability

    void SetHostId(HostId_t Id);//used by the CableCard to parse Code Version Tables such that the host can be informed when new code versions are available

    void SetHardwareId( HardwareId_t HardwareId);//The Hardware ID in conjunction with the Vendor ID is used by the CableCard to parse Code Version
                            //Tables such that the host can be informed when new code versions are available.

    void NotifySystemTimeValid(void);          //used to indicate to the Card Manager, that the System Time has been set to a valid time.
                           // This notification is required because certain CableCard resources, such as System Time Resource,
                           // cannot initialize properly until System Time has been set.

    void NotifyHostLowSpeedReturnSupported(void); // This API is used to indicate to the Card Manager that the host supports a Low-Speed Return channel
                             // via DOCSIS. If this API is not called then the Card Manager will assume DOCSIS Return is not
                             // available and default to OOB return.

    CaSystemId_t GetCASystemId(void);           //query the CA System ID. The Card Manager manages the querying of this value internally and makes
                           //the result available to the interface. The Card Manager also handles the state of this parameter

                            // when CableCard is removed/re-inserted.


    void    NotifyChannelChange( IOResourceHandle    hIn, unsigned short usVideoPid,unsigned short usAudioPid);

    bool GetCCIInformation( int ProgramIndex, CCI_t &CCI );




    CMStatus_t SendAPDU(short SesnId, int tag, uint8_t *apdu, uint32_t apduLength); // response is sent through a pfc event

    void GetCardStatus( CardStatus_t& Status );//NOT Implemented

// RAJASREE: 091306: Added new methods needed by OCAP [start]
//    void GetApplicationInfo(CApplicationList &list);
    int GetCardManufacturerId();
    int GetCardVersionNumber();
    void GetApplicationInfo(vector<cApplicationInfoList *> &list);
    //This function returns the the recent bufferred MMI received from the cable card
    // Memory will be allocated by the called function , caller NEED NOT free the memory after using it..
    //Returns "0" on success with buffer pointer and size of the message, returns non-zero value on error or no
    //MMI message

    int GetLatestMMIMessageFromCableCard(unsigned char **pData,unsigned long *pSize);
       //Added by Mamidi for SNMP purpose
    unsigned int SNMPGet_ocStbHostSerialNumber(char* SerialNumber);
     int SNMPGet_ocStbHostHostID(char* HostID,char len);
     int SNMPGet_ocStbHostCapabilities(int*  HostCapabilities_val); //enum
     int SNMPget_ocStbHostSecurityIdentifier(HostCATypeInfo_t* HostCAtypeinfo);

     int SNMPGetCableCardCertInfo(vlCableCardCertInfo_t *pCardCertInfo);
    int SNMPGetApplicationInfo(vlCableCardAppInfo_t *pCardAppInfo);
    bool vlGetPodMmi(string & rStrReply, const unsigned char* message, int msgLength);
    bool vlGetPodSingleMmi(string & rStrReply, const unsigned char* message, int msgLength);
    bool vlGetPodMmiTable(int iPage, string & rStrReply, int nSnmpBufLimit);
    int SNMPGetApplicationInfoWithPages(vlCableCardAppInfo_t *pAppInfo);
    int SNMPGetCardIdBndStsOobMode(vlCableCardIdBndStsOobMode_t *pCardInfo);
    int SNMPSendCardMibAccSnmpReq( unsigned char *pMessage, unsigned long MsgLength);
    int GetCableCardId(vlCableCardId_t *p);// Gives Card Id in string and 8 byte data
    int GetCableCardDetails(vlCableCardDetails_t *pCardDetails);//Gives Manufacturer Id, Version Serial Number
    int GetCableCardVctId(unsigned short *pVctId);//Gives Vct Id received by card, default value is "0" if not received by card
    int SNMPGetGenFetResourceId(unsigned long *pRrsId);// Returns the ResourceId of the generic feature parameter
    int SNMPGetGenFetTimeZoneOffset(int *pTZOffset);// Returns the Time zone offset of the generic feature parameter
    int SNMPGetGenFetDayLightSavingsTimeDelta(char *pDelta);
    int SNMPGetGenFetDayLightSavingsTimeEntry(unsigned long *pTimeEntry);
    int SNMPGetGenFetDayLightSavingsTimeExit(unsigned long *pTimeExit);
    int SNMPGetGenFetEasLocationCode(unsigned char *pState,unsigned short *pDDDDxxCCCCCCCCCC);//
    int SNMPGetGenFetVctId(unsigned short *pVctId);
    int SNMPGetCpInfo(unsigned short *pCpInfo);
    int SNMPGetCpAuthKeyStatus(int *pKeyStatus);//pKeyStatus = 1 ready , 2 not ready
    int SNMPGetCpCertCheck(int *pCerttatus);//pCerttatus = 1 OK , 2 failed
    int SNMPGetCpCciChallengeCount(unsigned long *pCount);
    int SNMPGetCpKeyGenerationReqCount(unsigned long *pCount);
    int SNMPGetCpIdList(unsigned long *pCP_system_id_bitmask);

    //

/* Returns the 0 on successful allocation */
    int vlGetCaFreeResource(int *pResourceHandle);
/* Returns the 0 on successful release*/
    int vlReleaseCaResource(int ResourceHandle);

/* Returns  CM_IF_CA_RES_ALLOC_SUCCESS on successful allocation */
/* Returns  CM_IF_CA_RES_BUSY_WITH_LTSID_PRG if the resource is already allocated with given ltsid and prg number
the called will get the resource handle, it means the caller is granted the resource for given ltsid and program number
but the caller  should not call the ca_pmt call to the cardmanager */
/* Returns CM_IF_CA_RES_ALLOC_FAILED_ALL_RES_BUSY if all the CA resources are busy */
int vlAllocateCaFreeResource(int *pResourceHandle,unsigned char Ltsid, unsigned short PrgNum);
/* Returns  CM_IF_CA_RES_DEALLOC_SUCCESS on successful release*/
/* Returns CM_IF_CA_RES_DEALLOC_FAILED if failed to release the resource */
int vlDeAllocCaResource(int ResourceHandle);
int InvokeCableCardReset();
// RAJASREE: 091306: Added new methods [end]

    int  cableCardVersion;
    int  mfgID;
    unsigned char cableCardSerialNumLen;
    unsigned char cardMAC[6];
     void test(){


        //RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "cCardManagerIF::test\n");
    }

    int vlGetCMCciCAenableFlag( unsigned char Ltsid,unsigned short PrgNum, unsigned char *pCci,unsigned char *pCAEnable);

private:
    cCardManagerIF();
    ~cCardManagerIF(); // do not do a delete _instance  in the destructor as it can get called indefinitely.
    CaSystemId_t            caID;

    static    cCardManagerIF          *_instance;
    static  rmf_osal_Mutex        mutex;
    int                refCount;
    bool                mmi_handler;

};








class  cHostTunerControl
{
public:
    void TuningResponse(Tuner_response_t ); //

private:
    int    tuneRequestID;

};





class    cFWUpgradeControl
{

    //oob_tune and inb_tune host control requests are sent as events
public:
    void card_getUserText(int *length, uint8_t *data); //used with card_fw_upgrade_needed_event
    void card_upgrade_start();    // this is translated by CardManager into the firmware_upgrade_reply APDU


private:

};

class    cSystemControl
{
public:

    void host_sendInformation();
    void host_cvt_ack(HostResponse_t response);

private:
    int    vendor_ID;
    uint32_t    hw_ver_ID;


};


class  cDiagnostics
{
public:
    bool GetDiagnosticParameter(uint8_t &Parameter);
    bool ForceCardReset(int ResetType);

private:


};



typedef enum{

    PFC_EVENT_CATEGORY_CardManagerHCEvents=40,//Host Control


    PFC_EVENT_CATEGORY_CardManagerHomingEvents=44,//Homing
    PFC_EVENT_CATEGORY_CardManagerGFCEvents=46, //generic feature control
    PFC_EVENT_CATEGORY_CardManagerSysControlEvents=47, //system control

    PFC_EVENT_CATEGORY_CardManagerCPEvents=49 //Card Manager CP events
}pfcEventCategoryTypes_CM;






typedef enum{
        card_xchan_lost_flow_ind,
        card_xchan_data_available,
        card_xchan_resource_opened,
        card_xchan_start_oob_ip_over_qpsk,
        card_xchan_stop_oob_ip_over_qpsk,
        card_xchan_start_ipdu,						// IPDU register flow
        card_xchan_stop_ipdu
}eventTypes_XChan;



typedef struct {
    int        flowID;
    uint8_t        flowsRemaining;
    uint32_t    extChDataLength;
    uint8_t        *extChDataPtr;
    uint8_t        status;
    void         *obj;
}EventCableCard_XChan;

void EventCableCard_XChan_free_fn(void *);




typedef enum{

        card_mmi_open_mmi_req,
        card_mmi_close_mmi_req,
        card_mmi_open_mmi_cnf_sent
}eventTypes_MMI;



typedef struct
{

      int     dialogNum;
      uint8_t  *url;        //used with server query (url address )
      uint16_t urlLength;   // url length   

}EventCableCard_MMI;
void EventCableCard_MMI_free_fn(void *ptr);

typedef enum{
    //Host Control related
        card_server_reply, //! event type used to indicate that the file data corresponding to the url requested is available or error is specified
        card_app_info_avl  //!event type used to indicate that the AppInfoList member of cCardManagerIF singleton object  has the valid App Info data.

    }eventTypes_AI;


typedef struct
{

    int             dialogNum;         //valid only with Server Reply
    ServerReplyStatus_t     fileStatus;        //valid only for Server Reply
    uint8_t             *file;            //used only with Server Reply to specify url file data
    uint16_t         fileLength;           //specifies number of bytes in member variable url
    uint8_t            *header;        //used only with Server Reply
    uint16_t        headerLength;        //used only with Server Reply

}EventCableCard_AI;

void EventCableCard_AI_free_fn(void *ptr);


typedef enum{

         card_Raw_APDU_response_avl,
        card_Raw_APDU_send_error_response


}eventTypes_RawAPDU;



void EventCableCard_RawAPDU_free_fn(void *ptr);

typedef struct
{

    uint8_t     *resposeData;
    uint32_t dataLength;
    short  sesnId;
}EventCableCard_RawAPDU;


class pfcEventCableCard_FeatureControl
{

  public:
      typedef enum{

         card_feature_list_changed,
        card_feature_list


    }eventTypes_FeatureControl;

      pfcEventCableCard_FeatureControl ();

     ~pfcEventCableCard_FeatureControl ();

    /** Debug function: must be overloaded.
   *
   * \return A string literal for this event.
   */
    //virtual const char *get_type_string ();

};




class pfcEventCableCard_System
{


  public:
      enum  eventTypes_System
    {

         card_inserted_event,
        card_removed_event,
        card_invalid_event,
        card_error_event

    };



     pfcEventCableCard_System ();

     ~pfcEventCableCard_System ();

    /** Debug function: must be overloaded.
   *
   * \return A string literal for this event.
   */
    //virtual const char *get_type_string ();

};

class pfcEventCableCard_CP
{



  public:


    typedef enum{
    //to be defined

    }CP_ERROR;
    unsigned char CCI_Data;
      typedef enum{

         card_cci_changed_event,
        card_cp_error_event        //

    }eventTypes_SysControl;

      pfcEventCableCard_CP ();

     ~pfcEventCableCard_CP ();

    /** Debug function: must be overloaded.
   *
   * \return A string literal for this event.
   */
    //virtual const char *get_type_string ();

};

int GetHostIdLuhn( unsigned char *pHostId,  char *pHostIdLu );


#endif



