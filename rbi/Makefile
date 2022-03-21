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
ifeq ($(PLATFORM_SDK),)
$(error PLATFORM_SDK not set.  Please define PLATFORM_SDK to point to the SDK"!)
endif

RMF_BASE=$(RDK_DIR)/mediaframework
include $(RMF_BASE)/config.mak

TOOLBASE=$(PLATFORM_SDK)/bin

TARGETCC ?= $(TOOLBASE)/i686-cm-linux-g++
TARGETLN ?= $(TOOLBASE)/i686-cm-linux-g++

SHAREDLIB = rbi

INCLUDES += \
           -I$(RMF_BASE)/osal/include \
           -I$(RDK_DIR)/rdklogger/include \
           -I$(RDK_DIR)/opensource/src/curl/include \
           -I$(RDK_DIR)/iarmbus/core/include

ifdef USE_SYSRES_MLT
INCLUDES += -I $(SYSRES_DIR)/sys_resource/include
endif

LIBS= -L$(RDK_DIR)/iarmbus/install -l IARMBus \
      -L$(RDK_DIR)/opensource/lib -lfusiondale

CFLAGS += -O2 -g -Wall -fno-strict-aliasing $(LIBS) --sysroot=$(PLATFORM_SDK)

all : $(SHAREDLIB)

$(SHAREDLIB) : rbi.o
	$(TARGETCC) $(CFLAGS) -shared -o lib$@.so $^

.c.o:
	$(TARGETCC) -c $(CFLAGS) -x c++ $(INCLUDES) $< -o $(@D)/$(@F)

.cpp.o:
	$(TARGETCC) -c $(CFLAGS) -x c++ $(INCLUDES) $< -o $(@D)/$(@F)

debug : all

clean:
	$(RM) *.o lib$(SHAREDLIB).so

