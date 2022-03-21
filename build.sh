#
# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2014 RDK Management
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
set -x
PWD=`pwd`

#Check if realpath is available
which realpath
if [ $? -ne 0 ]; then
  REALPATH=echo
else
  REALPATH=realpath
fi

export RDK_DIR=`${REALPATH} $PWD/..`
source platform/soc/scripts/hal_env.sh

# -------- Four config modes ------------
#    HEADED_GW : Both server and client
#    HEADLESS_GW : Only server(streamer)
#    CLIENT_TSB : Limited streamer and client 
#    CLIENT : only client 
configMode=HEADED_GW
buildMode=""
instalLibs=0
instaldir=build
setupSDK=0

for option in $@
do
  echo option = $option
  if [ "$option" = "clean" ]; then
    buildMode="clean"
  elif [ "$option" = "install" -o  "$option" = "instal" ] ; then
    instalLibs=1
  elif [ "$option" = "setup" ] ; then
    setupSDK=1
  elif [ "$option" = "headless" ] ; then
    configMode=HEADLESS_GW
	XG5_GW=1
	INTEL_XG5=1
 fi
done


export cfgMode=${BASH_ARGV[0]}
case "$cfgMode" in
   "mediaclient") export configMode=CLIENT_TSB;;
   "headlessmediaclient") export configMode=CLIENT_TSB;;
   "legacy") export configMode=HEADED_GW;;
   "hybrid") export configMode=HEADED_GW;;
esac

echo buildMode = $buildMode
echo configMode = $configMode
echo instalLibs = $instalLibs
 
# SOC specific settings, may override generic settings, and alter some option (e.g. setup)
SOC_BUILDRMF=platform/soc/scripts/soc_config.sh
if [ -e ${SOC_BUILDRMF} ]; then
	source ${SOC_BUILDRMF}
fi

export configMode
  
if [ $setupSDK -eq 1 ];
then
  echo $PLATFORM_SDK
  if [ -d "$PLATFORM_SDK" ]
  then
	  echo "toolchain is already installed..."
  else
	  echo Installing toolchain, it may take few seconds depends on your system
	  tar zxf $RDK_DIR/sdk/toolchain/staging_dir.tgz -C $RDK_DIR/sdk/toolchain
	  echo "toolchain installed $PLATFORM_SDK"
  fi


  echo $SNMP_DIR
  if [ -d "$SNMP_DIR" ]
  then
	  echo "SNMP dir is already installed..."
  else
	  echo Installing SNMP dir, it may take few seconds depends on your system
	  tar zxf $RDK_DIR/opensource/target-snmp.tar.gz -C $RDK_DIR/opensource
	  echo "SNMP dir installed $SNMP_DIR"
  fi

  echo $ROOTFS
  if [ -d "$ROOTFS" ]
  then
	  echo "ROOT FS is already extracted..."
  else
	  echo extracting ROOT FS, it may take few seconds depends on your system
	  sudo tar zxf $RDK_DIR/sdk/fsroot/fsroot.tgz -C $RDK_DIR/sdk/fsroot
	  sudo tar zxf $RDK_DIR/sdk/fsroot/curl.tgz -C $RDK_DIR/sdk/fsroot
	  sudo tar zxf $RDK_DIR/sdk/fsroot/mafLib.tgz -C $RDK_DIR/sdk/fsroot
	  echo "ROOT FS is extracted $ROOTFS"
  fi
fi

if [ "x$disableTranscode" = "x" ]
then
  use_transcoderfilter=transcoderfilter
fi

if [ "x$disableIppvsource" = "x" ]
then
  use_ippvsource=ippvsource
fi

if [ "x$disableVodsource" = "x" ]
then
  use_vodsource=vodsource
fi

if [ "x$disableDiagnostics" = "x" ]
then
  use_diagnostics=diagnostics
fi

if [ "x$disableTrh" = "x" ]
then
  use_trh=trh
fi

if [ "x$disablesnmp" = "x" ]
then
  use_snmp=snmp
fi
if [ "x$enabledtv" = "x" ]
then
  use_dtv=dtvsource
fi
if [ "$configMode" = "CLIENT" ] 
then
  buildList+="init osal $use_transcoderfilter hnsource $use_diagnostics $use_snmp"
  echo "No RMF component needed to build for CLIENT mode"
elif [ "$configMode" = "CLIENT_TSB" ] ; then
  buildList+="init osal $use_transcoderfilter hnsink hnsource dumpfilesink dvr/common/tasseograph dvr/common/dvrmgr $use_diagnostics $use_snmp"
elif [ "$configMode" = "NODVR_GW" ] ; then
  buildList+="init osal $use_transcoderfilter hnsink hnsource $use_trh qamsource dumpfilesink $use_ippvsource $use_vodsource $use_diagnostics $use_snmp $use_dtv"
else # Full mode .. headless gateway
  buildList="init osal rmfproxyservice $use_transcoderfilter hnsink hnsource $use_trh qamsource dumpfilesink $use_ippvsource $use_vodsource dvr/common/tasseograph dvr/common/dvrmgr $use_diagnostics $use_snmp $use_dtv"
fi

if [ "$buildMode" = "clean" ];
then
  echo "Cleaning the build and $instaldir"
  rm -rf build/objs_*
  for components in $buildList rmfApp
  do
    make -C $components clean
    if [ $? -ne 0 ] ;
    then
      echo $components clean failed 
      #exit -1
    fi
  done
  find -name "*.o" | xargs rm
  find -name "*.d" | xargs rm
fi

for components in $buildList
do
  echo =========================================================================================================================================================
  echo --------------------------BUILDING $components ---------------------------------
  echo =========================================================================================================================================================
  make -C $components
  if [ $? -ne 0 ] ;
  then
    echo $components build failed 
    exit -1
  fi
  echo ========================================================================================================================================================
  echo --------------------------BUILDING $components DONE ---------------------------------
  echo ========================================================================================================================================================
done

if [ "$configMode" != "CLIENT" ]
then
  echo =========================================================================================================================================================
  echo --------------------------BUILDING DVR gstreamer-plugins ---------------------------------
  echo =========================================================================================================================================================
  sh buildGstPlugin.sh $buildMode
  if [ $? -ne 0 ] ;
  then
    echo gst-plugins build failed 
    exit -1
  fi
    
  echo ========================================================================================================================================================
  echo --------------------------BUILDING DVR gstreamer-plugins DONE ---------------------------------
  echo ========================================================================================================================================================
fi

echo =========================================================================================================================================================
echo --------------------------BUILDING rmfApp ---------------------------------
echo =========================================================================================================================================================
  make -C rmfApp $buildMode all
if [ $? -ne 0 ] ;
then
  echo rmfApp build failed 
  exit -1
fi
echo ========================================================================================================================================================
echo --------------------------BUILDING rmfApp DONE ---------------------------------
echo ========================================================================================================================================================

#echo =========================================================================================================================================================
#echo --------------------------BUILDING rmfplayer ---------------------------------
#echo =========================================================================================================================================================
#  make -C rmfplayer
#if [ $? -ne 0 ] ;
#then
#  echo rmfplayer build failed 
#  exit -1
#fi
#echo ========================================================================================================================================================
#echo --------------------------BUILDING rmfplayer DONE ---------------------------------
#echo ========================================================================================================================================================

#echo =========================================================================================================================================================
#echo --------------------------BUILDING mplayer ---------------------------------
#echo =========================================================================================================================================================
#  make -C mediaplayer
#if [ $? -ne 0 ] ;
#then
#  echo mplayer build failed 
#  exit -1
#fi
#echo ========================================================================================================================================================
#echo --------------------------BUILDING mplayer DONE ---------------------------------
#echo ========================================================================================================================================================

# Update the build date in STACK_VERSION.txt - the imagename will be updated during packaging.
months="Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec"
month=`date | cut -f2 -d " "`
string="${months%$month*}"
dec_month="$((${#string}/4 + 1))"

day=`date | cut -f4 -d " "`
year=`date | cut -f7 -d " "`
old_day=`grep -r DATE_DAY STACK_VERSION.txt | cut -f2 -d"="`
old_month=`grep -r DATE_MONTH STACK_VERSION.txt | cut -f2 -d"="`
old_year=`grep -r DATE_YEAR STACK_VERSION.txt | cut -f2 -d"="`
sed -i -e "s/DATE_DAY=$old_day/DATE_DAY=$day/g" STACK_VERSION.txt
sed -i -e "s/DATE_MONTH=$old_month/DATE_MONTH=$dec_month/g" STACK_VERSION.txt
sed -i -e "s/DATE_YEAR=$old_year/DATE_YEAR=$year/g" STACK_VERSION.txt

if [ $instalLibs -eq 1 ];
then
  echo installing binaries.... configMode is $configMode
  mkdir -p $instaldir/env
  mkdir -p $instaldir/gstreamer-plugins
  mkdir -p $FSROOT/mnt/nfs/env
  mkdir -p $FSROOT/mnt/nfs/lib
  mkdir -p $FSROOT/mnt/nfs/gstreamer-plugins
  
#    cp qamsource/*.ini $instaldir/env
    cp qamsource/rmfconfig.ini $instaldir/env
#    cp qamsource/log4crc $instaldir/env
    cp qamsource/*.xml $instaldir/env
    cp STACK_VERSION.txt $instaldir/env
    cp snmp/runsnmp/runSnmp $instaldir/env
    cp snmp/snmpinterface/libsnmpinterface.so $instaldir/lib/
    #cp snmp/halsnmp/libhalsnmp.so $instaldir/lib/
	cp snmp/ipcclient/libipcclient.so $instaldir/lib/
	cp snmp/ipcutils/libipcutils.so $instaldir/lib/
	cp snmp/snmpmanager/libsnmpmanager.so $instaldir/lib/

  if [ "$configMode" != "CLIENT_TSB" -a "$configMode" != "CLIENT" ] ; then
    # ------- Copy qamsource config --------
    mv build/lib/libgstqamtunersrcplugin.so $instaldir/gstreamer-plugins
    cp qamsource/podmgr/runpod/runPod $instaldir/env
    mkdir -p $FSROOT/mnt/nfs/bin/scripts/
    cp -f qamsource/podmgr/podserver/hrvScripts/*.sh $FSROOT/mnt/nfs/bin/scripts/
  fi
  
  if [ "$configMode" != "CLIENT"  -a "$configMode" != "NODVR_GW" ] ; then
      # ------- Copy DVR stuff ------------
      cp dvr/common/tasseograph/lib*.so $instaldir/lib/	
      cp dvr/common/dvrmgr/lib*.so $instaldir/lib/
      cp dvr/common/dvrmgr/dvmtest $instaldir/env
      # ----- Copy dvr gstreamer plugins --------------------
      cp dvr/common/gstelements/aesdecrypt/lib*.so $instaldir/gstreamer-plugins
      cp dvr/common/gstelements/aesencrypt/lib*.so $instaldir/gstreamer-plugins
      cp dvr/common/gstelements/dvrsink/lib*.so $instaldir/gstreamer-plugins
      cp dvr/common/gstelements/dvrsrc/lib*.so $instaldir/gstreamer-plugins
  fi
  cp -a ../gst-plugins/lib/*.so  $instaldir/gstreamer-plugins
  
  # ---------- copy RMFApiCaller ------------------
  cp $instaldir/rmfapicaller $instaldir/env
  
  # ---------- copy RMFApp ------------------
  cp $instaldir/rmfapicaller $instaldir/env
  cp $instaldir/rmfthreadanalyzer $instaldir/env
  cp rmfApp/rmfApp $instaldir/env
  cp rmfApp/runRMFApp $instaldir/env
#  cp rmfplayer/rmfplayer $instaldir/env
#  cp mediaplayer/mplayer $instaldir/env
  cp -a $instaldir/env/* $FSROOT/mnt/nfs/env
  cp -a $instaldir/lib/* $FSROOT/mnt/nfs/lib
  cp -a $instaldir/gstreamer-plugins/* $FSROOT/mnt/nfs/gstreamer-plugins
  
  if [ "$configMode" != "CLIENT_TSB" -a "$configMode" != "CLIENT" ] ; then
  # ---------- copy diagnostic ------------------
  mkdir -p $FSROOT/diagnostics
  cp -r diagnostics/snmp2json* $FSROOT/diagnostics
  mkdir -p $FSROOT/opt_back
  # cp -r diagnostics/www $FSROOT/opt_back
  fi
exit 0
fi

