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
BASE_DIR=$(RMF_DIR)/qamsource
#CFLAGS= -Wall -g
CFLAGS?=  -Wno-format -Wunused -DUSE_CC_GETTIMEOFDAY -DRMF_OSAL_FEATURE_IPV6 -DRMF_OSAL_LITTLE_ENDIAN
#-DRMF_OSAL_FEATURE_MEM_PROF
ARFLAGS?=rcs
#OOBSI_DIR=$(BASE_DIR)/simgr/oob
RMFOSAL_DIR=$(RMF_DIR)/osal
SRC_DIR=.
OPEN_LIBS=$(RDK_DIR)/opensource/lib
TST_DIR=.
TST_BIN=rmfproxyservice_test
PROXYSERVICE_LIBNAME=rmfproxyservice
PROXYSERVICE_LIBFILE=$(LIBDIR)/lib$(PROXYSERVICE_LIBNAME).so

OBJ_DIR=$(BUILD_DIR)/objs_$(PROXYSERVICE_LIBNAME)
#TEST_OBJ_DIR=$(BUILD_DIR)/objs_$(TST_BIN)

INCLUDES+= -I . \
	   -I $(RMFOSAL_DIR)/include \
          -I $(RMF_DIR)/core

#           -I$(RMF_DIR)/qamsource/simgr/inband/include \
	   -I $(OOBSI_DIR)/include \
	   -I$(RMF_DIR)/qamsource/sectionfilter/include \
	   -I$(RMF_DIR)/qamsource/podmgr/podserver/inc \
	   -I$(RMF_DIR)/qamsource/podmgr/inc 
 

ifeq ($(USE_SYS_RESOURCE_MLT), y)
INCLUDES+= -I $(SYSRES_DIR)/sys_resource/include
endif

ifeq ($(USE_SYS_RESOURCE_MLT), y)
ifeq ($(RDK_PLATFORM_SOC),broadcom)
SYSRESOURCE_LIBS = -L $(SYSRES_DIR)/sys_resource/lib/platform/broadcom -lsysResource
else
SYSRESOURCE_LIBS = -L $(SYSRES_DIR)/sys_resource/lib/platform/canmore -lsysResource
endif
endif

ifeq ($(USE_SYS_RESOURCE_MON), y)
ifeq ($(RDK_PLATFORM_SOC),broadcom)
SYSRESOURCE_LIBS = -L $(SYSRES_DIR)/sys_resource/lib/platform/broadcom -lsysResource
else
SYSRESOURCE_LIBS = -L $(SYSRES_DIR)/sys_resource/lib/platform/canmore -lsysResource
endif
endif

#fdef XONE_STB
#ACE_SHLIBS=-lPaceDsgIf -ldsg_rpc_cm2h -lPaceDiagApi -lPaceMfrApi -lPaceEcmApi -lPaceTempControl -lPaceNvramApi -lPacePosixApi -lflash_appdata -lidl_i2c -lnand_config -lmfh
#ndif

#fdef XG5_GW
#XG5_SHLIBS= -larrismfr -lflash_appdata -ldsg_rpc_cm2h -lidl_i2c -lnand_config -lmfh -litstore
#endif

#WHOLE_LIBS=$(OPENSRC_LIBS) $(OPENSSL_LIBS) $(PACE_SHLIBS) $(INTEL_SHLIBS) $(SNMPSHLIBS) $(XG5_SHLIBS)

#LD_FLAGS=  -loobsectionfilter -loobsicache -loobsimanager -lsectionfilter $(WHOLE_LIBS) $(INTERNAL_LIBS) $(SYSRESOURCE_LIBS)


CFLAGS += $(INCLUDES)
PROXYSERVICE_OBJS= $(OBJ_DIR)/rmfproxyservice.o \
                   $(OBJ_DIR)/T2PClient.o

#TESTS=$(TEST_OBJ_DIR)/rmfproxyservice_test.o

.PHONY : all clean
#all: test
all: $(PROXYSERVICE_LIBFILE)

$(PROXYSERVICE_LIBFILE): $(LIBDIR) $(OBJ_DIR) $(PROXYSERVICE_OBJS)
	$(CXX) -shared -L $(LIBDIR) -lc -lrt -lrmfosal -lrmfosalutils $(SYSRESOURCE_LIBS) -o $@ $(CFLAGS) ${PROXYSERVICE_OBJS} $(SYSROOT_FLAG)

$(OBJ_DIR)/%.o :$(SRC_DIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)/%.o :$(SRC_DIR)/%.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)
	
$(LIBDIR):
	mkdir -p $(LIBDIR)

#$(TEST_OBJ_DIR):
#	mkdir -p $@

#$(TEST_OBJ_DIR)/%.o :$(TST_DIR)/%.c
#	$(CC) -c -o $@ $< $(CFLAGS)

clean :
	rm -rf $(OBJ_DIR) $(PROXYSERVICE_LIBFILE)

#test : $(TEST_OBJ_DIR) $(LIBFILE) $(TESTS)
#	$(CC) -o $(BUILD_DIR)/$(TST_BIN) $(TESTS) -l$(PROXYSERVICE_LIBNAME) -llog4c $(RDK_DIR)/opensource/src/openssl/libcrypto.a -lrmfosalutils -lpodserver -lrmfosal -llog4c -Wl,--start-group $(LD_FLAGS) -Wl,--end-group $(WHOLE_LIB_PATH) $(SYSRESOURCE_LIBS) -L $(LIBDIR) -L $(OPEN_LIBS) $(TST_FLAGS) --sysroot=$(TOOLCHAIN_DIR)


	
