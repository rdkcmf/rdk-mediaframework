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

. /etc/env_setup.sh
. /etc/device.properties
. /etc/include.properties

Timestamp()
{
    date +"%Y-%m-%d %T"
}

DIBBLER_LOG="$LOG_PATH/dibbler.log"

echo "`Timestamp` start_dhcp_v6.sh : Starting Dhcp V6..."  >> $DIBBLER_LOG

if [ ! -f /etc/os-release ];then
    # exit if an instance is already running
    echo "`Timestamp` Checking for previous running instance of start_dhcp_v6.sh" >> $DIBBLER_LOG
    if [ ! -f /tmp/.v6_client.pid ];then
        # store the PID
        echo $$ > /tmp/.v6_client.pid
    else
        pid=`cat /tmp/.v6_client.pid`
        if [ -d /proc/$pid ];then
             echo "`Timestamp` Previous instance with pid $pid of start_dhcp_v6.sh is running !!! Exiting " >> $DIBBLER_LOG
             exit 0
        fi
    fi
fi

echo "`Timestamp` DHCP_CLIENT  : $DHCP_CLIENT"  >> $DIBBLER_LOG

if [ "$DHCP_CLIENT" = "dhcp6c" ]; then 
    mkdir -p /opt/dhcpv6/

    if [ -f /opt/dhcpv6/dhcp6c_duid ]; then                                    
       cp /opt/dhcpv6/dhcp6c_duid /var/db/dhcp6c_duid                   
    fi                        

    dhcp6c -d -c /usr/local/etc/dhcp6c.conf -f -D $1 &
    sleep 5
    ps | grep -E "dhcp6c.*$1"
    echo
    if [ -f /var/db/dhcp6c_duid ]; then               
        cp /var/db/dhcp6c_duid /opt/dhcpv6/dhcp6c_duid
    fi      
else
    echo "`Timestamp` Preparing DHCP v6 Config file for dibbler"  >> $DIBBLER_LOG
    if [ -f /lib/rdk/prepare_dhcpv6_config.sh ]; then
 	/lib/rdk/prepare_dhcpv6_config.sh
    fi
    echo "`Timestamp` DHCP v6 Config file generation complete"  >> $DIBBLER_LOG
    mkdir -p /opt/dibbler                                                      
    mkdir -p /tmp/dibbler
    if [ -f /opt/dibbler/client-duid ]; then                                    
       cp /opt/dibbler/client-duid /tmp/dibbler/client-duid
    fi

    /usr/local/sbin/dibbler-client start >> $DIBBLER_LOG 2>&1 &
    echo "`Timestamp` dibbler-client started Successfully !!!" >> $DIBBLER_LOG
    sleep 5

    ps | grep -E "dibbler-client" >> $DIBBLER_LOG

    if [ -f /tmp/dibbler/client-duid ]; then
       cp /tmp/dibbler/client-duid /opt/dibbler/client-duid
    fi

    if [ -f /tmp/dibbler/client.pid ]; then 
        startPid=`cat /tmp/dibbler/client.pid`
        duid=`cat /tmp/dibbler/client-duid`
        echo "`Timestamp` dibbler-client started with pid $startPid and DUID $duid" >> $DIBBLER_LOG
    fi
    
    if [ -f /tmp/dibbler/client-AddrMgr.xml ]; then
        t1t2Timeout=`grep -i 'AddrIA unicast' /tmp/dibbler/client-AddrMgr.xml`
        lifeTime=`grep -i 'AddrAddr timestamp' /tmp/dibbler/client-AddrMgr.xml`
        echo "`Timestamp` dhcp server response " >> $DIBBLER_LOG
        echo "$t1t2Timeout" >> $DIBBLER_LOG
        echo "$lifeTime" >> $DIBBLER_LOG
        echo "======================================" >> $DIBBLER_LOG
    fi
    if [ ! -f /etc/os-release ];then 
        echo "`Timestamp` clearing start_dhcp_v6.sh execution flags ." >> $DIBBLER_LOG
        rm -f /tmp/.v6_client.pid
    fi
fi
