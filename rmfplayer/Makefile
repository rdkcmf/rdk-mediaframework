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
#######################################################################
# Makefile for rmfplayer
# For Intel build PLATFORM_SDK must be defined
# Also you may need to define FSROOT
# If PLATFORM_SDK is not defined it will do a Linux build which fails
# add Broadcom/Nexus build
# To add new source/sink
# 1) add a BUILD_X define to Source/Sink list
# 2) add a ifeq clause like the others to the Source/Sink additions
# 3) update rmfplayer.cpp
#######################################################################

BUILD_CONFIGURATION=RELEASE
BUILD_OUTPUT=rmfplayer

#begin Source/Sink list
BUILD_HNSOURCE=1
BUILD_MEDIAPLAYERSINK=1
#end Source/Sink list

M32_FLAG ?=-m32

ifdef PLATFORM_SDK

CFLAGS+=-fno-builtin-memcpy -fno-strict-aliasing -DFORMAT_ELF -D_INTERNAL_PCI_ -O2 -fPIC -MD $(M32_FLAG) -DGCC4_XXX $(SYSROOT_FLAG)

INCLUDE+=-I${PLATFORM_SDK}/lib/glib-2.0/include \
        -I${PLATFORM_SDK}/include/glib-2.0 \
        -I${PLATFORM_SDK}/include/gstreamer-0.10 \
        -I$(PLATFORM_SDK)/include \
        -I$(PLATFORM_SDK)/usr/local/include \
        -I$(PLATFORM_SDK)/usr/include \
        -I$(PLATFORM_SDK)/include/linux_user \
        -I${PLATFORM_SDK}/include/libxml2

LIB+=-L${PLATFORM_SDK}/lib \
        -L${PLATFORM_SDK}/lib/glib-2.0 \
        -L$(PLATFORM_SDK)/usr/lib \
        -L$(PLATFORM_SDK)/usr/local/lib \
        -L$(PLATFORM_SDK)/lib \
        -L$(FSROOT)/usr/local/lib \
        -L$(FSROOT)/lib \
        -lglib-2.0 \
        -lgstreamer-0.10 \
        -lhnsource \
        -lmediaplayersink \
        -lrmfcore
CC=$(GCC)
CXX=$(GXX)
else
    $(warning PLATFORM_SDK not defined, therefore, doing a Linux desktop build)
endif

ifeq ($(BUILD_CONFIGURATION),DEBUG)
    CFLAGS+=-g -DDEBUG -DDBG
endif

INCLUDE +=  -I$(RMF_DIR)/core
SOURCE  +=  rmfplayer.cpp
#end Source/Sink additions

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

all: build

build: clean $(OBJ)
	$(CXX) -o $(BUILD_OUTPUT) $(OBJ) $(LIB) --sysroot=$(PLATFORM_SDK)

.PHONY : clean distclean
clean:
	-$(RM) $(OBJ) $(DEP) $(BUILD_OUTPUT)

distclean:
