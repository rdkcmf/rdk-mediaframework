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
 
RMF_DIR=$(RDK_DIR)/mediaframework
include $(RMF_DIR)/config.mak

BASE_DIR=$(RMF_DIR)/qamsource

CFLAGS?=  -Wno-format -Wunused -DUSE_CC_GETTIMEOFDAY -DRMF_OSAL_FEATURE_IPV6 -DRMF_OSAL_LITTLE_ENDIAN

ARFLAGS?=rcs
HTTPSINK_INCL=$(RMF_DIR)/httpsink
INBSI_INCL=$(BASE_DIR)/simgr/inband/include
SF_INCL=$(BASE_DIR)/sectionfilter/include
OOBSI_INCL=$(BASE_DIR)/simgr/oob/include
GSTQAMTUNERSRC_INCL=$(BASE_DIR)/gstqamtunersrc

PODMGR_DIR=$(BASE_DIR)/podmgr

SRC_DIR=.
LIBNAME=qamsrc

LIBFILE=$(LIBDIR)/lib$(LIBNAME).so

OBJ_DIR=$(BUILD_DIR)/objs_$(LIBNAME)

INCLUDES+= -I . -I $(HTTPSINK_INCL) \
		-I $(BASE_DIR) -I $(RMF_DIR)/core \
		-I $(BASE_DIR)/oobsimgrstub -I $(INBSI_INCL) -I $(SF_INCL) \
		-I${PLATFORM_SDK}/lib/glib-2.0/include \
		-I${PLATFORM_SDK}/include/glib-2.0 \
		-I${PLATFORM_SDK}/include/gstreamer-0.10 \
	    -I${PLATFORM_SDK}/include/libxml2 \
		-I$(OOBSI_INCL) \
		-I$(GSTQAMTUNERSRC_INCL)
#ifdef TEST_WITH_PODMGR
INCLUDES+=-I$(PODMGR_DIR)/inc
INCLUDES+=-I$(PODMGR_DIR)/canh 
#endif

ifdef IPPV_CLIENT_ENABLED
INCLUDES+=-I$(BASE_DIR)/ippv
endif

CFLAGS+= $(INCLUDES)

OBJS=$(OBJ_DIR)/rmfqamsrc.o $(OBJ_DIR)/rmfqamsrcpriv.o

ifndef TEST_WITH_PODMGR
LIBS += -loobsimgrstub -L $(LIBDIR)
endif

.PHONY : all clean
all: $(LIBFILE)

$(LIBFILE): $(LIBDIR) $(OBJ_DIR) $(OBJS)
	$(CC) -shared -o $(LIBFILE) $(OBJS) $(LIBS)

$(OBJ_DIR)/%.o :$(SRC_DIR)/%.cpp
	$(CC) -c -o $@ $< $(CFLAGS)
		
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)
	
$(LIBDIR):
	mkdir -p $(LIBDIR)
	
clean :
	rm -rf $(OBJ_DIR) $(LIBFILE)

