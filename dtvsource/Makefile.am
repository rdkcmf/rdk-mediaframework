##################################################################################
# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2020 RDK Management
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
# ============================================================================
#
# File originally created by The DTVKit Open Software Foundation Ltd (www.dtvkit.org)
# This file is part of a DTVKit Software Component and is
# licensed by RDK Management, LLC under the terms of the RDK license.
# The RDK License agreement constitutes express written consent by DTVKit.
#
# THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
# EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
##################################################################################

SUBDIRS = 
AM_CXXFLAGS = -pthread -Wall $(GST_CFLAGS) -I${top_srcdir}/core -I${top_srcdir}/osal/include -DRMF_OSAL_LITTLE_ENDIAN
lib_LTLIBRARIES = libdtvsource.la
libdtvsource_la_SOURCES = rmfdtvsource.cpp
libdtvsource_la_LDFLAGS = $(GST_LIBS) -lwebsockets -lrdkloggers -llog4c
libdtvsource_la_LIBADD = $(top_builddir)/core/librmfbase.la
libdtvsource_la_includedir = $(includedir)
libdtvsource_la_include_HEADERS = rmfdtvsource.h
