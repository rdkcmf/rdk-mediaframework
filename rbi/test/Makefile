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

BUILD_CONFIGURATION=RELEASE
DAEMON=rbiDaemon

RDK_DIR=../../..
RMF_BASE=../..

BASE_DIR=$(RMF_BASE)/qamsource
include $(RMF_BASE)/config.mak

CFLAGS+=-fno-builtin-memcpy -fno-strict-aliasing -DFORMAT_ELF -D_INTERNAL_PCI_ -O2 -fPIC -MD -m32 -DGCC4_XXX --sysroot=$(PLATFORM_SDK)

INCLUDE+=-I${PLATFORM_SDK}/lib/glib-2.0/include \
        -I${PLATFORM_SDK}/include/glib-2.0 \
        -I${PLATFORM_SDK}/include/gstreamer-0.10 \
        -I$(PLATFORM_SDK)/include \
        -I$(PLATFORM_SDK)/usr/local/include \
        -I$(PLATFORM_SDK)/usr/include \
        -I$(PLATFORM_SDK)/include/linux_user \
        -I${PLATFORM_SDK}/include/libxml2

ifdef TEST_WITH_PODMGR
LIB+= -lpodmgr -loobsimanager $(WHOLE_LIB_PATH) $(WHOLE_LIBS) 
 ifdef USE_TRM
   LIB += -ltrh -ljansson
 endif	
endif

LIB+= \
    $(RDK_DIR)/opensource/src/openssl/libcrypto.a \
    -L${PLATFORM_SDK}/lib \
    -L${PLATFORM_SDK}/lib/glib-2.0 \
    -L$(PLATFORM_SDK)/usr/lib \
    -L$(PLATFORM_SDK)/usr/local/lib \
    -L$(PLATFORM_SDK)/lib \
    -lglib-2.0 \
    -lgstreamer-0.10 \
    -L$(RDK_DIR)/rdklogger/build/lib -lrdkloggers \
    $(RDK_DIR)/opensource/lib/libuuid.a \
    -lcurl

ifeq ($(USE_SYS_RESOURCE_MLT), y)
LIB += -L $(SYSRES_DIR)/sys_resource/lib/platform/canmore -lsysResource
endif

ifeq ($(USE_SYS_RESOURCE_MON), y)
LIB += -L $(SYSRES_DIR)/sys_resource/lib/platform/canmore -lsysResource
endif

CC=$(PLATFORM_SDK)/bin/i686-cm-linux-gcc
CXX=$(PLATFORM_SDK)/bin/i686-cm-linux-g++
AR=$(PLATFORM_SDK)/bin/i686-cm-linux-ar
LD=$(PLATFORM_SDK)/bin/i686-cm-linux-ld

INCLUDE +=  -I$(RMF_BASE)/core
SOURCE  +=  $(RMF_BASE)/core/rmfbase.cpp

CFLAGS += -DBUILD_MEDIAPLAYERSINK
INCLUDE += -I$(RMF_BASE)/mediaplayersink
SOURCE += $(RMF_BASE)/mediaplayersink/mediaplayersink.cpp

CFLAGS += -DBUILD_QAMSOURCE
INCLUDE += -I$(RMF_BASE)/qamsource/main
INCLUDE += -I$(RMF_BASE)/osal/include
INCLUDE += -I$(RMF_BASE)/osal/utils/inc
INCLUDE += $(INCLUDES)
INCLUDE += -I$(RMF_BASE)/qamsource/tuner/include
INCLUDE += -I$(RMF_BASE)/qamsource/test
INCLUDE += -I$(RMF_BASE)/qamsource/common
INCLUDE += -I$(RMF_BASE)/qamsource/gstqamtunersrc
INCLUDE += -I$(RMF_BASE)/qamsource/simgr/inband/include
INCLUDE += -I$(RMF_BASE)/qamsource/simgr/oob/include
INCLUDE += -I$(RMF_BASE)/qamsource/sectionfilter/include
INCLUDE += -I$(RMF_BASE)/qamsource/podmgr/inc/
LIB += -L$(RMF_BASE)/build/lib -lrmfosal -lrmfosalutils $(OOBSILIBS) $(INBSILIBS)
LIB += -L$(RMF_BASE)/osal/lib -llog4c -L../build/lib
LIB += -L$(TOOLCHAIN_DIR)/lib -lismd_core -lismd_demux
LIB += -L$(RMF_BASE)/thirdparty/pace -lPaceTuner -lPaceFrontEnd
LIB += -L $(RDK_DIR)/../combined/sdk/fsroot/ramdisk/lib
LIB += $(SOC_SHLIBS) $(PLATFORM_SHLIBS)
LIB += -lPacePosixApi
LIB += -L $(RDK_DIR)/opensource/target-snmp/lib
LIB += $(SNMPSHLIBS)
LIB += -lgstapp-0.10 -lhtx_dsg -ldsg_rpc_h2cm
LIB += -lqamsrc
SOURCE += $(RMF_BASE)/qamsource/main/rmfqamsrc.cpp

CFLAGS += -DBUILD_DVRSOURCE
SOURCE += $(RMF_BASE)/dvr/source/DVRSource.cpp

CFLAGS += -DBUILD_DVRSINK
SOURCE += $(RMF_BASE)/dvr/sink/DVRSink.cpp

SOURCE += $(RMF_BASE)/rbi/RBIFilter.cpp

INCLUDE += -I$(RMF_BASE)/dvr/source
INCLUDE += -I$(RMF_BASE)/dvr/sink
INCLUDE += -I$(RMF_BASE)/rbi
INCLUDE += -I$(RMF_BASE)/thirdparty/tinyxml
INCLUDE += -I$(RMF_BASE)/dvr/common/dvrmgr
INCLUDE += -I$(RMF_BASE)/dvr/common/tasseograph
INCLUDE += -I$(SOC_DVRINC)
INCLUDE += -I$(RDK_DIR)/iarmbus/core/include
LIB += -L$(RMF_BASE)/rbi -lrbi
LIB += -L$(RMF_BASE)/dvr/common/dvrmgr -ldvrmgr
LIB += -L../thirdparty/e2fsprogs/e2fsprogs/lib $(RDK_DIR)/opensource/lib/libuuid.a
LIB += -L$(RMF_BASE)/dvr/common/tasseograph -lTasseograph
LIB += -L$(RMF_BASE)/thirdparty/tinyxml -ltinyxml
LIB += -L$(RDK_DIR)/iarmbus/install -l IARMBus
LIB += -L$(RDK_DIR)/opensource/lib -lfusiondale

CXXFLAGS=$(CFLAGS)

OBJCPP=$(SOURCE:.cpp=.o)
OBJ=$(OBJCPP:.c=.o)
DEPCPP=$(SOURCE:.cpp=.d)
DEP=$(DEPCPP:.c=.d)

.SUFFIXES: .o .c .cpp .cc .cxx .C

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCLUDE) -o "$@" "$<"

.cc.o:
	$(CXX) -c $(CXXFLAGS) $(INCLUDE) -o "$@" "$<"

.cxx.o:
	$(CXX) -c $(CXXFLAGS) $(INCLUDE) -o "$@" "$<"

.C.o:
	$(CXX) -c $(CXXFLAGS) $(INCLUDE) -o "$@" "$<"

.c.o:
	$(CC) -c $(CFLAGS) $(INCLUDE) -o "$@" "$<"

all: $(DAEMON)

$(DAEMON): rbiDaemon.o
	$(CXX) -o $(DAEMON) $^ -L$(RDK_DIR)/iarmbus/install -l IARMBus -L$(RDK_DIR)/opensource/lib -lfusiondale -L${PLATFORM_SDK}/lib/glib-2.0 -lglib-2.0

.PHONY : clean
clean:
	-$(RM) $(OBJ) $(DEP) $(DAEMON)


