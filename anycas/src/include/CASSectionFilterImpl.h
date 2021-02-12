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

#ifndef CASSECTIONFILTERIMPL_H
#define CASSECTIONFILTERIMPL_H

#include "CASSectionFilter.h"
#include "CASSectionFilterResponse.h"
#include "ICasSectionFilter.h"

namespace anycas {

class CASSectionFilterImpl : public CASSectionFilter {
public:
    CASSectionFilterImpl(std::shared_ptr<CASHelperContext> context, ICasSectionFilter *streamFilter)
        : CASSectionFilter(context),
          upstreamFilter_(streamFilter) {}
    virtual ~CASSectionFilterImpl();

    void startSectionFilter();

    void destroySectionFilter();

    void setState(CASSessionState state);

    void setFilterResponse(std::shared_ptr<CASSectionFilterResponse> filterResp);

    static std::shared_ptr<CASSectionFilterResponse> getFilterResponse(const uint32_t& filterId);

private:
    ICasSectionFilter *upstreamFilter_;
    std::shared_ptr<CASSectionFilterResponse> filterResponse_;     //To findout the sectiondata for the corresponding (session & context)
    static std::vector<std::shared_ptr<CASSectionFilterResponse>> filterResponseList_;	//To findout the sectiondata for the corresponding (session & context)
};

} // namespace

#endif /* CASSECTIONFILTERIMPL_H */