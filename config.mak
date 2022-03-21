#
# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2014 RDK Management
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
#

#Please make sure that 
# 1. PLATFORM_SDK,
# 2. GST_SOURCE 
# 3. RDK_DIR are exported properly before using this file

#Path of directory containing configuration files in STB. If commented, application expects config files to be present in working directory.
#CONFIG_DIR=/config
#Configuration flags.  Comment the definition for disabling and uncomment for enabling.

#Flag for adding memory profiling in osal
#RMF_OSAL_FEATURE_MEM_PROF=1

#Flag for testing with construction and init of qam src during initialisation
#SRC_INIT_IN_CONSTRUCTOR=1

# -------- Four Configuration modes ------------
#    HEADED_GW : Both server and client
#    HEADLESS_GW : Only server(streamer)
#    NODVR_GW : Gateway without DVR
#    CLIENT_TSB : Limited streamer and client 
#    CLIENT : only client 
configMode ?= HEADED_GW

#XG5_GW=1

#USE_HW_DEMUX=1
ifneq ($(configMode),HEADLESS_GW)
  USE_MEDIAPLAYERSINK=1
  USE_FRONTPANEL=1
  USE_GLI=1
endif

# Temporary - Nilay
ifeq ($(configMode),CLIENT_TSB)
  USE_FRONTPANEL=0
  USE_GLI=0
endif

ifeq ($(configMode),HEADLESS_GW)
  HEADLESS_GW=1
endif

#USE_SMOOTHSRC=0

ifneq ($(configMode),CLIENT)
  USE_DVR=1
  USE_HNSINK=1
endif

ifeq ($(configMode),NODVR_GW)
  USE_DVR=0
endif

ifneq ($(configMode),CLIENT_TSB)
ifneq ($(configMode),CLIENT)
  USE_QAMSRC=1
  USE_VODSRC=1
  # Flag for enabling Recorder scheduler support in rmfStreamer  application
  RECORDER_SUPPORT = 1
  # Flag for enabling Tuner Reservation Manager
  USE_TRM = 1
  # Flag for enabling DTV source for full DVB support
#  USE_DTVSRC=1
endif
endif
ifeq ($(MLT_ENABLED),y)
USE_SYSRES_MLT=1
endif
 
USE_HNSRC=1
RMF_STREAMER=1
USE_DTCP=1
USE_PXYSRVC=1
SNMP_IPC_CLIENT=1
#Flag for enabling/disabling STT event notifications to POD Server
#ENABLE_TIME_UPDATE=1

ifdef USE_VODSRC
  #Enable Auto Discovery for VOD
  ENABLE_AUTODISCOVERY_FOR_VOD=1
endif

#Flag for enabling breakpad integration.
#INCLUDE_BREAKPAD=1

#Disable qamsrc/vodsource from mediastreamer. This can be used for streaming only recordings.
#LIMITED_STREAMER=1

ifdef USE_QAMSRC
  ifdef USE_POD
    #Flag for enabling POD
    TEST_WITH_PODMGR=1
  endif

  #Pass PMT as a gstreamer buffer property to gstqamtunersrc
  QAMSRC_PMTBUFFER_PROPERTY=1

  #Generate PMT in qamsrc rather that using that from stream
  #QAMSRC_GENERATE_PMT=1

  #If enabled, QAMSRC factory implementation is enabled.
  QAMSRC_FACTORY=1
ifdef TEST_WITH_PODMGR
  #if iPPV is enabled
  IPPV_CLIENT_ENABLED=1		
  USE_POD_IPC=1
endif
  #Flag for enabling optimization of Inband SI Cache
  OPTIMIZE_INBSI_CACHE=1

  #QAMSRC_USE_OOBSI_STUB=1
  #Flag for building PC utility
  #GENERATE_SI_CACHE_UTILITY=1
  #Flag for enabling inband SI caching.
  #ENABLE_INB_SI_CACHE=1

  ifndef GENERATE_SI_CACHE_UTILITY
    # Flag for enabling oob section filter to start fetching OOB SI sections and send it to OOB SI manager 
    OOBSI_SUPPORT = 1
  endif
endif



#Flag for enabling Memory Leak Tracker
#USE_SYSRES_MLT = 1

#ifndef PLATFORM_SDK 
#  TOOLCHAIN_DIR=/opt/toolchain/staging_dir
#else
#  TOOLCHAIN_DIR=$(PLATFORM_SDK)
#endif

#ifndef GENERATE_SI_CACHE_UTILITY
#  CC=$(TOOLCHAIN_DIR)/bin/i686-cm-linux-g++
#  CXX=$(TOOLCHAIN_DIR)/bin/i686-cm-linux-g++
#  AR=$(TOOLCHAIN_DIR)/bin/i686-cm-linux-ar
#  LD=$(TOOLCHAIN_DIR)/bin/i686-cm-linux-ld
#else
#  CC=g++
#  AR=ar
#endif
USE_TRANSCODEFILTER=1

# Sunil - Added to enable the separate handling of IPV6 and IPV4 in Pod
#USE_BRCM_SOC_IPV6=1

ifeq ($(USE_FRONTPANEL),1)
USE_FPTEXT=1
USE_FPLED=1
endif

-include $(RDK_DIR)/mediaframework/platform/soc/scripts/config.mak


ifeq ($(configMode),HEADLESS_GW)
CFLAGS += -DHEADLESS_GW
endif

ifeq ($(configMode),CLIENT_TSB)
 CFLAGS += -DCLIENT_TSB
endif

ifdef USE_HW_DEMUX
 CFLAGS += -DUSE_HW_DEMUX
endif

ifdef USE_QAMSRC
 CFLAGS += -DUSE_QAMSRC
endif

ifdef USE_DTVSRC
 CFLAGS += -DUSE_DTVSRC
endif

ifdef SNMP_IPC_CLIENT
 CFLAGS += -DSNMP_IPC_CLIENT
endif

ifeq ($(USE_TRANSCODEFILTER),1)
 CFLAGS += -DUSE_TRANSCODEFILTER
endif

ifeq ($(USE_FRONTPANEL),1)
 CFLAGS += -DUSE_FRONTPANEL
endif

ifeq ($(USE_FPTEXT),1)
 CFLAGS += -DUSE_FPTEXT
endif

ifeq ($(USE_FPLED),1)
 CFLAGS += -DUSE_FPLED
endif

ifeq ($(USE_GLI),1)
 CFLAGS += -DGLI
 CFLAGS += -DUSE_IARM_BUS
endif

ifdef USE_DTCP
 CFLAGS+=-DUSE_DTCP
endif

ifdef USE_HNSINK
 CFLAGS+=-DUSE_HNSINK
endif

ifdef USE_HNSRC
 CFLAGS+=-DUSE_HNSRC
endif

ifeq ($(USE_DVR),1)
 CFLAGS+=-DUSE_DVR
endif

ifdef USE_VODSRC
 CFLAGS += -DUSE_VODSRC
endif

ifdef USE_MEDIAPLAYERSINK
 CFLAGS += -DUSE_MEDIAPLAYERSINK
endif

ifdef USE_PXYSRVC
 CFLAGS += -DUSE_PXYSRVC
endif

ifdef ENABLE_TIME_UPDATE
 CFLAGS += -DENABLE_TIME_UPDATE
endif

ifdef SRC_INIT_IN_CONSTRUCTOR
 CFLAGS+=-DSRC_INIT_IN_CONSTRUCTOR
endif

ifdef RMF_OSAL_FEATURE_MEM_PROF
 CFLAGS+=-DRMF_OSAL_FEATURE_MEM_PROF
endif

ifdef RMF_STREAMER
 CFLAGS += -DRMF_STREAMER
endif


ifdef ENABLE_AUTODISCOVERY_FOR_VOD 
 CFLAGS+=-DENABLE_AUTODISCOVERY_FOR_VOD  
endif

ifdef QAMSRC_FACTORY
 CFLAGS += -DQAMSRC_FACTORY
endif

ifdef QAMSRC_GENERATE_PMT 
 CFLAGS += -DQAMSRC_GENERATE_PMT
endif

ifdef QAMSRC_PMTBUFFER_PROPERTY
 CFLAGS += -DQAMSRC_PMTBUFFER_PROPERTY
endif

ifdef GENERATE_SI_CACHE_UTILITY
 CFLAGS += -DGENERATE_SI_CACHE_UTILITY
endif

ifdef CONFIG_DIR
 CONFIG_PREFIX=$(CONFIG_DIR)/
endif

ifdef RECORDER_SUPPORT
 CFLAGS += -DRECORDER_SUPPORT
endif

ifdef USE_TRM
 CFLAGS += -DUSE_TRM
endif

ifdef TEST_WITH_PODMGR
 CFLAGS += -DTEST_WITH_PODMGR
ifdef IPPV_CLIENT_ENABLED
 CFLAGS += -DIPPV_CLIENT_ENABLED
endif
ifdef USE_POD_IPC
 CFLAGS += -DUSE_POD_IPC
endif 
endif

ifdef ENABLE_INB_SI_CACHE
 CFLAGS += -DENABLE_INB_SI_CACHE
endif

ifdef OOBSI_SUPPORT
 CFLAGS += -DOOBSI_SUPPORT
endif

ifdef USE_SYSRES_MLT
  #CFLAGS += 
  export USE_SYS_RESOURCE_MON=y
  export USE_SYS_RESOURCE_MLT=y
  USE_SYS_RESOURCE_MON=y
 USE_SYS_RESOURCE_MLT=y
endif

ifdef INCLUDE_BREAKPAD
  CFLAGS += -DINCLUDE_BREAKPAD
endif

CFLAGS += -DDEBUG_CONF_FILE="\"$(CONFIG_PREFIX)debug.ini\"" \
		  -DENV_CONF_FILE="\"$(CONFIG_PREFIX)rmfconfig.ini\""

ARFLAGS=rcs
BUILD_DIR=$(RMF_DIR)/build
LIBDIR=$(BUILD_DIR)/lib

OSAL_INCL=$(RMF_DIR)/osal/include
QAMSRC_INCL=-I $(RMF_DIR)/qamsource/main -I $(RMF_DIR)/qamsource/common
OOBSI_INCL=$(RMF_DIR)/test
RMF_HNSINK_INCLUDE_DIR=$(RDK_DIR)/mediaframework/hnsink

INCLUDES?=-I$(OSAL_INCL)\
	  $(QAMSRC_INCL) \
	-I$(OOBSI_INCL) \
	-I$(RMF_DIR)/../core\
	-I$(RMF_DIR)/osal/utils/inc\
	-I$(PLATFORM_SDK)/include/ \
	-I$(PLATFORM_SDK)/include/gstreamer-0.10  \
	-I$(PLATFORM_SDK)/include/glib-2.0  \
	-I$(PLATFORM_SDK)/lib/glib-2.0/include  \
	-I$(PLATFORM_SDK)/usr/include/ \
	-I$(PLATFORM_SDK)/include/libxml2 \
	-I$(RDK_DIR)/opensource/include/libxml2 \
	-I$(RDK_DIR)/opensource/include/gstreamer-0.10 \
	-I$(RDK_DIR)/opensource/include/glib-2.0 \
	-I$(RDK_DIR)/opensource/include/ \
	-I$(RDK_DIR)/opensource/lib/glib-2.0/include  \
	-I$(RDK_DIR)/rdklogger/include  \
	-I$(PLATFORM_SDK)/include/linux_user

ifdef USE_SYSRES_MLT
  INCLUDES += -I $(SYSRES_DIR)/sys_resource/include -DUSE_SYSRES_MLT=1 -DUSE_FILE_LINE_INFO=1 -DRETURN_ADDRESS_LEVEL=0 -DANALYZE_IDLE_MLT=1
endif

INBSILIBS= -linbsimanager -linbsectionfilter -lsectionfilter
ifeq ($(TEST_WITH_PODMGR),1)
  OOBSILIBS= -lsectionfilter
else
  OOBSILIBS= -loobsimgrstub
endif

ifdef TEST_WITH_PODMGR
  SOC_SHLIBS+= $(MSPOD_LIBS)
endif


RAMDISK_DIR=$(RDK_DIR)/sdk/fsroot/ramdisk

SNMPSHLIBS=-lnetsnmp -lnetsnmpagent -lnetsnmphelpers

OPENSRC_LIBS= -lglib-2.0 -lpthread

WHOLE_LIB_PATH ?= -L$(RDK_DIR)/opensource/lib/ -L$(OPENSOURCE_SRC_DIR)/openssl -L$(LIBDIR) -L$(RDK_DIR)/mfrlibs -L$(RDK_DIR)/sdk/fsroot/ramdisk/lib -L$(TOOLCHAIN_DIR)/lib -L$(RAMDISK_DIR)/lib -L$(BUILD_DIR)/objs_podmgr/ -L$(SNMP_DIR)/lib -L$(RDK_DIR)/mediaframework/build/gstreamer-plugins
WHOLE_LIBS=$(OPENSRC_LIBS) $(SOC_SHLIBS)

ifdef TEST_WITH_PODMGR
  WHOLE_LIBS+=$(OPENSSL_LIBS) $(PLATFORM_SHLIBS) $(SNMPSHLIBS)  $(TUNER_SHLIBS)
endif

SYSRES_DIR=$(RDK_DIR)/sys_mon_tools
