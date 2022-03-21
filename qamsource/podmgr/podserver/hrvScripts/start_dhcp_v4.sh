#
# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2011 RDK Management
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
####

. /etc/device.properties
. /etc/env_setup.sh 

echo start_dhcp_v4.sh : Starting Dhcp V4...

if [ $SOC = "BRCM" ]; then 
    /etc/udhcpc_control restart >> /tmp/dhcp_log.txt 2>&1
else
    udhcpc -i $1 -s /etc/udhcpc_cm.script -V OpenCable2.1 -x &
    sleep 5
fi
ps | grep -E "udhcpc.*$1"
echo

