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

#ifndef ICASSECTIONFILTER_H
#define ICASSECTIONFILTER_H

#include "ICasHandle.h"
#include "ICasStatusReport.h"

namespace anycas {

#define FILTER_SIZE 10

typedef struct ICasFilterParam_ {
	typedef enum {
		SECTION_FILTER_REPEAT = 0,
		SECTION_FILTER_ONE_SHOT
	} FilterMode;
	uint8_t pos_value[FILTER_SIZE];		// << The pos value to apply to the filter
	uint8_t pos_mask[FILTER_SIZE];		// << The pos mask to apply to the filter
	uint32_t pos_size;					// << The number of valid bytes in the above

	uint8_t neg_value[FILTER_SIZE];		// << The neg value to apply to the filter
	uint8_t neg_mask[FILTER_SIZE];		// << The neg maskto apply to the filter
	uint32_t neg_size;					// << The number of valid bytes in the above

	bool disableCRC;
	bool noPaddingBytes;
	FilterMode mode;
} ICasFilterParam ;

typedef enum ICasFilterStatus {
	ICasFilterStatus_NoError = 0		// << No Error
};

class ICasSectionFilter {
public:
	virtual ~ICasSectionFilter() {};
protected:
	ICasSectionFilter() {};

public:
	/**
	 * Create a Section Filter
	 * @param pid The PID to create the filter on
	 * @param param Filter params @see ICasFilterParam
	 * @param flags Creation flags, a combination of ICasCreateFlags
	 * @param pHandle return handle if created correctly
	 * @return The status
	 */
	virtual ICasFilterStatus create(uint16_t pid, ICasFilterParam &param,
		ICasHandle **pHandle) = 0;

	/**
	 * Start a filter
	 * @param handle the handle to start the filter, object is released on successful return and should not be referenced
	 * @return status of command
	 */
	virtual ICasFilterStatus start(ICasHandle* handle) = 0;

	/**
	 * Run or Pause a filter
	 * @param isRunning true to run the filter, false to pause
	 * @param handle the handle to control
	 * @return status of command
	 */
	virtual ICasFilterStatus setState(bool isRunning, ICasHandle* handle) = 0;

	/**
	 * Destroy a filter
	 * @param handle the handle to destroy, object is released on successful return and should not be referenced
	 * @return status of command
	 */
	virtual ICasFilterStatus destroy(ICasHandle* handle) = 0;
};

}  // namespace

#endif /* ICASSECTIONFILTER_H */

