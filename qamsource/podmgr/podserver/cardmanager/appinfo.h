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
 

  
#ifndef __APPINFO_H__
#define __APPINFO_H__
#include "core_events.h"
#include "cmThreadBase.h"
//
//#include "pfcresource.h"
#include "rmf_osal_event.h"
#ifdef GCC4_XXX
#include <list>
#else
#include <list.h>
#endif
#include "cm_api.h"


#ifdef __cplusplus
extern "C" {
#endif
extern void aiProc(void* par);
extern void aiInit(void );

typedef struct //struct as required by the POD stack
{
    unsigned int  header;         // 24-bit tag & 8-bit APDU byte length
                            //    nb: UI should *NOT* fill this field

    unsigned short  displayRows;    // number of displayable rows (vert pixels), if value is more than 255 then set 0xFF
    unsigned short  displayCols;    // number of displayable columns (horiz pixels),if value is more than 255 then set 0xFF
    unsigned char   vertScroll;     // boolean indication of vertical scroll support, 
                    //0x00 vertical scroll not supported
                    //0x01 vertical scroll is supported
    unsigned char   horizScroll;    // boolean indication of horizontal scroll support
                   //0x00 horizontal scroll not supported
                    //0x01 horizontal scroll is supported
    unsigned char   multiWin;       // window display capability:
                            //    use values defined in eMultiWin_t above
    unsigned char   dataEntry;      // data entry capability:
                            //    use values defined in eDataEntry_t above
    unsigned char   html;           // HTML rendering capability:
                            //    use values defined in eHtml_t above

        // remaining fields only applicable if 'html' == 1 (eCustom)
    unsigned char   link;           // single (0)/multiple (1) link capability
    unsigned char   form;           // forms capability:
                            //    use values defined in eForm_t above
    unsigned char   table;          // no (0)/HTML 3.2 table (1) capability
    unsigned char   list;           // list capability:
                            //    use values defined in eList_t above
    unsigned int    image;          // image capability:
                            //    use values defined in eImage_t above
} pod_appInfo_t;
extern void    POD_Send_app_info_request(pod_appInfo_t    *pInfo);
#ifdef __cplusplus
}
#endif


#if 0

class cDisplayCapabilities_t;
 






class cDisplayCapabilities_t{
public:

    cDisplayCapabilities_t(){
    RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.POD", "cDisplayCapabilities\n");
    DisplayRows=502;
    DisplayColumns=566;
    verticalScrolling=0;
    horizontal_scrolling=0;
    multi_window_support=0;
    data_entry_support=0;
    HTML_Support= 1/*TRUE*/;
    link_support=0;
    form_support=0;
    table_support=0;
    list_support=0;
    image_support=0;
    
    };
    ~cDisplayCapabilities_t();
//private:    
    int    DisplayRows;
    int    DisplayColumns;
    int    verticalScrolling;
    int    horizontal_scrolling;
    int    multi_window_support;
    int    data_entry_support;
    bool    HTML_Support;
    int    link_support;
    int    form_support;
    int    table_support;
    int    list_support;
    int    image_support;
    
    void    SetRows(int num_rows){
        DisplayRows = num_rows;
    }
    void     SetColumns(int num_columns){
        DisplayColumns = num_columns;
            
    }
    void    SetScrolling(int vert, int hor){
        horizontal_scrolling= hor;
        verticalScrolling = vert;
    
    }
    
    void    SetMultiWindowSupport( int multi_window_cap){
        multi_window_support = multi_window_cap;
    
    }
     void   SetDataEntrySupport(int data_entry_cap){
         data_entry_support = data_entry_cap;
     
     }
     
     void    SetHTMLSupport(bool htmlSupport,int link = 0,int form=0,int table=0,int list=0, int image=0){
         if(HTML_Support = htmlSupport){
            link_support = link;
            form_support = form;
            table_support = table;
            list_support = list;
            image_support =image;
    
        }
     
     }
     
     //write the Get code for the above

};

#endif


class cAppInfo:public  CMThread 
{

public:

    cAppInfo(CardManager *cm,char *name);
    ~cAppInfo();
    void initialize(void);
    void run(void);
    void Send_app_info_request(void);
    cDisplayCapabilities    cDispCapabilities;
    int vlAppInfoGetMMIMessageFromCableCard(unsigned char **pData,unsigned long *pSize); 
private:
    
    CardManager     *cm;
    rmf_osal_eventqueue_handle_t     event_queue;
    rmf_osal_Mutex    aiMutex;

    void CreateAppInfoList(LCSM_EVENT_INFO *pEventInfo);
    uint8_t ParseServerReply(LCSM_EVENT_INFO *pEventInfo);    
    
};


typedef struct vlCardAppInfo
{
    unsigned short CardManufactureId;// 2 byte cable card Manufacture Id
    unsigned short CardVersion; //  2 buy cable card version number
    unsigned char CardMAC[6];
    unsigned char CardSerialNumberLen;// Cable card Serial number length
    unsigned char CardSerialNumber[256]; // Cable card serial number array of 256 max
    unsigned char CardNumberOfApps;// number of cable card applications
    vlCableCardApp_t Apps[VL_MAX_CC_APPS];// Max 64 cable card applications
    int nSubPages;
}vlCardAppInfo_t;
#endif





