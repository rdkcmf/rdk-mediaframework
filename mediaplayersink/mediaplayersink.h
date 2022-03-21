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
#ifndef MEDIA_PLAYER_SINK_H
#define MEDIA_PLAYER_SINK_H

/**
 * @file mediaplayersink.h
 *
 * @defgroup HAL_MediaPlayer_Sink MediaPlayer Sink
 * MediaPlayerSink is a Sink element of RMF and it uses “playersinkbin” gstreamer plugin.
 *
 * Following are the list of features provided by MediaPlayerSink
 * - Create custom bin template with generic properties.
 * - SoC specific "playersinkbin" gstreamer element is used by generic mediaplayersink to play the AV.
 * - Added support for different video/audio types.
 * - Added support for selection of multiple audio streams. This includes dependency on SoC demux plugin
 * for providing readable pmt_info. This is necessary to get available languages and associated pids in the stream.
 * Inherit Intel's mediaplayersink and implement custom bin, change mediaplayersink to use the custom bin.
 * Provides test platform for independently test the functionalities using RMF mediaplayersink.
 *
 * Following are the generic property list for MediaPlayerSink which can be used by the application.
 * - Program number(set)
 * - Available languages(get)
 * - Preferred language(set)
 * - Video Decoder Handle(get)
 * - Stop Keep Frame(set)
 * - Video Rectangle(set)
 * - Plane(set)
 * - Play Speed(set)
 * - Current_position(get)
 * - resolution(set/get)
 * @ingroup  RMF
 * @defgroup HAL_MediaPlayer_Sink_Interface MediaPlayer Sink API
 * @ingroup HAL_MediaPlayer_Sink
 * @defgroup HAL_MediaPlayer_Sink_Class MediaPlayer Sink Class
 * @ingroup HAL_MediaPlayer_Sink
 */

#include "rmfbase.h"
#include <string>

#define MEDIAPLAYERSINK_SHOWLASTFRAME_SUPPORTED 1
#define MEDIAPLAYERSINK_VIDEO_RECTANGLE_MOVE_SUPPORTED 1
#define MEDIAPLAYERSINK_GET_AUDIOTRACKS_SUPPORTED 1
#define MEDIAPLAYERSINK_GET_AUDIOTRACK_SELECTED_SUPPORTED 1
#define MEDIAPLAYERSINK_PMT_CALLBACK_SUPPORTED 1
#define MEDIAPLAYERSINK_DVBSUB_AND_TTXT_SUPPORTED 1
#define MEDIAPLAYERSINK_PREFERRED_AUDIO_TRACK_SUPPORTED 1

typedef enum _rmf_current_asset_type {
    RMF_CURRENTLY_PLAYING_AV_ASSET = 0,
    RMF_CURRENTLY_PLAYING_MC_ASSET,
    RMF_CURRENTLY_PLAYING_FM_ASSET
} RMFCurrentAssetType;
#define MEDIAPLAYERSINK_VIDEO_MUTE_SUPPORTED   1
#define MEDIAPLAYERSINK_AUDIO_MUTE_SUPPORTED   1

/**
 * @brief video player sink.
 * @ingroup HAL_MediaPlayer_Sink_Class 
 */
class MediaPlayerSink: virtual public RMFMediaSinkBase
{
public:

/** 
 * @addtogroup HAL_MediaPlayer_Sink_Interface
 * @{
 */
/**
 * @brief This function is used to set the properties of a rectangle to player sink bin.
 *
 * @param[in]  x          X coordinate of the rectangle.
 * @param[in]  y          Y coordinate of the rectangle.
 * @param[in]  w          Width of the rectangle.
 * @param[in]  h          Height of the rectangle
 * @param[in] apply_now   Boolean value indicates value to be set or not.
 *
 * @return Returns the status of the operation.
 * @retval Returns RMF_RESULT_SUCCESS if successful, appropiate error code otherwise.
 */
    RMFResult setVideoRectangle(unsigned x, unsigned y, unsigned w, unsigned h, bool apply_now = false);
    RMFResult setVideoRectangle(unsigned x, unsigned y, unsigned w, unsigned h, unsigned steps, unsigned max_w, unsigned max_h, bool blocking, bool apply_now = false);

    // from RMFMediaSinkBase

/**
 * @brief This function changes the speed of the video.
 *
 * This API sets the "play-speed" property of the playersinkbin to the "new_speed".
 *
 * @param[in]  new_speed  New speed to set.
 */
    /*virtual*/ void onSpeedChange(float new_speed);

/**
 * @brief This API creates a new playersink bin element.
 *
 * Makes use of "gst_element_factory_make" to create the playersink bin element and adds the
 * element to the bin.
 * Add the audio and video event handler to the bin to signal the events.
 * Add a ghostpad to the bin  and connect it to the sink.
 *
 * @return Returns the playersink bin element
 */
    /*virtual*/ void* createElement();

/**
 * @brief This API is used to terminate the playersink module instance.
 */
    /*virtual*/ RMFResult close();
/**
 * @brief This API will fetch the current time position in the total media length.
 *
 * @param[out] time Returns the current media time in double.
 *
 * @return Returns the status of the operation.
 * @retval Returns RMF_RESULT_SUCCESS if successful, appropiate error code otherwise.
 */
    /*virtual*/ RMFResult getMediaTime(double& t);

/**  Callback Function */
    typedef void (*callback_t)(void* data);

/**
 * @brief This API sets the audio to mute.
 *
 * @param[in]  muted  Boolean value to enable/disable mute.
 */
    void setMuted(bool muted);

/**
 * @brief This function is used to get the audio mute status.
 *
 * @return Returns the audio status - muted/unmuted.
 * @retval Returns true on success, false otherwise.
 */
    bool getMuted();

/**
 * @brief This API is used to set the volume to the media player.
 *
 * @param[in]  volume  Volume to be set.
 */
    void setVolume(float volume);

/**
 * @brief This API returns the volume from the player.
 *
 * @return Returns the volume in floats.
 */
    float getVolume();

/**
 * @brief This function returns the video decoder handle.
 *
 * Uses "video-decode-handle" property of playersink bin to retrieve the handle information.
 *
 * @return Returns the handle information.
 */
    unsigned long getVideoDecoderHandle() const;

/**
 * @brief This function returns the language supported by the device.
 *
 * Uses "available-languages" property to return the status.
 *
 * @return Returns the audio language supported.
 */
    const char *getAudioLanguages();

/**
 * @brief This API returns the closed caption descriptor from the device.
 *
 * Uses "cc-descriptor" property of the playersink bin to retrieve the value.
 *
 * @return Returns the CC descriptor.
 */
    const char *getCCDescriptor();

/**
 * @brief This API returns the EISS information available in the stream.
 *
 * EISS(ETV Integrated Signaling Stream) an MPEG elementary stream which carries media timeline messages,
 * stream events and EISS tables for an ETV application.
 *
 * @return Returns EISS information.
 */
    const char *getEISSData();

/**
 * @brief This API is to set the audio language preference.
 *
 * @param[in]  pAudioLang    Language to be set.
 */
    void setAudioLanguage (const char* pAudioLang);

/**
 * @brief This API is to set the auxiliary audio language preference.
 *
 * @param[in]  pAudioLang    Language to be set.
 */
    void setAuxiliaryAudioLang (const char* pAudioLang);

/**
 * @brief This API is to set the audio track preference.
 *
 * @param[in]  pAudioTrackPreference Audio Track preference to be set.
 *
 * format: list_of_languages;list_of_audio types;list_of_codecs
 *         with every list can contain descriptors separated by comma
 *
 * for example:
 * "eng,fra,deu;da;ac3,aac,mp1"
 *     - configures preferred language list as: eng,fra,deu
 *     - if possible audio description track should be selected: da
 *     - the preferred codec list is: ac3,aac,mp1
 *
 * If preferred audio type should be normal audio type track, then use :
 * "eng,fra,deu;;ac3,aac,mp1"
 *
 */
    void setPreferredAudioTrack(const char* pAudioTrackPreference);

/**
 * @brief This function is zoom in/out the video.
 *
 * @param[in]  zoomVal The value to set.
 */
    void setVideoZoom (unsigned short zoomVal);

/**
 * @brief This function is used to set the EISS(ETV Integrated Signaling Stream) filter status.
 *
 * @param[in]  eissStatus    Boolean value to enable/disable the filter.
 */
    void setEissFilterStatus (bool eissStatus);

/**
 * @brief This API adds the EISS(ETV Integrated Signaling Stream) filter function.
 *
 * @param[in]  maxNumOfDiplays       Number of displays connected to AV outport.
 * @param[out] ptAVOutInterfaceInfo  Address where AV out information is stored.
 *
 * @return Returns the status of the operation.
 * @retval Returns 1 if successful, appropiate error code otherwise.
 */
    void addInbandFilterFunc(unsigned int funcPointer,unsigned int cbData);

/**
 * @brief This API sets the network buffer size.
 *
 * @param[in]  bufferSize Network buffer size.
 */
    void setNetWorkBufferSize (short bufferSize);

/**
 * @brief This API registers the video callback function.
 *
 * @param[in]  cb    callback function.
 * @param[in]  data  Video callback data.
 */
    void setHaveVideoCallback(callback_t cb, void* data);

/**
 * @brief This API registers the audio callback function.
 *
 * @param[in]  cb    callback function.
 * @param[in]  data  Audio callback data.
 */
    void setHaveAudioCallback(callback_t cb, void* data);
/**
 * @brief This API registers the EISS data callback function.
 *
 * @param[in]  cb    callback function.
 * @param[in]  data  EISS callback data.
 */
    void setEISSDataCallback(callback_t cb, void* data);
/**
 * @brief This API registers the callback function for controlling volume.
 *
 * @param[in]  cb    callback function.
 * @param[in]  data  Volume change callback data.
 */
    void setVolumeChangedCallback(callback_t cb, void* data);
/**
 * @brief This API registers the mute change callback function.
 *
 * @param[in]  cb    callback function.
 * @param[in]  data  Data for set/unset mute.
 */
    void setMuteChangedCallback(callback_t cb, void* data);
/**
 * @brief This API registers the video playback callback function.
 *
 * @param[in]  cb    callback function.
 * @param[in]  data  Data to control video playback.
 */
    void setVideoPlayingCallback (callback_t cb, void* data);
/**
 * @brief This API registers the audio playback callback function.
 *
 * @param[in]  cb    callback function.
 * @param[in]  data  Data to control audio playback.
 */
    void setAudioPlayingCallback (callback_t cb, void* data);
/**
 * @brief This API registers the media warning callback function.
 *
 * @param[in]  cb    callback function.
 * @param[in]  data  Data to set/unset media warning.
 */
    void setMediaWarningCallback (callback_t cb, void* data);

/**
 * @brief This API returns the media warning information as string.
 *
 * @return Returns the media warning information.
 */
    const char* getMediaWarningString(void);

/**
 * @brief This API registers the PMT update callback function.
 *
 * @param[in]  cb    callback function.
 * @param[in]  data  Data for PMT callback.
 */
    void setPmtUpdateCallback (callback_t cb, void* data);

/**
 * @brief This function returns the media buffer size.
 *
 * @return Returns the buffer size.
 */
    short getMediaBufferSize(void);
    void showLastFrame(bool enable);
    void setPrimeDecode(bool enable);
    void setPipOutput(bool pip);
    bool getPipOutput() const;
    void setZOrder(int order);
    int getZOrder() const;
    void setVideoMute(bool muted);
    void setAudioMute(bool muted);
    bool getVideoMute(void);

/**
 * @brief This function returns the language tracks.
 *
 * Uses "audio-tracks" property to return the status.
 *
 * @return Returns the audio tracks supported.
 */
    const char *getAudioTracks();

/**
 * @brief This function returns the currently selected audio track.
 *
 * Uses "audio-track-selected" property to return the status.
 *
 * @return Returns the currently selected audio tracks.
 */
    const char *getAudioTrackSelected(void);

/**
 * @brief This function returns the subtitle tracks.
 *
 * Uses "subs-tracks" property to return the status.
 *
 * @return Returns the subtile tracks.
 */
    void getSubtitles (char* value, int max_size);

/**
 * @brief This function returns the teletext tracks.
 *
 * Uses "ttxt-tracks" property to return the status.
 *
 * @return Returns the teletext tracks.
 */
    void getTeletext (char* value, int max_size);

/**
 * @brief This function set the current subtec path and selected subtitle track.
 *
 * Uses "subtecsink-path" property to set the subtec path.
 * Uses "subs-track-selected" property to select the subtitle.
 *
 * @return Sets the current subtec path and selected subtitle tracks.
 */
    void setSubtitlesTrack (const char* channel_path, const char* value);

/**
 * @brief This function set the current subtec path and selected teletext track.
 *
 * Uses "subtecsink-path" property to set the subtec path.
 * Uses "ttxt-track-selected" property to select the teletext track.
 *
 * @return Sets the current subtec path and selected teletext tracks.
 */
    void setTeletextTrack (const char* channel_path, const char* value);

/**
 * @brief This function returns the currently selected subtitle track.
 *
 * Uses "subs-track-selected" property to return the status.
 *
 * @return Returns the currently selected subtitle tracks.
 */
    void getSubtitlesTrack (char* value, int max_size);

/**
 * @brief This function returns the currently selected teletext track.
 *
 * Uses "ttxt-track-selected" property to return the status.
 *
 * @return Returns the currently selected teletext tracks.
 */
    void getTeletextTrack (char* value, int max_size);

/**
 * @brief This function resets the subtec system.
 */
    void resetTtxtSubs(void);

/**
 * @brief This function returns the channel type that is playing now.
 *
 * @return Returns the channel type.
 */
    RMFCurrentAssetType getCurrentAssetType(void);
/** @} */

protected:
    // from RMFMediaSinkBase
    /*virtual*/ RMFMediaSinkPrivate* createPrivateSinkImpl();
};

/** @} */

#endif // MEDIA_PLAYER_SINK_H
