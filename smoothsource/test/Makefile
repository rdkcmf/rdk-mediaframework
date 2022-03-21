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
BUILD_OUTPUT=smoothtest
#CFLAGS+=-fno-builtin-memcpy -fno-strict-aliasing -DFORMAT_ELF -D_INTERNAL_PCI_ -O2 -fPIC -MD -m32
CFLAGS+=-g -DDEBUG -DDBG
CXXFLAGS=$(CFLAGS)

PLATFORM_SDK=/home/mark/X1/br_1.0_PC25.2_K39/combined/sdk/toolchain/staging_dir

INCLUDE=-I$(PLATFORM_SDK)/include \
        -I$(PLATFORM_SDK)/usr/local/include \
        -I$(PLATFORM_SDK)/include/glib-2.0 \
        -I$(PLATFORM_SDK)/lib/glib-2.0/include \
        -I$(PLATFORM_SDK)/include/linux_user \
        -I$(PLATFORM_SDK)/include/libxml2 \
        -I/home/mark/Fluendo/binaries/ccplatform_all_dev_X1_PR23/usr/local/ccplatform/include/gstreamer-0.10 \
        -I../ \
        -I../../core \
        -I../../../xre/include

LIB=-L$(PLATFORM_SDK)/usr/lib \
    -L$(PLATFORM_SDK)/usr/local/lib \
    -L$(PLATFORM_SDK)/lib \
    -L/home/mark/Fluendo/binaries/ccplatform_all_dev_X1_PR23/usr/local/ccplatform/lib/gstreamer-0.10 \
    -L../../../xre/bin/smoothvideo \
    -lxress \
    -lplayready \
    -lismd_core \
    -lgstreamer-0.10 \
    -lgstbase-0.10

CC=$(PLATFORM_SDK)/bin/i686-cm-linux-gcc
CXX=$(PLATFORM_SDK)/bin/i686-cm-linux-g++
AR=$(PLATFORM_SDK)/bin/i686-cm-linux-ar
LD=$(PLATFORM_SDK)/bin/i686-cm-linux-ld

SOURCE= main.cpp sampvidsink.cpp ../smoothsource.cpp ../../core/rmfbase.cpp

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

all: build #stage

build: $(OBJ)
	$(CXX) -o $(BUILD_OUTPUT) $(OBJ) $(LIB)

.PHONY : clean
clean:
	-$(RM) $(OBJ) $(DEP) $(BUILD_OUTPUT)


