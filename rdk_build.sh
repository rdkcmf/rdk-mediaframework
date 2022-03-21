#!/bin/bash
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

#######################################
#
# Build Framework standard script for
#
# MediaFramework component

# use -e to fail on any shell issue
# -e is the requirement from Build Framework
set -e


# default PATHs - use `man readlink` for more info
# the path to combined build
export RDK_PROJECT_ROOT_PATH=${RDK_PROJECT_ROOT_PATH-`readlink -m ..`}
export COMBINED_ROOT=$RDK_PROJECT_ROOT_PATH

# path to build script (this script)
export RDK_SCRIPTS_PATH=${RDK_SCRIPTS_PATH-`readlink -m $0 | xargs dirname`}

# path to components sources and target
export RDK_SOURCE_PATH=${RDK_SOURCE_PATH-$RDK_SCRIPTS_PATH}
export RDK_TARGET_PATH=${RDK_TARGET_PATH-$RDK_SOURCE_PATH}

# fsroot and toolchain (valid for all devices)
export RDK_FSROOT_PATH=${RDK_FSROOT_PATH-`readlink -m $RDK_PROJECT_ROOT_PATH/sdk/fsroot/ramdisk`}
export RDK_TOOLCHAIN_PATH=${RDK_TOOLCHAIN_PATH-`readlink -m $RDK_PROJECT_ROOT_PATH/sdk/toolchain/staging_dir`}


# default component name
export RDK_COMPONENT_NAME=${RDK_COMPONENT_NAME-`basename $RDK_SOURCE_PATH`}
export RDK_DIR=${RDK_PROJECT_ROOT_PATH}
source ${RDK_SCRIPTS_PATH}/platform/soc/scripts/hal_env.sh
export PKG_CONFIG_LIBDIR=${RDK_FSROOT_PATH}/lib/pkgconfig
export LDFLAGS="$LDFLAGS -Wl,-rpath-link=$RDK_FSROOT_PATH/usr/local/lib"


# parse arguments
INITIAL_ARGS=$@

function usage()
{
    set +x
    echo "Usage: `basename $0` [-h|--help] [-v|--verbose] [action]"
    echo "    -h    --help                  : this help"
    echo "    -v    --verbose               : verbose output"
    echo
    echo "Supported actions:"
    echo "      configure, clean, build (DEFAULT), rebuild, install"
}

# options may be followed by one colon to indicate they have a required argument
if ! GETOPT=$(getopt -n "build.sh" -o hv -l help,verbose -- "$@")
then
    usage
    exit 1
fi

eval set -- "$GETOPT"

while true; do
  case "$1" in
    -h | --help ) usage; exit 0 ;;
    -v | --verbose ) set -x ;;
    -- ) shift; break;;
    * ) break;;
  esac
  shift
done

ARGS=$@


# component-specific vars
export FSROOT=$RDK_FSROOT_PATH


# functional modules

function configure()
{
	pd=`pwd`
	cd ${RDK_SOURCE_PATH}
	aclocal -I cfg
	libtoolize --automake
	autoheader
	automake --foreign --add-missing
	rm -f configure
	autoconf
    	echo "  CONFIG_MODE = $CONFIG_MODE"
	configure_options=" "
	if [ "x$DEFAULT_HOST" != "x" ]; then
	configure_options="--host $DEFAULT_HOST"
        fi
        configure_options="$configure_options --enable-shared --with-pic"
        generic_options="$configure_options"

        export ac_cv_func_malloc_0_nonnull=yes
        export ac_cv_func_memset=yes
        extraenablers="--enable-osal --enable-mediaplayersink"
	if [ "$RDK_PLATFORM_DEVICE" == "rng150" ];then
        extraenablers="$extraenablers --enable-rmfproxyservice --enable-hnsink --enable-ippvsource --enable-vodsource --enable-qam --enable-snmp --enable-dumpfilesink --disable-xfs --enable-podmgr"
	elif [ "x$RDK_PLATFORM_VENDOR" = "xcisco" ] ;then
        extraenablers="$extraenablers --enable-rmfproxyservice --enable-hnsink --enable-ippvsource --enable-vodsource --enable-tasseograph --enable-dvrmgr --enable-sharedtsb --enable-dvrsource --enable-dvrsink --enable-trh --enable-qam --enable-snmp --enable-dumpfilesink"
	elif [ "$RDK_COMCAST_PLATFORM" == "XG2" ];then
        extraenablers="$extraenablers --enable-rmfproxyservice --enable-hnsink --enable-ippvsource --enable-vodsource --enable-tasseograph --enable-dvrmgr --enable-sharedtsb --enable-dvrsource --enable-dvrsink --enable-rbi --enable-rbidaemon --enable-trh --enable-qam --enable-snmp --enable-dumpfilesink --disable-xfs"
	elif [ "x$RDK_PLATFORM_SOC" == "xintel" ] ;then
        extraenablers="$extraenablers --enable-rmfproxyservice --enable-hnsink --enable-ippvsource --enable-vodsource --enable-tasseograph --enable-dvrmgr --enable-sharedtsb --enable-dvrsource --enable-dvrsink --enable-rbi --enable-rbidaemon --enable-trh --enable-qam --enable-snmp --enable-dumpfilesink"
	elif [ "$RDK_PLATFORM_DEVICE" != "xi3" ] && [ "$RDK_PLATFORM_DEVICE" != "xi4" ];then
        extraenablers="$extraenablers --enable-rmfproxyservice --enable-hnsink --enable-ippvsource --enable-vodsource --enable-tasseograph --enable-dvrmgr --enable-sharedtsb --enable-dvrsource --enable-dvrsink --enable-trh --enable-qam --enable-snmp --enable-dumpfilesink --enable-rbitest --enable-rbi --enable-rbidaemon"
        else
        extraenablers="$extraenablers --enable-hnsink --enable-tasseograph --enable-dvrmgr --enable-sharedtsb --enable-dvrsource --enable-dvrsink --enable-dumpfilesink --disable-xfs"
        fi

        if [ "x$RDK_PLATFORM_SOC" = "xintel" ];then
           extraenablers="$extraenablers --enable-transcoderfilter"
		if [ "x$RDK_PLATFORM_DEVICE" = "xIntelXG5" ];then
			extraenablers="$extraenablers --enable-headless"
		fi
	fi


	#Enabling SDV Component 
if [ -d $RDK_PROJECT_ROOT_PATH/sdvagent ];then
		extraenablers="$extraenablers --enable-sdv"
	fi	

        if [ "x$RDK_PLATFORM_DEVICE" = "xxg1" ] && [ "x$BUILD_CONFIG" = "x" ];then       
               extraenablers="--enable-mediaplayersink --enable-hnsink --enable-tasseograph --enable-dvrmgr --enable-sharedtsb --enable-dvrsource --enable-dvrsink --enable-dumpfilesink"
         fi

        if [ "$RDK_PLATFORM_SOC" = "stm" ];then
           configure_options="$configure_options --with-libtool-sysroot=${RDK_FSROOT_PATH}"
           autoreconf --verbose --force --install
        fi
     
         if [ "$RDK_PLATFORM_SOC" = "stm" ];then
            ./configure --prefix=/usr $configure_options $extraenablers
         else 
            ./configure --prefix=${RDK_FSROOT_PATH}/usr $configure_options $extraenablers
        fi
	cd $pd
}

function clean()
{
    pd=`pwd`
    dnames="${RDK_SOURCE_PATH} ${RDK_SOURCE_PATH}/rmfApp"
    for dName in $dnames
    do
        cd $dName
 	if [ -f Makefile ]; then
    		make distclean
	fi
    	rm -f configure;
    	rm -rf aclocal.m4 autom4te.cache config.log config.status libtool
    	find . -iname "Makefile.in" -exec rm -f {} \; 
        if [ "$RDK_PLATFORM_SOC" = "stm" ];then
            find . -name Makefile | grep -v "aes" | xargs rm -rf
        else
    	    find . -iname "Makefile" -exec rm -f {} \; 
        fi
    	ls cfg/* | grep -v "Makefile.am" | xargs rm -f
    	cd $pd
    done
}

function build()
{
    cd ${RDK_SOURCE_PATH}

    if [ "x$RDK_PLATFORM_SOC" != "xintel" ];then
        sed -i '/platform\/soc\/dvrmgr/d' ./configure.ac
    fi

    make
    #make install
    #make -C qamsource
    #make -C snmp
}

function rebuild()
{
    clean
    configure
    build
}

function install()
{
    cd ${RDK_SOURCE_PATH}

    if [ "$RDK_PLATFORM_SOC" = "stm" ];then
       make install DESTDIR=${RDK_FSROOT_PATH}
    else
       make install
    fi

    if [ -d qamsource/podmgr/podserver/hrvScripts ]; then
	mkdir -p $FSROOT/mnt/nfs/bin/scripts/
	cp -f qamsource/podmgr/podserver/hrvScripts/*.sh $FSROOT/mnt/nfs/bin/scripts/
    fi

    if [ -f qamsource/rmfconfig.ini ]; then
    cp qamsource/rmfconfig.ini $FSROOT/etc
    fi
    cp qamsource/*.xml $FSROOT/usr/bin
    cp STACK_VERSION.txt $FSROOT/usr/bin
    mkdir -p $FSROOT/opt_back
    if [ "x${RDK_PLATFORM_DEVICE^^}" == "xXG1" ] && [ "x${RDK_PLATFORM_SOC^^}" == "xBROADCOM" ];then
       if [ "x${RDK_PLATFORM_VENDOR^^}" == "xPACE" ]; then
        if [ "x${RDK_COMCAST_PLATFORM^^}" == "xXG2" ]; then
          if [ -f platform/soc/scripts/rmfconfig.ini.7429.pace ]; then
             cp -f platform/soc/scripts/rmfconfig.ini.7429.pace $FSROOT/etc/rmfconfig.ini
          fi
       else
          if [ -f platform/soc/scripts/rmfconfig.ini.7435.pace ]; then
             cp -f platform/soc/scripts/rmfconfig.ini.7435.pace $FSROOT/etc/rmfconfig.ini
          fi
       fi
       elif [ "x${RDK_PLATFORM_VENDOR^^}" == "xSAMSUNG" ]; then
          if [ -f platform/soc/scripts/rmfconfig.ini.7429.samsung ]; then
             cp -f platform/soc/scripts/rmfconfig.ini.7429.samsung $FSROOT/etc/rmfconfig.ini
          fi
       elif [ "x${RDK_PLATFORM_VENDOR^^}" == "xARRIS" ]; then
          if [ "x${MOTO_PLATFORM}" == "xXG1_v3" ]; then   
	         if [ -f platform/soc/scripts/rmfconfig.ini.7435.arris ]; then
                cp -f platform/soc/scripts/rmfconfig.ini.7435.arris $FSROOT/etc/rmfconfig.ini
                 fi
          else
	         if [ -f platform/soc/scripts/rmfconfig.ini.7425 ]; then
                cp -f platform/soc/scripts/rmfconfig.ini.7425 $FSROOT/etc/rmfconfig.ini
             fi
          fi
       elif [ "x${RDK_PLATFORM_VENDOR^^}" == "xCISCO" ]; then
          if [ -f platform/soc/scripts/rmfconfig.ini.7425.cisco ]; then
             cp -f platform/soc/scripts/rmfconfig.ini.7425.cisco $FSROOT/etc/rmfconfig.ini
          fi
       else 
          if [ -f platform/soc/scripts/rmfconfig.ini ]; then
             cp -f platform/soc/scripts/rmfconfig.ini $FSROOT/etc/rmfconfig.ini
          fi
       fi
       if [ -f platform/soc/scripts/run.sh ]; then
          cp -f platform/soc/scripts/run.sh $FSROOT/etc/
       fi
    fi
    if [ "x${RDK_PLATFORM_DEVICE^^}" == "xRNG150" ] && [ "x${RDK_PLATFORM_SOC^^}" == "xBROADCOM" ];then
       if [ "x${RDK_PLATFORM_VENDOR^^}" == "xPACE" ]; then
          if [ -f platform/soc/scripts/rmfconfig.ini.7125.pace ]; then
             cp -f platform/soc/scripts/rmfconfig.ini.7125.pace $FSROOT/etc/rmfconfig.ini
          fi
       elif [ "x${RDK_PLATFORM_VENDOR^^}" == "xSAMSUNG" ]; then
          if [ -f platform/soc/scripts/rmfconfig.ini.7125.samsung ]; then
             cp -f platform/soc/scripts/rmfconfig.ini.7125.samsung $FSROOT/etc/rmfconfig.ini
          fi
       else 
          if [ -f platform/soc/scripts/rmfconfig.ini ]; then
             cp -f platform/soc/scripts/rmfconfig.ini $FSROOT/etc/rmfconfig.ini
          fi
       fi
       if [ -f platform/soc/scripts/run.sh ]; then
          cp -f platform/soc/scripts/run.sh $FSROOT/etc/
       fi
   fi
   if [ "x${RDK_PLATFORM_DEVICE^^}" == "xXI3" ] && [ "x${RDK_PLATFORM_SOC^^}" == "xBROADCOM" ];then
       if [ -f platform/soc/scripts/rmfconfig.ini.7428 ]; then
          cp -f platform/soc/scripts/rmfconfig.ini.7428 $FSROOT/etc/rmfconfig.ini
       fi
       if [ -f platform/soc/scripts/run.sh ]; then
          cp -f platform/soc/scripts/run.sh $FSROOT/etc/
       fi
   fi

   if [ "x${RDK_PLATFORM_DEVICE}" == "xxi4" ] && [ "x${RDK_PLATFORM_SOC}" == "xstm" ];then
      cp -f platform/soc/scripts/rmfconfig.ini $FSROOT/etc/rmfconfig.ini
   fi
}


# run the logic

#these args are what left untouched after parse_args
HIT=false

for i in "$ARGS"; do
    case $i in
        configure)  HIT=true; configure ;;
        clean)      HIT=true; clean ;;
        build)      HIT=true; build ;;
        rebuild)    HIT=true; rebuild ;;
        install)    HIT=true; install ;;
        *)
            #skip unknown
        ;;
    esac
done

# if not HIT do build by default
if ! $HIT; then
  rebuild
fi
