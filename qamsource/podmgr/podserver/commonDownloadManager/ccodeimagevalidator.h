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


#ifndef CCODEIMAGEVALIDATOR_H
#define CCODEIMAGEVALIDATOR_H


/**
  *@author Eric S. Allen
  */
#include "CommonDownloadMgr.h"
class CCodeImageValidator {
public: 
    CCodeImageValidator();
    ~CCodeImageValidator();

  int processCodeImage(void *code, int len,CommonDLObjType_e  objType, bool bCheckTime = true);
  int getRootCACert(void *code, int len, void **cert, int *certLen);
  int getBaseCACert(void *code, int len, void **cert, int *certLen);
  int getMfrCACert(void *code, int len, void **cert, int *certLen);
};

#endif
