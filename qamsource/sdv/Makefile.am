#
# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2018 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
##########################################################################

LOCAL_DEFINES =  -DRMF_OSAL_LITTLE_ENDIAN \
                 -DUSE_CC_GETTIMEOFDAY \
                 -DRMF_OSAL_FEATURE_IPV6

AM_CXXFLAGS = -pthread -Wall -Wno-format -Wunused $(GST_CFLAGS) $(LOCAL_DEFINES) \
              -I${top_srcdir}/osal/include \
              -I${top_srcdir}/qamsource/podmgr/canh \
              -I${top_srcdir}/qamsource/podmgr/inc \
              -I${top_srcdir}/qamsource/simgr/inband/include \
              -I${top_srcdir}/qamsource/common \
              -I${top_srcdir}/qamsource/main \
              -I${top_srcdir}/qamsource/sdv/include \
              -I${top_srcdir}/core $(JANSON_CFLAGS) \
              -I${top_srcdir}/qamsource/podmgr/podserver/inc \
              -I${top_srcdir}/qamsource/oobsimgrstub \
              -I$(RDK_PROJECT_ROOT_PATH)/sdvagent/src


lib_LTLIBRARIES = libsdvsectionfilter.la

libsdvsectionfilter_la_SOURCES = SDVForceTuneEventHandler.cpp SDVSectionFilter.cpp SDVSectionFilterRegistrar.cpp
