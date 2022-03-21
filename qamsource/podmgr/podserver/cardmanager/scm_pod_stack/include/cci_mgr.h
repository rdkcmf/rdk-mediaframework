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

#ifndef _CCI_MGR_H_
#define _CCI_MGR_H_


#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 *
 * Paste into your file & run to load the CCI dynamic library
 *
 *********************************************************************/
#if 0
void *trhandle;
static CCI_INTERFACE *cciInterface;

static int loadCciInterface(void)
{
    int   returnValue;
    char  *error;
    
    returnValue = 0;
    trhandle = dlopen ("./libcci.so", RTLD_LAZY);
    if ( trhandle == NULL ) 
    {
        RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "Cannot load cci Library \n");
        returnValue = -1;
    }
    else
    {
        cciInterface = (CCI_INTERFACE*)dlsym(trhandle, "cciInt");
        if ((error = dlerror()) != NULL)  
        {
            RDK_LOG(RDK_LOG_ERROR, "LOG.RDK.POD", "cannot open cci interface \n");
            returnValue = -1;
        }
    }
    
    return returnValue;
}
#endif


typedef int (*_cci_setEMI)( LCSM_DEVICE_HANDLE lcsmHandle, unsigned char emi_value );
typedef int (*_cci_setAPS)( LCSM_DEVICE_HANDLE lcsmHandle, unsigned char aps_value );

typedef struct 
{
    _cci_setEMI cci_setEMI;
    _cci_setAPS cci_setAPS;

} CCI_INTERFACE;


#ifndef  _CCI_INTERFACE_MODULE_
#include <dlfcn.h>


#define  cci_setEMI( lcsmHandle, emi_value ) \
    cciInterface->cci_setEMI( lcsmHandle, emi_value )

#define cci_setAPS( lcsmHandle, aps_value ) \
    cciInterface->cci_setAPS( lcsmHandle, aps_value )
    


#else /* _CCI_INTERFACE_MODULE_ */
    
// use these for cci_mgr.c
int cci_setEMI( LCSM_DEVICE_HANDLE lcsmHandle, unsigned char emi_value );
int cci_setAPS( LCSM_DEVICE_HANDLE lcsmHandle, unsigned char aps_value );


#endif /* _CCI_INTERFACE_MODULE_ */



#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* _CCI_MGR_H_ */
