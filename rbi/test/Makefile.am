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
AM_CPPFLAGS = -pthread -Wall $(GST_CFLAGS) -I${top_srcdir}/osal/include \
	           -I${top_srcdir}/core -I${top_srcdir}/mediaplayersink -I${top_srcdir}/rbi -I${top_srcdir}/dvr/source -I${top_srcdir}/dvr/sink -I${top_srcdir}/dvr/common/dvrmgr -I${top_srcdir}/dvr/common/tasseograph -I${top_srcdir}/qamsource/main -I${top_srcdir}/qamsource/podmgr/inc -I${top_srcdir}/qamsource/simgr/inband/include -I${top_srcdir}/qamsource/common \
	           -DRMF_OSAL_FEATURE_IPV6 -DRMF_OSAL_LITTLE_ENDIAN -DUSE_MEDIAPLAYERSINK \
	           -DGCC4_XXX $(QAM_CFLAGS) $(DVR_CFLAGS) $(UUID_CFLAGS)

if ENABLE_PODMGR
AM_CPPFLAGS += -DTEST_WITH_PODMGR
endif

BUILT_SOURCES=../rbi.o ../RBIFilter.o  ../../core/.libs/librmfbase.so.0.0.0

bin_PROGRAMS=rbiDaemon

rbiDaemon_SOURCES= rbiDaemon.cpp

if OSAL_ENABLE
rbiDaemon_LADD = ${top_builddir}/osal/librmfosal.la
rbiDaemon_LADD += ${top_builddir}/osal/utils/librmfosalutils.la
rbiDaemon_LADD += ${top_builddir}/core/librmfbase.la
else
rbiDaemon_LADD = -lrmfosal -lrmfosalutils -lrmfbase
endif

rbiDaemon_LDFLAGS= \
	-L${top_builddir}/core/.libs \
	-lrdkloggers \
	-llog4c \
	-ltinyxml \
	-lcurl \
	-luuid \
	-lIARMBus \
	$(IARMBUS_LIBS)

