#
# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2013 RDK Management
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
#

CC=$RDK_DIR/sdk/toolchain/staging_dir/bin/i686-cm-linux-g++
$CC -o ippv_test -DRMF_OSAL_LITTLE_ENDIAN -I $RDK_DIR/mediaframework/osal/include -I$RDK_DIR/mediaframework/qamsource/podmgr/ippv -lrmfosal -lrmfosalutils -llog4c -pthread  -lglib-2.0 -lsysResource  -losal -L $PLATFORM_SDK/lib  -L $RDK_DIR/mediaframework/build/lib -L $RDK_DIR/opensource/lib/ -L $RDK_DIR/sys_mon_tools/sys_resource/lib/platform/canmore/ ../ippvclient.cpp ippv_test.cpp 



#-lsysResource -L $(RDK_DIR)/sys_mon_tools/sys_resource/lib/platform/canmore/
