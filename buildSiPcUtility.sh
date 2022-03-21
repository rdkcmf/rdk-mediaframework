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
export RDK_DIR=$PWD/../
export PLATFORM_SDK=$RDK_DIR/sdk/toolchain/staging_dir
export TOOLCHAIN_DIR=$PLATFORM_SDK
export GST_SOURCE=$PLATFORM_SDK

if [ "$1" = "clean" ];
then
    echo "Cleaning the build..."
    make -C ../tools/generate_si_cache clean
    if [ $? -ne 0 ] ;
    then
      echo si_cache pc utility clean failed
      exit -1
    fi

    make -C qamsource clean
    if [ $? -ne 0 ] ;
    then
      echo qamsource clean failed
      exit -1
    fi

    make -C osal clean
    if [ $? -ne 0 ] ;
    then
      echo osal clean failed
      exit -1
    fi

exit 0
fi

make -C osal
if [ $? -ne 0 ] ;
then
  echo osal build failed 
  exit -1
fi

make -C qamsource
if [ $? -ne 0 ] ;
then
  echo qamsource build failed 
  exit -1
fi

make -C ../tools/generate_si_cache
if [ $? -ne 0 ] ;
then
  echo si_cache pc utility build failed 
  exit -1
fi
