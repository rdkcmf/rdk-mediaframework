/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
 * ============================================================================
 *
 * File originally created by The DTVKit Open Software Foundation Ltd (www.dtvkit.org)
 * This file is part of a DTVKit Software Component and is
 * licensed by RDK Management, LLC under the terms of the RDK license.
 * The RDK License agreement constitutes express written consent by DTVKit.
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
 * EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
 */

/**
 * @brief   RDK media framework DTV source
 * @file    rmfdtvsource.h
 * @date    November 2019
 */

#ifndef __RMFDTVSOURCE_H
#define __RMFDTVSOURCE_H

#include "rmfbase.h"
#include "rmf_error.h"
#include "rmf_osal_sync.h"


class RMFDTVSource : public RMFMediaSourceBase
{
   public:
      static RMFDTVSource* getDTVSourceInstance(const char *uri);
      static void freeDTVSourceInstance(RMFDTVSource *dtvsrc);

      ~RMFDTVSource(void);

      RMFResult init();
      RMFResult term();
      RMFResult open(const char* uri, char* mimetype);
      RMFResult close();
      RMFResult play();
      RMFResult play(float& speed, double& time);
      RMFResult pause();
      RMFResult stop();

      uint32_t refcount() const {return(m_refcount);}

   protected:
      void *createElement(void);
      RMFMediaSourcePrivate* createPrivateSourceImpl();

   private:
      RMFDTVSource(void);
      uint32_t m_refcount;
};

#endif // __RMFDTVSOURCE_H

