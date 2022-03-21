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

if USE_EMULATOR_QAM 
SUBDIRS_PODMGR = 
else
if BUILD_PODMGR
if !ENABLE_YOCTO
SUBDIRS_PODMGR = podmgr
else
SUBDIRS_PODMGR = 
endif
endif
endif

SUBDIRS_DEFAULT = simgr oobsimgrstub main

SUBDIRS =  $(SUBDIRS_DEFAULT) $(SUBDIRS_PODMGR)
DIST_SUBDIRS =  $(SUBDIRS_DEFAULT) $(SUBDIRS_PODMGR)

if ENABLE_DVB
SUBDIRS += dvb
DIST_SUBDIRS += dvb
endif

if ENABLE_SDV
SUBDIRS += sdv
DIST_SUBDIRS += sdv
endif


EXTRA_DIST = common/rmf_qamsrc_common.h  \
             common/rmf_sectionfilter.h  \
             common/rmf_sectionfilter_util.h  \
             common/rmf_symboltable.h

libqam_la_includedir = $(includedir)
libqam_la_include_HEADERS = $(EXTRA_DIST)
