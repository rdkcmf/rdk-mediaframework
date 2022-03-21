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

echo The Reload Host Firmware flag, when set, shall force the Host to reload its firmware. If a reload of firmware entails
echo a deletion of the Host firmware, it may force the Host back to a “factory” state. A factory state can either be a
echo ROM’d version of code or a validated default version of code. If only one version of the Host firmware is
echo maintained, then the Host shall delete the old firmware only if it has already received and verified the availability
echo and viability of the new replacement firmware. If a Reset Host or Reset All flag is set in the Reset Field, the Host
echo shall be capable of processing the reload, while continuing through execution of the Reset Vector.
