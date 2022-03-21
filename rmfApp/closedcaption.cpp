/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2014 RDK Management
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

#include <stdio.h>
#include <glib.h>
#include "ccDataReader.h"
//for graphics
#include "cc_util.h"
#include "vlGfxScreen.h"
#include "closedcaption.h"

ClosedCaptions& ClosedCaptions::instance()
{
   static ClosedCaptions ccObject;
   return ccObject;
}

ClosedCaptions::ClosedCaptions()
{
}

ClosedCaptions::~ClosedCaptions()
{
}

void ClosedCaptions::setVisible(bool is_visible)
{
    if(m_isCCRendering)
    {
      if(is_visible)
          ccShow();
      else
          ccHide();
   }
   else
      g_print("Closedcaption not enabled\n");
}

bool ClosedCaptions::start(void* pViddecHandle)
{
    g_print("start: viddecHandle=%p", pViddecHandle);
    if (NULL != pViddecHandle)
    {
        if (!m_isCCRendering)
        {
            ccInit();
            int status = media_closeCaptionStart (pViddecHandle);
            if (status < 0)
            {
                g_print("start media_closeCaptionStart failed %d", status);
                return false;
            }
            m_viddecHandle = pViddecHandle;
            ccStart();
	    g_print("ClosedCaptions::start - Started CC data fetching");
	    m_wasLoadedOnce = true;
        }
        else
        {
            g_print("ClosedCaptions::start - Already started");
            return false;
        }
    }
    else
    {
        g_print("Invalid video decoder handle passed");
        return false;
    }
    return true;
}

bool ClosedCaptions::stop()
{
    int status;
    g_print("stop");

    if (ccStop())
    {
        if (m_viddecHandle)
        {
            status = media_closeCaptionStop();
            if (status < 0)
            {
                g_print("stop media_closeCaptionStop failed %d", status);
                return false;
            }
            else
            {
                g_print("stop succeeded");
            }

            m_viddecHandle = NULL;
        }
   // shutdown();	
    }
    return true;
}

void ClosedCaptions::shutdown()
{
    struct MyvlGfxScreen: public vlGfxScreen
    {
        static void shutdown()
        {
            IDirectFB* dfb = get_dfb();
            g_print("Releasing DirectFB %p", dfb);
            if(dfb)
                dfb->Release(dfb);
        }
    ~MyvlGfxScreen();
    };

    if(m_wasLoadedOnce)
    {
        m_wasLoadedOnce = false;
        MyvlGfxScreen::shutdown();
    }
}
bool ClosedCaptions::ccStart()
{
    if (!m_isCCRendering)
    {
        ccSetCCState(CCStatus_ON, 1);
        m_isCCRendering = true;
        g_print("ccStart started");
        return true;
    }
    return false;
}

bool ClosedCaptions::ccStop()
{
    if (m_isCCRendering)
    {
        ccSetCCState(CCStatus_OFF, 1);
        m_isCCRendering = false;
        g_print("ccStop stopped");
    }
    return true;
}

void ClosedCaptions::ccInit (void)
{
    static bool bInitStatus = false;

    if (!bInitStatus)
    {
        vlGfxInit(0);
        int status = vlMpeosCCManagerInit();
        if (0 != status)
        {
            g_print("ccInit vlMpeosCCManagerInit failed %d", status);
            return;
        }

        bInitStatus = true;
        g_print("ccInit succeeded");
    }
    else
    {
        g_print("ClosedCaptions::ccInit - Already inited.. nothing to be done");
    }

    return;
}
