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

SUBDIRS = 
AM_CPPFLAGS = -pthread -Wall
noinst_LTLIBRARIES = libsrc.la
libsrc_la_SOURCES = rmf_osal_file.c rmf_osal_ipc.cpp rmf_osal_storage.c rmf_osal_thread.c rmf_osal_file_hd.c rmf_osal_mem.c rmf_osal_storage_priv.c rmf_osal_time.c rmf_osal_event.c rmf_osal_init.c rmf_osal_socket.c rmf_osal_sync.c rmf_osal_util.c
libsrc_la_CFLAGS =  $(GLIB_CFLAGS) -I$(srcdir)/include -I$(srcdir)/../include -I$(srcdir)/../utils/inc -DDISABLE_XFS -DRMF_OSAL_FEATURE_IPV6 -DRMF_OSAL_LITTLE_ENDIAN -DENV_CONF_FILE="\"rmfconfig.ini\""
libsrc_la_CXXFLAGS =  $(GLIB_CFLAGS) -I$(srcdir)/include -I$(srcdir)/../include -I$(srcdir)/../utils/inc -DDISABLE_XFS -DRMF_OSAL_FEATURE_IPV6 -DRMF_OSAL_LITTLE_ENDIAN -DENV_CONF_FILE="\"rmfconfig.ini\""
libsrc_la_LDFLAGS = $(GLIB_LIBS) 
