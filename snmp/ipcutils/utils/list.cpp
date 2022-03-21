/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2014 RDK Management
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
#include <rmf_osal_mem.h>
#include <si_util.h>

#if USE_SYSRES_MLT
#include "rpl_new.h"
#endif

/*
 * =========================================================================
 * Create and initialize a list header.
 * Returns a pointer to the header, or NULL on error.
 * =========================================================================
 */



LINKHD * llist_create(void)
{
    LINKHD * hp;

    /*
     * Get space.
     */
    if (RMF_SUCCESS != rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(LINKHD),
            (void **) &(hp)))
    {
        return NULL;
    }

    /*
     * Init list to empty and return it.
     */
    hp->headp = NULL;
    hp->tailp = NULL;
    hp->nlinks = 0;

    return hp;
}

/*
 * =========================================================================
 * Destroy (free) a list header and all it's links.
 *  headerp     header of list to destroy.
 * Returns headerp or NULL on error.
 * =========================================================================
 */
void llist_free(LINKHD *headerp)
{
    LINK * lp;

    while (1)
    {
        lp = llist_first(headerp);
        if (lp == NULL)
            break;

        llist_rmfirst(headerp);
        llist_freelink(lp);
    }

    rmf_osal_memFreeP(RMF_OSAL_MEM_POD, headerp);
}

/*
 * =========================================================================
 * Create a new link, and bind the 'user' data.
 *  datap       pointer to arbitrary data.
 * Returns a pointer to the new link, or NULL on error.
 * =========================================================================
 */
LINK * llist_mklink(void * datap)
{
    LINK * lp;

    /*
     * Get a free link.
     */
    if (RMF_SUCCESS != rmf_osal_memAllocP(RMF_OSAL_MEM_POD, sizeof(LINK),
            (void **) &(lp)))
    {
        return NULL;
    }

    lp->datap = datap;
    return lp;
}

/*
 * =========================================================================
 * Destroy (free) a link.
 *  linkp       pointer to link to destroy.
 * The link must not be in a list.
 * Returns a pointer to the freed link, or NULL on error.
 * Really oughta return BOOL status, instead of ptr to free memory.
 * =========================================================================
 */
void llist_freelink(LINK *linkp)
{
    rmf_osal_memFreeP(RMF_OSAL_MEM_POD, linkp);
}

/*
 * =========================================================================
 * Append a link to the list tail.
 *  headerp     pointer to the list header.
 *  linkp       the link to append.
 * The link must not already be in a list.
 * Returns a pointer to the link, or NULL on error.
 * =========================================================================
 */
void llist_append(LINKHD *headerp, LINK *linkp)
{
    linkp->nextp = NULL;
    linkp->headerp = headerp;
    linkp->prevp = headerp->tailp;

    if (headerp->tailp)
        headerp->tailp->nextp = linkp;

    headerp->tailp = linkp;

    if (headerp->headp == NULL)
        headerp->headp = linkp;

    ++headerp->nlinks;
}

/*
 * =========================================================================
 * Return a pointer to the first link in the list.
 *  headerp     the list to look in.
 * Returns the link pointer found, or NULL on error.
 * =========================================================================
 */
LINK * llist_first(LINKHD * headerp)
{
    return headerp->headp;
}

/*
 * =========================================================================
 * Return a pointer to the last link in the list.
 *  headerp     the list to look in.
 * Returns the link pointer found, or NULL on error.
 * =========================================================================
 */
LINK * llist_last(LINKHD * headerp)
{
    return headerp->tailp;
}

/*
 * =========================================================================
 * Return a pointer to the link after the given link.
 *  afterp      the link to look after.
 * Returns the link pointer found, or NULL on error.
 * =========================================================================
 */
LINK * llist_after(LINK * afterp)
{
    return afterp->nextp;
}

/*
 * =========================================================================
 * Remove specified link from it's list and return a pointer to it.
 * The link is not destroyed.
 *  linkp       the link to remove.
 * Returns the link removed, or NULL on error.
 * =========================================================================
 */
void llist_rmlink(LINK *linkp)
{
    LINKHD * hp = linkp->headerp;

    if (linkp == hp->headp)
        llist_rmfirst(hp);
    else
        llist_rmafter(linkp->prevp);
}

/*
 * =========================================================================
 * Remove the first link from the list and return a pointer to it.
 * The link is not destroyed.
 *  headerp     the list to look in.
 * Returns the link removed, or NULL on error.
 * =========================================================================
 */
void llist_rmfirst(LINKHD * headerp)
{
    LINK * lp = headerp->headp;
    if (lp == NULL)
        return;

    headerp->headp = lp->nextp;
    if (lp->nextp != NULL)
        lp->nextp->prevp = NULL;

    if (headerp->tailp == lp)
        headerp->tailp = NULL;

    --headerp->nlinks;

    lp->nextp = NULL;
    lp->prevp = NULL;
    lp->headerp = NULL;
}

/*
 * =========================================================================
 * Remove the link after the given link and return a pointer to it.
 * The link is not destroyed.
 *  afterp      the link to look after.
 * Returns the link removed, or NULL on error.
 * =========================================================================
 */
void llist_rmafter(LINK * afterp)
{
    if (afterp->nextp == NULL)
        return;

    if (afterp->nextp == afterp->headerp->tailp)
    {
        llist_rmlast(afterp->headerp);
    }
    else
    {
        LINK * lp = afterp->nextp;

        lp->nextp->prevp = lp->prevp;
        afterp->nextp = lp->nextp;

        --afterp->headerp->nlinks;

        lp->nextp = NULL;
        lp->prevp = NULL;
        lp->headerp = NULL;
    }
}

/*
 * =========================================================================
 * Remove the last link from the list and return a pointer to it.
 * The link is not destroyed.
 *  headerp     the list to look in.
 * Returns the link removed, or NULL on error.
 * =========================================================================
 */
void llist_rmlast(LINKHD * headerp)
{
    LINK * lp = headerp->tailp;
    if (lp == NULL)
        return;

    headerp->tailp = lp->prevp;
    if (lp->prevp != NULL)
        lp->prevp->nextp = NULL;

    if (headerp->headp == lp)
        headerp->headp = NULL;

    --headerp->nlinks;

    lp->nextp = NULL;
    lp->prevp = NULL;
    lp->headerp = NULL;
}

/*
 * =========================================================================
 * Change the arbitrary data pointer bound to a link.
 *  linkp       the link to rebind.
 *  datap       pointer to be bound.
 * Returns linkp, or NULL on error.
 * =========================================================================
 */
LINK * llist_setdata(LINK * linkp, void * datap)
{
    linkp->datap = datap;
    return linkp;

}

/*
 * =========================================================================
 * Return the arbitrary data pointer bound to a link.
 *  linkp       the link to look at.
 * Returns a void pointer, or NULL on error.
 * =========================================================================
 */
void * llist_getdata(LINK * linkp)
{
    return linkp->datap;
}

/*
 * =========================================================================
 * Return the number of links in a list.
 *  headerp     the list to look at.
 * Returns a link count, 0 on error.
 * =========================================================================
 */
unsigned long llist_cnt(LINKHD * headerp)
{
    return headerp->nlinks;
}

/*
 * =========================================================================
 * Return the first link in the list that is bound to the given data
 * pointer. Note that nothing prevents a data pointer from being bound to
 * more than one link.
 *  headerp     the list to look in.
 *  datap       the data pointer to look for.
 * Returns the link found, or NULL on error.
 * =========================================================================
 */
LINK * llist_linkof(LINKHD * headerp, void * datap)
{
    LINK * lp = headerp->headp;

    while (lp != NULL)
        if (lp->datap == datap)
            break;
        else
            lp = lp->nextp;

    return lp;
}

/*
 * =========================================================================
 * Return the next link after the given link that is bound to the given data
 * pointer. Note that nothing prevents a data pointer from being bound to
 * more than one link.
 *  afterp      the link to start from.
 *  datap       the data pointer to look for.
 * Returns the link found, or NULL on error.
 * =========================================================================
 */
LINK * llist_nextlinkof(void * datap, LINK * afterp)
{
    LINK * lp = afterp->nextp;
    while (lp != NULL)
        if (lp->datap == datap)
            break;
        else
            lp = lp->nextp;
    return lp;
}

LINK * llist_getNodeAtIndex(LINKHD * headerp, unsigned long index)
{
    unsigned long i = 0;
    LINK * lp = headerp->headp;

    if (index == 0)
        return headerp->headp;

    for (i = 0; i < headerp->nlinks && lp; i++)
    {
        if (i == index)
        {
            return lp;
        }
        else
        {
            lp = lp->nextp;
        }
    }

    return NULL;
}

/*
 * =========================================================================
 * Return pointer to the header of the list a link is in,
 * or NULL if not in a list, or not a link.
 * =========================================================================
 */
LINKHD * llist_hdrof(LINK * lp)
{
    return lp->headerp;
}

