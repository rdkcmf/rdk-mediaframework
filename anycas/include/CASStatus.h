/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2021 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
**/

#ifndef CASSTATUS_H
#define CASSTATUS_H

namespace anycas {

typedef uint32_t CASStatus;

#ifdef USE_INLINE_WORKAROUND
#define INLINE inline
#else
#define INLINE
#endif

class CASStatusInform {
public:
	INLINE static constexpr CASStatus const& CASStatus_OK                     = 0x000000u;
	INLINE static constexpr CASStatus const& CASStatus_Blocked                = 0x0100FFu;
	INLINE static constexpr CASStatus const& CASStatus_Blocked_Video          = 0x010001u;
	INLINE static constexpr CASStatus const& CASStatus_Blocked_Audio          = 0x010002u;
	INLINE static constexpr CASStatus const& CASStatus_Blocked_Subtitle       = 0x010004u;
	INLINE static constexpr CASStatus const& CASStatus_Blocked_Other          = 0x010008u;
	INLINE static constexpr CASStatus const& CASStatus_GeneralErrorState      = 0x040000u;
	INLINE static constexpr CASStatus const& CASStatus_InternalError          = 0x040001u;
	INLINE static constexpr CASStatus const& CASStatus_HardwareError          = 0x040002u;
	INLINE static constexpr CASStatus const& CASStatus_NetworkError           = 0x040003u;
	INLINE static constexpr CASStatus const& CASStatus_VendorSpecificBlocked  = 0x810000u;
	INLINE static constexpr CASStatus const& CASStatus_VendorSpecificFatal    = 0x840000u;

    /**
     * Inform the status of a session
     * @param status
     */
    virtual void informStatus(const CASStatus& status) = 0;
public:
    CASStatusInform() {};
    virtual ~CASStatusInform() {};
};

} // namespace

#endif //CASSTATUS_H
