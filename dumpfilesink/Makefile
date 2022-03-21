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
CC?=g++
AR?=ar
#CFLAGS= -Wall -g
CFLAGS?=  -Wno-format -Wunused -DUSE_CC_GETTIMEOFDAY -DRMF_OSAL_FEATURE_IPV6 -DRMF_OSAL_LITTLE_ENDIAN
#-DRMF_OSAL_FEATURE_MEM_PROF
ARFLAGS?=rcs
SRC_DIR=.

LIBNAME=dumpfilesink
LIBFILE=$(LIBDIR)/lib$(LIBNAME).a
OBJ_DIR=$(BUILD_DIR)/objs_$(LIBNAME)

RMF_HTTPSINK_INCLUDE_DIR=$(RMF_DIR)/mediastreamer

INCLUDES+= -I . -I ../core  -I${PLATFORM_SDK}/lib/glib-2.0/include \
	-I${PLATFORM_SDK}/include/glib-2.0 \
	-I${PLATFORM_SDK}/include/gstreamer-0.10 \
	-I${PLATFORM_SDK}/include/libxml2 \
	-I$(RDK_DIR)/gst-plugins/generic/src/httpsink \
	-I$(RDK_DIR)/dtcp/include \
	-I$(RDK_DIR)/rdklogger/include \
	-I$(RMF_HTTPSINK_INCLUDE_DIR)
            
CFLAGS+= $(INCLUDES)  #-DUSE_APP_SINK
OBJS=$(OBJ_DIR)/dumpfilesink.o  


.PHONY : all clean test
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
	rm -rf $(OBJ_DIR) $(LIB)

	
