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

SUBDIRS = 
LOCAL_DEFINES =   -DMERGE_INTEL \
		  -DENABLE_NEW_LOGSVR \
		  -DCMCST_EXT_X1 \
		  -DUSE_POD \
		  -DUSE_DSG \
		  -DUSE_CDL \
		  -DMPOD_SUPPORT \
		  -DUSE_MFR \
		  -DUSE_TUNER \
		  -DNV_RAM_READ \
		  -DINTEL_PR13 \
		  -DINTEL_PR18 \
		  -DUSE_VIDSINK_API \
		  -g -O2 -Wall \
		  -fno-short-enums \
		  -ffloat-store \
		  -fno-strict-aliasing \
		  -DHAVE_STDINT_H \
		  -D_GNU_SOURCE \
		  -DMPE_FEATURE_DEBUG \
		  -DMPE_MEM_HANDLES \
		  -DRMF_OSAL_LITTLE_ENDIAN \
		  -DJGFX_DEV \
		  -D$(RMF_BUILD_OS) \
		  -D$(POD_PLATFORM) \
		  -DLINUX \
		  -DUSE_CC_GETTIMEOFDAY \
		  -DGCC4_XXX \
		  -DNUM_SURF=1 \
		  -DDEPRECATED_INTERFACE_SUPPORT \
		  -DUSE_NEW_MSPOD_DRIVER \
		  -DUSE_CDL_SERVICE \
		  -DTEST_WITH_PODMGR \
		  -DCOMMON_DOWNLOAD \
		  -DSNMP_IPC_CLIENT  \
		  -D$(MFR_NAME) \
		  -DIPPV_CLIENT_ENABLED  \
		  -DUSE_POD_IPC  \
		  -DUSE_PXYSRVC \
		  -DHAVE_STDINT_H \
		  -DUSE_QAMSRC \
		  -DGLI \
                  -DLOGMILESTONE
if HEADLESS
LOCAL_DEFINES+= -DHEADLESS_GW
else
LOCAL_DEFINES+= -DHEADED_GW
endif

AM_CXXFLAGS = -pthread -Wall -Wno-format -Wunused $(GLIB_CFLAGS) $(LOCAL_DEFINES)

AM_CFLAGS =  $(BREAKPAD_CFLAGS)
AM_CPPFLAGS =  $(BREAKPAD_CFLAGS)
bin_PROGRAMS = runPod

runPod_LDFLAGS = $(GLIB_LIBS)  -L${FSROOT}/usr/lib -L${FSROOT}/usr/local/lib -lrdkloggers -llog4c\
                 $(NETSNMP_LIBS) $(IARMBUS_LIBS) $(DSMGR_LIBS) $(MFR_LIBS) -ljansson ${DUNFELL_LIBC_LIBS} -ltirpc\
                 -lbreakpadwrapper

if HEADLESS
runPod_SOURCES = runPod.cpp 
else  
runPod_SOURCES = runPod.cpp frontpanel.cpp
endif  
runPod_LDADD = ${RDK_FSROOT_PATH}/usr/lib/librmfosal.so  \
	       ${RDK_FSROOT_PATH}/usr/lib/librmfosalutils.so \
	       ${RDK_FSROOT_PATH}/usr/lib/liboobsimanager.so \
	       ${RDK_FSROOT_PATH}/usr/lib/liboobsicache.so \
	       ${RDK_FSROOT_PATH}/usr/lib/libpodserver.so \
	       ${RDK_FSROOT_PATH}/usr/lib/libhal.so \
	       ${RDK_FSROOT_PATH}/usr/lib/librmfproxyservice.so \
	       ${RDK_FSROOT_PATH}/usr/lib/libinbsimanager.so \
	       ${RDK_FSROOT_PATH}/usr/lib/libsectionfilter.so \
	       ${RDK_FSROOT_PATH}/usr/lib/libinbsectionfilter.so \
               ${RDK_FSROOT_PATH}/usr/lib/liboobsectionfilter.so \
	       ${RDK_FSROOT_PATH}/usr/lib/libsocinit.so \
	       ${RDK_FSROOT_PATH}/usr/lib/libbreakpadwrapper.so \
               ${MFR_LIBS}
