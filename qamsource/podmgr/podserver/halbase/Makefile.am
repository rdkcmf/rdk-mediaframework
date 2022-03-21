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
lib_LTLIBRARIES = libcablecarddriver.la

BUILD_CFLAGS = -DLINUX -fPIC -g -O2 -Wall -DLINUX -DLINUX26 -DGCC4_XXX -shared

libcablecarddriver_la_SOURCES = ${top_srcdir}/cablecarddriver.cpp

libcablecarddriver_la_CFLAGS =  $(BUILD_CFLAGS) -Wall -I${top_srcdir}/inc/headers -I${RDK_FSROOT_PATH}/usr/include

libcablecarddriver_la_LIBADD = -lpthread -L${RDK_FSROOT_PATH}

