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
. /etc/include.properties
. /etc/env_setup.sh

DIBBLER_LOG="$LOG_PATH/dibbler.log"
echo "`Timestamp` Checking for previous running instance of start_dhcp_v6.sh" >> $DIBBLER_PIPE
if [ -f /tmp/.v6_client.pid ];then
    pid=`cat /tmp/.v6_client.pid`
    if [ -d /proc/$pid ];then
         echo "`Timestamp` Previous instance with pid $pid of start_dhcp_v6.sh is running !!! Exiting " >> $DIBBLER_PIPE
         exit 0
    fi
fi

echo stop_dhcp_v6.sh : Stopping Dhcp V6...
if [ "$DHCP_CLIENT" = "dhcp6c" ]; then
#ps | grep "dhcp6c.*$1" | sed 's/\(^ *\)\([0-9]*\)\(.*\)/\2/g' | xargs kill
    ps | grep -E "dhcp6c.*$1" | grep -v grep | tr -s ' ' | cut -d ' ' -f2 | xargs kill
else
    # Better to use stop to enable graceful shutdown to possible extend 
    /usr/local/sbin/dibbler-client stop
    sleep 2
    ps | grep -E "dibbler-client" | grep -v grep | tr -s ' ' | cut -d ' ' -f2 | xargs kill
fi
echo

