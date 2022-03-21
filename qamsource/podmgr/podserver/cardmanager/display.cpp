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



#include "cm_api.h"
 

    cDisplayCapabilities::cDisplayCapabilities()
    {
    //put default values here
        DisplayRows= 0xFF; /*more than 255   */
        DisplayColumns=0xFF;/*more than 255   */
        verticalScrolling=0x00; /*None */
        horizontal_scrolling=0x00;/*None */
        display_type_support=0x00;/*Full screen*/
        data_entry_support=0x00; /* None */
        //VL: RAJASREE: 09262007: changed since baseline profile HTML support is mandatory. 
#if 1
        HTML_Support= 0x00 /* Baseline profile */;         

        link_support=0x00; /* One link */
        form_support=0x00; /*None */
        table_support=0x00; /*None */
        list_support=0x00; /*None */
        image_support=0x00; /*None */
#endif
    }
    
    cDisplayCapabilities::~cDisplayCapabilities()
    {
    
    }
    
    void    cDisplayCapabilities::SetRows(int num_rows)
    {
        DisplayRows = num_rows;
    }
    void     cDisplayCapabilities::SetColumns(int num_columns)
    {
        DisplayColumns = num_columns;
            
    }
    void    cDisplayCapabilities::SetScrolling(int vert, int hor)
    {
        horizontal_scrolling= hor;
        verticalScrolling = vert;
    
    }
    
    void    cDisplayCapabilities::SetDisplayTypeSupport( int multi_window_cap)
    {
        display_type_support = multi_window_cap;
    
    }
     void   cDisplayCapabilities::SetDataEntrySupport(int data_entry_cap)
     {
         data_entry_support = data_entry_cap;
     
     }
     
     void    cDisplayCapabilities::SetHTMLSupport(int htmlSupport = 0,int link = 0,int form=0,int table=0,int list=0, int  image=0)
     {
        //VL: RAJASREE: 09262007: changed since baseline profile HTML support is mandatory. [start]
         // if(HTML_Support == htmlSupport){
        HTML_Support = htmlSupport;
        link_support = link;
        form_support = form;
        table_support = table;
        list_support = list;
        image_support =image;
    
        //}
        //VL: RAJASREE: 09262007: changed since baseline profile HTML support is mandatory. [end]
     
     }
     
     //write the Get code for the above

    int   cDisplayCapabilities::GetRows()
    {
        return DisplayRows;
    }
    int     cDisplayCapabilities::GetColumns()
    {
        return DisplayColumns;
            
    }
    void    cDisplayCapabilities::GetScrolling(unsigned char &vert, unsigned char &hor)
    {
        hor = horizontal_scrolling;
        vert = verticalScrolling;
    
    }
    
    int    cDisplayCapabilities::GetDisplayTypeSupport()
    {
        return display_type_support;
    
    }
     int   cDisplayCapabilities::GetDataEntrySupport()
     {
         return data_entry_support;
     
     }
     
     void    cDisplayCapabilities::GetHTMLSupport(unsigned char &htmlSupport,unsigned char &link,unsigned char &form,unsigned char &table,unsigned char &list, unsigned int &image)
     {
        //VL: RAJASREE: 09262007: added since baseline profile HTML support is mandatory. [start]
        //HTML_Support = static_cast<unsigned char>(htmlSupport);
        htmlSupport = static_cast<unsigned char>(HTML_Support);         
        //VL: RAJASREE: 09262007: changed since baseline profile HTML support is mandatory. [end]
        link = static_cast<unsigned char>(link_support);
        form = static_cast<unsigned char>(form_support);
        table = static_cast<unsigned char>(table_support);
        list = static_cast<unsigned char>(list_support);
        image = static_cast<unsigned int>(image_support);
     }
      void    cDisplayCapabilities::GetHTMLSupport(unsigned char &htmlSupport)
       {
     
           htmlSupport =  static_cast<unsigned char>(HTML_Support);
       }
