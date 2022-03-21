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
RMF_DIR=$(RDK_DIR)/mediaframework
include $(RMF_DIR)/config.mak

BASE_DIR=$(RMF_DIR)/vodsource
IARM_PATH=$(RDK_DIR)/iarmbus

CFLAGS?=  -Wno-format -Wunused -DUSE_CC_GETTIMEOFDAY -DRMF_OSAL_FEATURE_IPV6 -DRMF_OSAL_LITTLE_ENDIAN

ARFLAGS?=rcs
HTTPSINK_INCL=$(RMF_DIR)/httpsink
INBSI_INCL=$(RMF_DIR)/qamsource/simgr/inband/include
SF_INCL=$(RMF_DIR)/qamsource/sectionfilter/include
OOBSI_INCL=$(RMF_DIR)/qamsource/simgr/oob/include
GSTQAMTUNERSRC_INCL=$(RMF_DIR)/qamsource/gstqamtunersrc
PODMGR_INCL=$(RMF_DIR)/qamsource/podmgr/canh
PODSERVER_INCL=$(RMF_DIR)/qamsource/podmgr/podserver/inc
IARM_PATH=$(RDK_DIR)/iarmbus
#PODMGR_DIR=$(BASE_DIR)/podmgr

SRC_DIR=.
LIBNAME=vodsrc

LIBFILE=$(LIBDIR)/lib$(LIBNAME).a

OBJ_DIR=$(BUILD_DIR)/objs_$(LIBNAME)

INCLUDES+= -I . -I $(HTTPSINK_INCL) \
		-I $(BASE_DIR) -I $(RMF_DIR)/core \
		-I $(BASE_DIR)/test -I $(INBSI_INCL) -I $(SF_INCL) \
		-I${PLATFORM_SDK}/lib/glib-2.0/include \
		-I${PLATFORM_SDK}/include/glib-2.0 \
		-I${PLATFORM_SDK}/include/gstreamer-0.10 \
	    -I${PLATFORM_SDK}/include/libxml2 \
	          -I$(OPENSOURCE_SRC_DIR)/jansson/src \
		-I$(RMF_DIR)/qamsource/podmgr/inc/ \
		-I $(RMF_DIR)/qamsource/oobsimgrstub \
		-I$(OOBSI_INCL) \
		-I$(GSTQAMTUNERSRC_INCL) \
		-I$(PODMGR_INCL) \
		-I$(PODSERVER_INCL)

#ifdef TEST_WITH_PODMGR
INCLUDES+=-I$(PODMGR_DIR)/inc 
INCLUDES+=-I$(IARM_PATH)/core/include
INCLUDES+=-I$(RMF_DIR)/rmfproxyservice
#endif

CFLAGS+= $(INCLUDES)
OBJS=$(OBJ_DIR)/rmfvodsource.o \
$(OBJ_DIR)/T2PClient.o \
$(OBJ_DIR)/T2PServer.o \
$(OBJ_DIR)/Autodiscovery.o 


.PHONY : all clean
all: $(LIBFILE)

$(LIBFILE): $(LIBDIR) $(OBJ_DIR) $(OBJS)
	$(AR) $(ARFLAGS) $(LIBFILE) $(OBJS)

$(OBJ_DIR)/%.o :$(SRC_DIR)/%.cpp
	$(CC) -c -o $@ $< $(CFLAGS)
		
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)
	
$(LIBDIR):
	mkdir -p $(LIBDIR)
	
clean :
	rm -rf $(OBJ_DIR) $(LIBFILE)

	
