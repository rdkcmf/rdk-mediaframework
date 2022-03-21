/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2011 RDK Management
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



#ifndef _PODAPI_H_
#define _PODAPI_H_

#include <cardmanagergentypes.h>
#include <mrerrno.h>        // MR error codes
//#include <mr.h>             // MR-specific defines 'n stuff   //cpmmented by Hannah

#ifdef __cplusplus
extern "C" {
#endif

/*! @name POD API Library

    The POD public API component of the Media Receiver is the sole interface
    for POD resources (applications) with the Passport UI. These applications
    are described in the primary POD references listed below.

    Note: In this public API, the term "POD" refers to a particular POD
          resource that is executing within the SD process space and
          *not* the POD (CableCARD) module itself. From the viewpoint of
          the UI, however, the distinction is unimportant and the UI can
          assume it is dealing directly with the POD.
    
    The POD API for the UI is implemented as a static library that contains
    the functions described here. These functions communicate with the POD
    via the LCSM event message mechanism.

    The UI must initialize the POD API, by calling pod_Open, before using any
    of the API's functionality. Upon a successful return from pod_Open, the POD
    API functionality is available to the UI. The UI should call pod_Close when
    it no longer intends to use the API.

    Assumptions
      - interface is simple C-style (eg: no name overloading)
      - the UI stores and maintains all user-selected settings

    References
      Primary
        [1] National Renewable Security Standard (NRSS), CEA-679-B, March 2000
        [2] HOST-POD Interface Standard, ANSI/SCTE-28 2003 (formerly DVS 295)
      Additional
        [3] Proposed Amendment 1 to SCTE-28 2001, Host-POD Interface Standard,
            SCTE DVS 519r2, 2 October 2002
        [4] POD Copy Protection System, ANSI/SCTE-41 2003 (formerly DVS 301)
        [5] Service Information Delivered Out-of-Band for Digital Cable
            Television, SCTE-65 2002 (formerly DVS 234)
 */
//@{


/**************************************
  *   V e r s i o n   c o n t r o l   *
 **************************************/

/*! defining POD_DEBUG does the following things
 *    - when message queue is flushed, the list of flushed messages is
 *      displayed
 *    - stand-alone logging is possible (see podutils.h)
 */
#define POD_DEBUG       // at least for awhile...

/*! Media Receiver (MR) supports at least 2 versions (dates are tentative)
 *    - Regular (Jun 2004) - same as 'demo' plus POD OOB PSI support
 *    - Elite (Sep 2004) - same as 'Regular' plus EPG grid for current/future
 *      events
 *  Only define a single version type
 */
#define MR_REGULAR        // Regular model features
//#define MR_ELITE        // Elite model features


/*******************************
 *   Application Information   *
 *******************************/

typedef enum
{
    eFullScreen = 0,    // single, full-screen window only
    eOverlay    = 1,    // mult full-screen windows, only 1 displayed at a time
    eMultiple   = 2     // mult partial-screen windows
        /* remaining values 3-255 are reserved for future use */
} eMultiWin_t;

// the following enum contains values that are used in multiple Application Info
// enums defined below. Consequently, this enum is not used as a type and, hence,
// has no name defined for it. Only its values are used
// NB: any changes to the names or assigned values in this enum
//     *MUST* be reflected in the corresponding enums below!


typedef enum
{
  eNone       = 0,    // no user input
    eLastNext   = 1,    // arrows ??
    eNumericPad = 2,    // just numbers
    eKeyboard   = 3     // alphanumeric keyboard with mouse
        /* remaining values 4-255 are reserved for future use */
} eDataEntry_t;

typedef enum
{
    eBaseline   = 0,    // see Appendix C in [2]
    eCustom     = 1,
  eHtml_32    = 2,    // HTML 3.2
    eXhtml_10   = 3,    // XHTML 1.0
        /* remaining values 4-255 are reserved for future use */
} eHtml_t;

typedef enum
{
    eLinkone = 0,        // one link support
    eLinkMul = 1,// multiple links support
    //0x02-0xFF Reserved
}eLink_support;
typedef enum
{
  eFormNone       = 0,    // no forms capability
    eFormNoPost     = 1,    // HTML 3.2 w/o POST method
  eformHtml_32    = 2     // HTML 3.2
        /* remaining values 3-255 are reserved for future use */
} eForm_t;

typedef enum
{
    eListNone       = 0,    // no list capability
    eListDesc    = 1,    // HTML 3.2 w/o Descriptive Lists
   eListHtml_32    = 2     // HTML 3.2
/* remaining values 3-255 are reserved for future use */
} eList_t;

typedef enum
{
  eImageNone       = 0,    // no image capability
    eImageResize   = 1,    // HTML 3.2 PNG RGB image w/o resizing
  eImageHtml_32    = 2     // HTML 3.2
        /* remaining values 3-255 are reserved for future use */
} eImage_t;

//Display capbities needs to be passed from the Display Application
typedef struct
{
    uint32  header;         // 24-bit tag & 8-bit APDU byte length
                            //    nb: UI should *NOT* fill this field

    uint16  displayRows;    // number of displayable rows (vert pixels),if value is more than 255 then set 0xFF
    uint16  displayCols;    // number of displayable columns (horiz pixels),if value is more than 255 then set 0xFF
    uint8   vertScroll;     // boolean indication of vertical scroll support
                //0x00 vertical scroll not supported
                //0x01 vertical scroll is supported
    uint8   horizScroll;    // boolean indication of horizontal scroll support
                  //0x00 horizontal scroll not supported
                    //0x01 horizontal scroll is supported
    uint8   multiWin;       // window display capability:
                            //0X00, full screen
               //0x01 Overlay
           //0x02-0x6F// Multiple simultaneous windows
        // The value equals the max number of simultaneous windows
    uint8   dataEntry;      // data entry capability:
                            //    use values defined in eDataEntry_t above
    uint8   html;           // HTML rendering capability:
                            //    use values defined in eHtml_t above

        // remaining fields only applicable if 'html' == 1 (eCustom)
    uint8   link;           // single (0)/multiple (1) link capability
    uint8   form;           // forms capability:
                            //    use values defined in eForm_t above
    uint8   table;          // no (0)/HTML 3.2 table (1) capability
    uint8   list;           // list capability:
                            //    use values defined in eList_t above
    uint    image;          // image capability:
                            //    use values defined in eImage_t above
} pod_appInfo_t;

/*******************************
 *   Server query type         *
 *******************************/
typedef struct
{
    uint8   transNum;       // Transaction number
    uint16  headerLength;   // Length of the header field in bytes
    uint8   *header;        // Header field
    uint16  urlLength;      // Length of the url field in bytes
    uint8   *url;           // URL field

} pod_serverQuery_t;

/*! UI opens POD API by informing POD of display/HTML capabilities
 
    The UI notifies the POD of the Media Receiver display and HTML capabilities
    during UI initialization and if/when the capabilities change. This call also
    makes the POD API available for the UI's use. The UI initializes the
    values in the passed pod_appInfo_t (defined above) to reflect MR display
    capabilities including resolution and richness of its HTML rendering engine,
    if any. A global handle to LCSM is opened that can be used by the rest of
    the API, if it is not currently open.
    NB: pod_Open *must* be called prior to calling any other functions in the
        POD public API otherwise -ERRNOINIT is returned
    @pre none
    @param info [IN] Description of display and HTML rendering engine
    capabilities
    @return Status of API initialization (EOK indicates success). If the values
    in the passed info buffer fail verification, a failure status (EINVAL) can
    be returned. Unless EOK is returned, the initialization failed and the API
    cannot be used
    @version 10/31/03 - prototype
 */
int pod_Open( pod_appInfo_t * info );

/*! UI closes POD API for use

    Close the POD API from further UI use. The global handle to LCSM is closed
    preventing any further use of the API until another call to pod_Open()
    @pre pod_Open must have been called first
    @param none
    @return Status of API close operation (EOK indicates success)
    @version 11/17/03 - prototype
 */
int pod_Close( void );

/*! UI retrieves appInfo data returned from POD
 
    This function should be called in when the UI received

        MSG_POD_AI_APPINFO_CNF

    event at some later time indicating that the requested data is available
    by calling this function.
    @pre pod_Open must have been called first
    @param pBufferPtr [OUT] The address of a buffer pointer
    NB: the UI is responsible for free'ing the buffer after use!
    @param len [OUT] The byte length of the URL data.
    @return Status of function:
       EOK - indicates success,
       < 0 - is a negated error code (from mrerrno.h/errno.h),
    The buffer pointer is set to NULL and the length to 0 if no data was available.
 */
int pod_appInfo_cnf( uint8 **pBufferPtr, unsigned *len);

/*! UI requests POD to deliver data related to passed URL
 
    The UI can request URL-related data at any time from the POD. Although
    the details of this transaction are vague, it probably originates from
    an ongoing MMI session when the viewer selects an on-screen URL. The
    data is requested by calling this function and passing a Universal
    Resource Locator (URL) as a null-terminated ASCII string. The UI is
    notified at some later time that the requested data is available when
    it receives a

        MSG_POD_SERVER_REPLY

    event. The UI can retrieve the requested data by calling pod_serverReply
    (see next function description).
    @pre pod_Open must have been called first
    @param url [IN] Null-terminated character string that likely was part
    of an open MMI session (although it may have been originated somewhere
    else) 
    @return Status of function:
       EOK - indicates success,
       < 0 - is a negated error code (from mrerrno.h/errno.h),
 */ 
int pod_serverQuery( pod_serverQuery_t * query );

/*! UI retrieves requested POD-delivered URL data
 
    After the UI calls pod_serverQuery() requesting URL-based data from
    the POD, the POD posts a

        MSG_POD_SERVER_REPLY

    event at some later time indicating that the requested data is available
    by calling this function.
    @pre pod_Open must have been called first
    @pre pod_serverQuery should have been called first
    @param pBufferPtr [OUT] The address of a buffer pointer that is set to
    point to the URL-related data returned by the POD. Typically the data is
    an HTML file that may contain one or more additional URLs.
    NB: the UI is responsible for free'ing the buffer after use!
    @param len [OUT] The byte length of the URL data.
    @return Status of function:
       EOK - indicates success,
       < 0 - is a negated error code (from mrerrno.h/errno.h),
    The buffer pointer is set to NULL and the length to 0 if no data was available.
 */ 
int pod_serverReply( uint8 ** pBufferPtr, unsigned * len );


/***********************************
 *   Man-Machine Interface (MMI)   *
 ***********************************/

/*! Retrieve parameters for opening an MMI

    The POD can request to open an MMI session with the UI/viewer (eg: a PPV
    dialogue) as long as the maximum number of MMI dialogues is not already
    open. The max number of simultaneous MMI diaglogues is contingent upon
    the number of windows supported by the UI (max of 1 MMI dialogue per
    window).

    The POD indicates its desire to open an MMI dialogue by asynchronously
    posting a

        MSG_POD_OPEN_MMI_SESSION

    event that the UI should subscribe to.

    The UI handles this message by calling pod_getMmiOpenParams to retrieve
    the session parameters as described in SCTE-28 2003 section 8.3.2.1.
    NOTE: this function is absolutely necessary to open an MMI session.

    @pre pod_Open must have been called first
    @pre MSG_POD_OPEN_MMI_SESSION must have been received first
    @param pBufferPtr [OUT] The address of a buffer pointer that is set to
    point to the session open parameters passed by the POD. Typically the data
    contains the display requested by the POD and a URL to be displayed by the UI.
    The buffer contains the entire POD APDU following the header (tag and length).
    NB: the UI is responsible for free'ing the buffer after use
    @param len [OUT] The address of a unsigned length that is filled in by
    the function.
    @return Status of function:
       EOK - indicates success,
       < 0 - is a negated error code (from mrerrno.h/errno.h),
    @version 02/12/04 - prototype
 */
int pod_getMmiOpenParams( uint8 ** pBufferPtr, unsigned * len );

/*! UI accepts/rejects POD's request to open new MMI session (i.e. Window)
 
    After the UI retrieves the session parameters that accompany an open
    session request (see pod_getMmiOpenParams above), it informs the POD
    whether the request is accepted or rejected. The UI must supply a
    unique, non-zero dialog number that is used by the POD to tag subsequent
    session-related activity in case there are mutiple simultaneous open
    sessions.
    
    When the POD decides to terminate the MMI session, it asynchronously posts a

        MSG_POD_CLOSE_MMI_SESSION

    event to the UI's message queue. Additional arguments that are part of the
    event include

        ud.ud1.userData1 - the 'dialogNumber' for the session to close

    The 'dialogNumber' can be reused on a subsequent MSG_POD_OPEN_MMI_SESSION,
    provided it is not in use at the time.
    Note: pod_serverQuery()/pod_serverReply() are used to transmit MMI
          session data between the UI and POD
    @pre pod_Open must have been called first
    @param dialogNumber [IN] The unique session ID received from the POD in
    the MSG_POD_OPEN_MMI_SESSION event. If the session request is accepted, the
    POD will use the dialogNumber to identify subsequent session activity.
    @param status [IN] Indication of whether the request to open a session is
    honored (0) or not (non-zero) based on the values given in [1] Table 8.3-F
    @return Status of function:
        EOK - indicates success. Unless EOK is returned, the session has not
              been opened.
    @version 10/31/03 - prototype
 */
int pod_openMmiSession( uint8 dialogNumber, Bool accept );

/*! UI requests or confirms close of an MMI session
 
    Invoking this function will notify the pod that the UI has closed an MMI
    dialog.
    
    @pre pod_Open must have been called first
    @param dlgNum [IN] Dialog number for the MMI session that was closed
    @return Status of function:
       EOK - indicates success,
       < 0 - is a negated error code (from mrerrno.h/errno.h),
 */
int pod_closeMmiSession_cnf(uint8 dialogNumber);

#if 0   // no longer used - see pod_serverQuery/pod_serverReply above
/*! Receive MMI session data from POD
 
    During an open MMI session, the POD can asynchronously inform the UI that
    there is session data it (POD) wants to send to the UI. This is indicated
    by the POD posting a

        MSG_POD_MMI_DATA

    event that the UI should subscribe to. Additional arguments that are part
    of the event include

        ud.ud1.userData1 - the 'dialogNumber' of the MMI session to
                           receive the data

    The UI handles this message by calling pod_getMmiData to gain access to
    the POD MMI data.
    NOTE: this function is an extension to the MMI application and may not
          be needed or implemented.
    @pre pod_Open must have been called first
    @pre The passed dialogNumber must reference a currently open MMI session
    @param dialogNumber [IN] Refers to an open MMI session
    @param pBufferPtr [OUT] The address of a buffer pointer that is set to
    point to the POD's data. Typically the data is an HTML file that may
    contain one or more URLs.
    NB: the UI is responsible for free'ing the buffer after use
    @param msWait [IN] Sync/Async flag as follows:
       ASYNC_REQ - asynchronous,
       SYNC_REQ - synchronous,
       > 0 - synchronous for msWait milliseconds, then asynchronous
    @return Status of function:
       EOK - indicates success,
       < 0 - is a negated error code (from mrerrno.h/errno.h),
       > 0 - is an asynchronous transaction sequence number
    The buffer pointer is not changed if the function returns anything other
    than success.
    @version 10/31/03 - prototype
 */
int pod_getMmiData( uint8 dialogNumber, uint8 ** pBufferPtr, int32 msWait ); 

/*! Send MMI session data to POD
 
    During an open MMI session, the UI can send data to the POD. This is done
    by calling pod_putMmiData.
    NOTE: this function is an extension to the MMI application and may not
          be needed or implemented.
    @pre pod_Open must have been called first
    @pre The passed dialogNumber must reference a currently open MMI session
    @param dialogNumber [IN] The MMI open session ID.
    @param pBuffer [IN] The address of a buffer that contains UI data to be
    sent to the POD. Typically, the buffer contains a URL that the viewer has
    selected in a POD-supplied HTML dialog. However, the buffer can contain
    any null-terminated ASCII data that the UI wants to send to the POD.
    @return Status of function (EOK indicates success)
    @version 10/31/03 - prototype
 */
int pod_putMmiData( uint8 dialogNumber, uint8 * pBuffer, int32 msWait ); 
#endif // 0


/*******************************
 *   Host Control and Homing   *
 *******************************/

/*! Asynchronous notification of POD In-band (IB) Tune
 
    At any time the POD can request to tune the in-band tuner. The POD Manager
    never honors the request unless the MR is in Standby mode. If MR is in
    Standby, the POD Manager may honor the tune request and inform the UI that
    the POD is in control of the in-band tuner. One of the reasons - possibly
    the only reason - POD tunes in-band is to update its firmware. Once the POD
    has been granted the tuner and signals that a firmware upgrade is in
    progress, the in-band tuner *cannot* be reliquished to the UI (or any other
    client). Since the firmware upgrade operation is uninterruptible, the UI
    is required to keep the MR in Standby until the upgrade is complete or it
    times-out.

    The POD posts the following asynchronous in-band tuner messages that the
    UI should register for:

        MSG_POD_IB_TUNER_START  POD is using in-band tuner; UI can still try
                                to tune but it may be prevented
        MSG_POD_IB_TUNER_BLOCK  POD is beginning firmware upgrade using in-
                                band tuner; UI is prevented from tuning until
                                it receives the POD_IB_TUNER_FREE message
        MSG_POD_IB_TUNER_FREE   POD relinquishes in-band tuner

    The events contain no other useful data. There is no UI reply to these
    messages. The POD requires the UI to honor in-band tuner blocking. When
    the POD stops using the tuner, it will be returned to its previous
    setting (i.e. major, minor channel).
 */


/*! UI informs POD that Media Receiver enters Standby mode
 
    Whenever the MR enters Standby mode (either through the viewer "powering
    off" the box or selecting a power off/standby option on any UI menu), the
    UI informs the POD by calling pod_enterStandby
    @pre pod_Open must have been called first
    @param none
    @return Status of function (EOK indicates success)
    @version 10/31/03 - prototype
 */
int pod_enterStandby( void );

/*! UI informs POD that Media Receiver exits Standby mode
 
    Whenever the MR exits Standby mode (either through the viewer "powering on"
    the box or a timed event that requires the box to not be in Standby) after
    having called pod_enterStandby, it does so by calling pod_exitStandby. The
    MR can always exit Standby mode *unless*

        MSG_POD_IB_TUNER_BLOCK

    is the most recently-seen IB_TUNER event. In this case, the POD is upgrading
    its firmware and cannot be interrupted. The UI is expected to remain in
    Standby until it receives one of the other IB_TUNER events

        MSG_POD_IB_TUNER_FREE
        MSG_POD_IB_TUNER_START
    
    at which time it can safely call pod_exitStandby. While the Media Receiver
    is "locked" in Standby, presumably, the UI informs the viewer that s/he must
    wait a moment until the current operation is completed. At this time, no upper
    bound on the amount of time required for POD firmware upgrade is known, but
    it could be as long as several minutes.
    The POD confirms the UI's intent to exit Standby by posting the

        MSG_POD_IB_TUNER_FREE

    event if the in-band tuner is not blocked. If the UI called pod_exitStandby
    when the tuner IS blocked, the POD rejects the exit from Standby by (re)posting
    the

        MSG_POD_IB_TUNER_BLOCK

    event. Thus, the UI should wait to exit Standby until it receives a subsequent
    MSG_POD_IB_TUNER_FREE event.
    @pre pod_Open must have been called first
    @pre pod_enterStandby should have been called previously
    @param none
    @return Status of function (EOK indicates success). Note: the function may
    return success even if the in-band tuner is blocked. In this case, the Media
    Receiver remains in Standby mode which is indicated by the POD re-posting the

        MSG_POD_IB_TUNER_BLOCK

    event.
    @version 10/31/03 - prototype
 */
int pod_exitStandby( void );


/*! Send Homing user notification message to Burbank
 
    When the pod requests a firmware upgrade and the system is not in standby, the
    request is declined and a user notification message is (optionally) displayed.
    The string to be displayed is sent with the firmware upgrade request.  Calling
    this function will retrieve the message (which is a char string).
    @pre pod_Open must have been called first
    @param pBufferPtr [OUT] char string
    @param len [OUT] length of string
    @return Status of function (EOK indicates success)
 */
int pod_getHomingUserNot( uint8 **pBufferPtr, unsigned *len);


/**************************
 *   Conditional Access   *
 **************************/

/*! Asynchronous notification of change in currently-tuned program's CA
 
    At any time the POD can inform the UI that the currently-tuned program's
    Conditional Access setting has changed. Every program that is "known" to
    the POD has a CA setting that is one of the following:
        - descrambling possible
        - descrambling possible via purchase dialogue
        - descrambling possible via technical dialogue
        - descrambling not possible due to lack of entitlement
        - descrambling not possible due to technical reasons
    Whenever the POD detects a change to the currently-tuned program's CA
    setting, it posts an event
    
        MSG_POD_CA_UPDATE
        
    that the UI subscribes to. When the UI receives this notification, it
    can retrieve the current CA info for the program by calling
    
        pod_getCaUpdate
        
    @pre pod_Open must have been called first
    @pre MSG_POD_CA_UPDATE event seen by UI
    @param pCaInfo [IN] The address of a buffer that contains UI data to be
    sent to the POD. Typically, the buffer contains a URL that the viewer has
    selected in a POD-supplied HTML dialog. However, the buffer can contain
    any null-terminated ASCII data that the UI wants to send to the POD.
    @return Status of function (EOK indicates success)
    @param none
    @return Status of function (EOK indicates success)
    @version  2/25/04 - prototype
*/
int pod_getCaUpdate( uint8 ** pCaInfo );


/************************
 *   Generic Features   *
 ************************/

#if 0   // not implemented
// features that can be exchanged between POD and Host (UI)
typedef enum
{
    eRfOutputChan   =  1,   // RF output channel
    ePcPin          =  2,   // parental control PIN
    ePcSettings     =  3,   // parental control settings
    eIppvPin        =  4,   // IPPV PIN
    eTimeZone       =  5,   // time zone
    eDST            =  6,   // Daylight Savings Time control
    eAcOutlet       =  7,   // AC outlet control
    eLanguage       =  8,   // primary language setting
    eRatingRegion   =  9,   // rating region (1=US, 2=Canada)
    eResetPin       = 10,   // reset to factory defaults PIN
    eCableUrls      = 11,   // ??
    eEAloc          = 12    // emergency alert location code
        /* values 13-63  are reserved for future use */
        /* values 64-255 are reserved for proprietary use */
} eFeatureList_t;

// feature parameters that can be exchanged between POD and Host (UI)
typedef struct
{
    uint8   rfChannel;      // RF output channel number (ignored if invalid)
    uint8   rfUiEnabled;    // 1-user can change RF channel, 2-cannot change
} rfOutputChan_t;

typedef struct
{
    uint8   pcPinLength;    // byte length of Parental Control PIN
    char    pcPin[255];     // non-null-terminated!
} pcPin_t;

typedef struct
{
    uint8   pcFactoryReset; // 0xA7 - perform factory reset
    uint16  pcChanCount;    // number of virtual channels under PC
    uint16  pcMajor[MAX_CHANNELS];
    uint16  pcMinor[MAX_CHANNELS];
} pcSettings_t;

typedef struct
{
    uint8   ippvPinLength;  // byte length of IPPV PIN
    char    ippvPin[255];   // non-null-terminated!
} ippvPin_t;

typedef struct
{
    int16   tzOffset;       // 2's comp offset in minutes (-720..720) from UTC
                            // nb: MIPs appears to be a 2's-complement cpu ??
} timezone_t;

typedef struct
{
    uint8   dst;            // 0-ignore, 1-no, 2-yes
} dst_t;

typedef struct
{
    uint8   acOutlet;       // 0-user setting, 1-switched, 2-unswitched
} acOutlet_t;

typedef struct
{
    uint32  iso639Language; // 3-char, non-null-term language code
                            // as specified by ISO 639-2
} language_t;

typedef struct
{
    uint8   ratingRegion;   // 0-forbidden, 1-US+possessions, 2-Canada
} rating_t;

typedef struct
{
    uint8   resetPin;       // 0-do not reset, 1-reset PC PIN,
                            // 2-reset purchase PIN, 3-reset PC & purchase PIN
} resetPin_t;

#define MAX_URLS    ( 4 )   // 1 per type ??
typedef struct
{
    uint8   urlCount;       // number of url entries that follow (<= MAX_URLS)
    uint8   urlType[MAX_URLS];      // 0-undefined, 1-web portal, 2-EPG, 3-VOD
    uint8   urlLength[MAX_URLS];    // byte length of URL
    char    url[MAX_URLS][255];     // not null-terminated
} cableUrl_t;

typedef struct
{               // refer to SCTE 18 2002 - Emergency Alert Message for Cable
    uint8   state;          // (1-99) See ATSC A/65b Annex H
    uint8   county_sub;     // area within county (1-16). See SCTE 18 2002 Tbl 5
    uint8   county;         // within state (0-999). See ATSC A/65b Annex H
} EAloc_t;

typedef struct
{
    rfOutputChan_t  rf;
    pcPin_t         pcPin;
    pcSettings_t    pcSettings;
    ippvPin_t       ippvPin;
    timezone_t      tz;
    dst_t           dst;
    acOutlet_t      ac;
    language_t      lang;
    rating_t        region;
    resetPin_t      resetPin;
    cableUrl_t      urls;
    EAloc_t         loc;
} featureParams_t;


/*! UI responds to POD's request for UI's features list
 
    The POD provider (a cable MSO) can preset certain viewer attributes
    (referred to as a "features list") within the POD. Normally, the attributes
    would be settable by the viewer via the UI. The features list can be
    requested by either the POD or the UI at any time. When the POD wants the
    UI to send it the generic features that the MR supports, it asynchronously
    posts a

        MSG_POD_FEATURE_LIST_REQUEST

    message. In response, the UI calls pod_featuresList passing a pointer to
    a bit mask (fList) whose values indicate the UI's features list. The bit
    positions in the mask are given by the eFeaturesList_t enum. For example,
    bit 0 of the mask is unused, bit 1 is the RF output channel feature,
    bit 2 is Parental Control PIN and so on. If the bit is set, the feature
    is supported by the UI otherwise it isn't.
    @pre pod_Open must have been called first
    @param fList [IN] Pointer to a bit mask whose bits indicate support for
    the generic features contained in the eFeaturesList_t enum defined above.
    @return Status of function (EOK indicates success)
    @version 10/31/03 - prototype
 */
int pod_featuresList( uint32 * fList );

/*! POD responds to UI's request for POD's features list
 
    When the UI wants the POD to send it the list of generic features that the
    POD has preset values for, it calls pod_featuresListRequest. If the transaction
    is asynchronous, the POD notifies the UI that the data is ready by posting the

        MSG_POD_FEATURE_LIST

    event. The UI can re-call this function to gain access to the POD's features
    list data.
    @pre pod_Open must have been called first
    @param fListPtr [OUT] The address of a buffer pointer that is set to point
    to the POD's features list. The bit positions in the mask are given by the
    eFeaturesList_t enum. For example, bit 0 of the mask is unused, bit 1 is
    the RF output channel feature, bit 2 is Parental Control PIN and so on. If
    the bit is set, the feature is supported by the POD otherwise it isn't.
    Unless the function returns success, the buffer pointer is not changed.
    NB: the UI is responsible for free'ing this buffer after use
    @param msWait [IN] Sync/Async flag as follows:
       ASYNC_REQ - asynchronous,
       SYNC_REQ - synchronous,
       > 0 - synchronous for msWait milliseconds, then asynchronous
    @return Status of function:
       EOK - indicates success,
       < 0 - is a negated error code (from mrerrno.h/errno.h),
       > 0 - is an asynchronous transaction sequence number
    The buffer pointer is not changed if the function returns anything other
    than success.
    @version 10/31/03 - prototype
 */
int pod_featuresListReq( uint32 ** fListPtr, int32 msWait );

/*! UI informs the POD that supported generic features list has changed
 
    When the POD wants to inform the UI that its (POD's) list of preset generic
    features has changed, it asynchronously posts a

        MSG_POD_FEATURE_LIST_CHANGE

    message. The UI can call pod_featuresListReq to determine the new features
    list.
    Although it probably won't happen, if the UI's features list changes,
    it calls pod_featuresListChanged to inform the POD that it might want to
    request the current features list from the UI (by posting the
    MSG_POD_FEATURE_LIST_REQUEST message).
    @pre pod_Open must have been called first
    @param none 
    @return Status of function (EOK indicates success)
    @version 10/31/03 - prototype
 */
int pod_featuresListChanged( void );

/*! POD requests/UI responds with feature parameter values
 
    Anytime after the exchange of features lists, the POD can request the UI
    to send it the values that its generic features have at the current time.
    The POD does this by posting a

        MSG_POD_FEATURE_PARAMS_REQUEST

    message asynchronously to the UI's message queue. In response, the UI calls
    pod_featureParameters passing a pointer to a structure containing all the
    possible generic features and their current values. Features that are not
    supported by the UI should be cleared (set to 0).
    @pre pod_Open must have been called first
    @param fParams [IN] Pointer to a buffer containing all possible features
    and their current values. Features that are not supported by the UI should
    have their values set to 0.
    @return Status of function (EOK indicates success)
    @version 10/31/03 - prototype
 */
int pod_putFeatureParameters( featureParams_t * fParams );

/*! POD responds with feature parameter values
 
    Anytime after the exchange of features lists, the POD can send the UI
    the values that its generic features have at the current time. This cannot
    be requested by the UI and is typically done under head-end control. The
    POD notifies the UI that it has parameter values to send by asynchronously
    posting the

        MSG_POD_FEATURE_PARAMS

    message. The UI calls pod_getFeatureParameters to gain access to the POD's
    feature parameters' values.
    @param fParamsPtr [OUT] The address of a featureParams_t buffer that is set
    to point to the POD's feature parameters' values. Unless the function
    returns success, the buffer pointer is not changed.
    NB: the UI is responsible for free'ing this buffer after use
    @param msWait [IN] Sync/Async flag as follows:
       ASYNC_REQ - asynchronous,
       SYNC_REQ - synchronous,
       > 0 - synchronous for msWait milliseconds, then asynchronous
    @return Status of function:
       EOK - indicates success,
       < 0 - is a negated error code (from mrerrno.h/errno.h),
       > 0 - is an asynchronous transaction sequence number
    The buffer pointer is not changed if the function returns anything other
    than success.
    @version 12/01/03 - prototype
 */
int pod_getFeatureParameters( featureParams_t ** fParamsPtr, int32 msWait );
#endif // 0

#ifdef __cplusplus
}   // end of extern "C"
#endif

#endif // _PODAPI_H_

