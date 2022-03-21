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
PWD=`pwd`

#Check if realpath is available
which realpath
if [ $? -ne 0 ]; then
  REALPATH=echo
else
  REALPATH=realpath
fi

export RDK_DIR=$PWD/../
source platform/soc/scripts/hal_env.sh

#SETTING SOURCE DIRECTORY AND PLATFORM DEPENDENT LIBRARY PATH
export SH_LIB_PATH=${PWD}/../lib
export INSTALLDIR=${PWD}/../lib


for components in  dvr/common/gstelements/aesdecrypt dvr/common/gstelements/aesencrypt dvr/common/gstelements/dvrsink dvr/common/gstelements/dvrsrc 
do
  echo =========================================================================================================================================================
  echo --------------------------BUILDING $components ---------------------------------
  echo =========================================================================================================================================================
  if [ "$1" = "clean" ] ; then
    make -C $components clean
  fi
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
