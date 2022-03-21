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
. /etc/include.properties
. /etc/device.properties
. /etc/env_setup.sh 
. $RDK_PATH/soc_specific_fn.sh
DHCP_SCRIPT_LOCATION="/mnt/nfs/bin/scripts"


LOCKFILE=/tmp/`basename $0`.lock
if [ -e ${LOCKFILE} ] && kill -0 `cat ${LOCKFILE}`; then
    echo "$0 is already running"
    exit
fi

# make sure the lockfile is removed when we exit and then claim it
trap "rm -f ${LOCKFILE}; exit" INT TERM EXIT
echo $$ > ${LOCKFILE}

loop=0
while [ $loop -eq 0 ];
do
     if [ -f /tmp/estb_ipv6 ]; then
          echo "TLV_IP_MODE: IPv6 Mode..!"
          loop=1
     elif [ -f /tmp/estb_ipv4 ]; then
          echo "TLV_IP_MODE: IPv4 Mode..!"
          loop=1
     else
          echo "Waiting for TLV flag (v4/v6)"
          sleep 1
     fi
done

entryCheck=`cat /tmp/resolv.conf | grep nameserver | grep 127.0.0.1`
if [ "x$entryCheck" = "x" ];then
     echo -e "nameserver \t127.0.0.1" >> /tmp/resolv.conf
fi

#echo -e "nameserver \t::1" >> /tmp/resolv.conf

if [ ! -f /etc/os-release ]; then
    sh $DHCP_SCRIPT_LOCATION/stop_dhcp_v4.sh $1
    sh $DHCP_SCRIPT_LOCATION/stop_dhcp_v6.sh $1
fi

if [ -f /tmp/estb_ipv6 ]; then
    if [ -f /etc/os-release ]; then
       systemctl restart dibbler.service
    else
       sh $DHCP_SCRIPT_LOCATION/start_dhcp_v6.sh $1 &
    fi
else
    if [ -f /etc/os-release ]; then
       systemctl restart udhcp.service
    else
       sh $DHCP_SCRIPT_LOCATION/start_dhcp_v4.sh $1 &
    fi
fi

if [ -f ${LOCKFILE} ];then rm -rf ${LOCKFILE}; fi

