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


INCLUDES+=-I$(RMF_DIR)/osal/include \
		-I$(RDK_DIR)/rdklogger/include \
		-I$(RMF_DIR)/core \
		-I$(RMF_DIR)/qamsource/main/ \
		-I$(RMF_DIR)/qamsource/simgr/inband/include \
		-I$(RMF_DIR)/platform/soc/soc_qamsource/sectionfilter/include \
		-I$(RMF_DIR)/qamsource/simgr/oob/include \
		-I$(RMF_DIR)/qamsource/podmgr/inc \
		-I$(RMF_DIR)/qamsource/podmgr/canh \
                -I${PLATFORM_SDK}/lib/glib-2.0/include \
                -I${PLATFORM_SDK}/include/glib-2.0 \
                -I${PLATFORM_SDK}/include/glib-2.0/glib \
                -I${PLATFORM_SDK}/include/gstreamer-0.10 \

ifndef TEST_WITH_PODMGR
	INCLUDES+=-I$(RMF_DIR)/qamsource/oobsimgrstub
endif

CFLAGS?=  -Wno-format -Wunused -DRMF_OSAL_LITTLE_ENDIAN -D_GNU_SOURCE -DGCC4_XXX

SRC_DIR=.
LIBNAME=ippvsource
LIBFILE=$(LIBDIR)/lib$(LIBNAME).a
OBJ_DIR=$(BUILD_DIR)/objs_$(LIBNAME)

CFLAGS+= $(INCLUDES)
OBJS=$(OBJ_DIR)/ippvclient.o $(OBJ_DIR)/ippvsrc.o 

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


