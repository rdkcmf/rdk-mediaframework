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


#ifndef TABLECVT_T2Vx_H
#define TABLECVT_T2Vx_H

#include <tablecvt.h>

/**
  *@author Eric S. Allen
  */

class TableCVT_T2Vx : public TableCVT  {
public: 
    TableCVT_T2Vx(void *buf, unsigned long in_len);
    ~TableCVT_T2Vx();

  int filterCVT(cvtfilterdata_t *data);

private:
 void parseCVTVer1();
  void parseCVTVer2();
  void *cvt;
  unsigned long len;
};

#endif
