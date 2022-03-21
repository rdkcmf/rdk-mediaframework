/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2018 RDK Management
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
*/

#ifndef _SIUTIL_H_
#define _SIUTIL_H_

#include "rmf_osal_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

// --------------------------------------------------------------
//                          LINKED LISTS
// --------------------------------------------------------------

/*
 * Format of a list link.
 */
typedef struct link LINK;
struct link
{
    LINK * nextp; /* List foreward pointer */
    LINK * prevp; /* List back pointer */
    struct linkhdr * headerp; /* Pointer to list link is in */
    void * datap; /* Pointer to arbitrary data */
};

/*
 * Format of a list header.
 */
typedef struct linkhdr LINKHD;
struct linkhdr
{
    LINK * headp; /* Pointer to first link */
    LINK * tailp; /* pointer to last link */
    unsigned long nlinks; /* Number of links in the list */
};

LINKHD * llist_create(void);
void llist_free(LINKHD *headerp);
LINK * llist_mklink(void * datap);
void llist_freelink(LINK *linkp);
void llist_append(LINKHD *headerp, LINK *linkp);
LINK * llist_first(LINKHD * headerp);
LINK * llist_after(LINK * linkp);
LINK * llist_last(LINKHD * headerp);
void llist_rmlink(LINK * linkp);
void llist_rmfirst(LINKHD * headerp);
void llist_rmafter(LINK * afterp);
void llist_rmlast(LINKHD * headerp);
LINK * llist_setdata(LINK * linkp, void * datap);
void * llist_getdata(LINK * linkp);
unsigned long llist_cnt(LINKHD * headerp);
LINK * llist_linkof(LINKHD * headerp, void * datap);
LINK * llist_nextlinkof(void * datap, LINK * afterp);
LINK * llist_getNodeAtIndex(LINKHD * headerp, unsigned long index);
LINKHD * llist_hdrof(LINK * lp);

typedef LINKHD * ListSI;

// --------------------------------------------------------------
//                          CRC UTILITY FUNCTIONS
// --------------------------------------------------------------

void init_mpeg2_crc(void);

uint32_t calc_mpeg2_crc(uint8_t * data, uint16_t len);

#define SI_CACHE_FILE_VERSION 0x100u

#ifdef __cplusplus
}
;
#endif

#endif /* _SIUTIL_H_ */
