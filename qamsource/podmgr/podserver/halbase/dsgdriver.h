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
 * \file pfcdsgdriver.h Docsis Settop Gateway interface.
 */

#ifndef PFCDSGDRIVER_H
#define PFCDSGDRIVER_H



#include "dsg_types.h"
 
class dsgDriver
{
  public:
  //! Sets the plugin text argument for this object.
    dsgDriver (char *plugin_title_argument);
  //! Closes the Dsg driver device.
  virtual ~ dsgDriver ();

  //! Return the class name, "dsgDriver".
    char *class_name ();
  //! Return the \a plugin_argument_title.
    virtual char *plugin_title ();

  //! Initialize the Dsg device driver. 
    virtual void initialize ();
    
  //! get cablecard device count
    virtual int get_device_count();
    
  //! Open the Dsg driver device.
    virtual int open_device (int device_instance);
  //! Close the Dsg driver device.
    virtual int close_device (int device_instance);
  //! Register for any of the callback types 
    virtual int register_callback(VL_DSG_NOTIFY_TYPE_t type , void *, int device_instance);
  //! send control data to ECM    
    virtual int Send_ControlMessage(unsigned char * pData, unsigned long length);
    virtual int Set_Config(int device_instance, unsigned long ulTag, void* pvConfig);
private:
    char *plugin_title_argument;
    
};

#endif // PFCDSGDRIVER_H
