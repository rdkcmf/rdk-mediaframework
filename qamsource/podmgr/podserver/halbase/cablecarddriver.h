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
 * \file pfcpaneldriver.h Graphics display panel interface.
 */

#ifndef PFCCABLECARDDRIVER_H
#define PFCCABLECARDDRIVER_H

#ifndef PFCDECLARATION_H
//#include "pfcdeclaration.h"
#endif
#ifndef PFCPLUGINBASE_H
//#include "pfcpluginbase.h"
#endif

 

typedef enum
{
NOTIFY_CARD_DETECTION_CB,
NOTIFY_POD_POLL_CMD_CB,
NOTIFY_DATA_AVAILABLE_CB,
NOTIFY_EXTCH_DATA_AVAILABLE_CB

}eCallbackType;


typedef enum
{
DATA_CHANNEL,
EXTENDED_DATA_CHANNEL

}CHANNEL_TYPE_T;

class cableCardDriver
{
  public:
  //! Sets the plugin text argument for this object.
	cableCardDriver ();
  //! Closes the CableCard driver device.
  virtual ~ cableCardDriver ();

  //! Return the class name, "cableCardDriver".
//	char *class_name ();
  //! Return the \a plugin_argument_title.
	virtual char *plugin_title ();

  //! Initialize the CableCard device driver. 
	virtual void initialize ();
	
  //! get cablecard device count
	virtual int get_device_count();
	
  //! Open the CableCard driver device.
	virtual int open_device (int device_instance);
  //! Close the CableCard driver device.
	virtual int close_device (int device_instance);

  //! Register for any of the callback types 
  	virtual int register_callback(eCallbackType , void *, int device_instance);
  //! send data of type DATA or EXTENDED_CHANNEL_DATA to POD	
	virtual int send_data(void *ptr, CHANNEL_TYPE_T  type, int device_instance);
  //! connect specified tuner Source to POD 
	virtual void	ConnectSourceToPOD(unsigned long tuner_in_handle,int device_instance,unsigned long videoPid);
  //! connect specified tuner Source to POD 
    virtual int    oob_control(int device_instance, unsigned long eCommand, void * pData);
    virtual int    if_control(int device_instance, unsigned long eCommand, void * pData);
    virtual int configPodCipher(unsigned char ltsid,unsigned short PrgNum,unsigned long *decodePid,unsigned long numpids,unsigned long DesKeyAHi,unsigned long DesKeyALo,unsigned long DesKeyBHi,unsigned long DesKeyBLo,void *pStrPtr);
    virtual int stopconfigPodCipher(unsigned char ltsid,unsigned short PrgNum,unsigned long *decodePid,unsigned long numpids);
  private:
	char *plugin_title_argument;
	
};


void set_cdl_device();

#endif
