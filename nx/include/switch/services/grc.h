/**
 * @file grc.h
 * @brief GRC Game Recording (grc:*) service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/sm.h"
#include "../services/caps.h"
#include "../kernel/event.h"
#include "../kernel/tmem.h"
#include "../display/native_window.h"

/// Stream type values for \ref grcdRead.
typedef enum {
    GrcStream_Video = 0,          ///< Video stream with H.264 NAL units. Official sw uses buffer size 0x32000.
    GrcStream_Audio = 1,          ///< Audio stream with PcmFormat_Int16, 2 channels, and samplerate = 48000Hz. Official sw uses buffer size 0x1000.
} GrcStream;

/// GameMovieTrimmer
typedef struct {
    Service s;              ///< IGameMovieTrimmer
    TransferMemory tmem;    ///< TransferMemory
} GrcGameMovieTrimmer;

/// IMovieMaker
typedef struct {
    Service a;              ///< applet IMovieMaker
    Service s;              ///< grc IMovieMaker
    Service video_proxy;    ///< IHOSBinderDriver VideoProxy
    Event recording_event;  ///< Output Event from GetOffscreenLayerRecordingFinishReadyEvent with autoclear=false.
    Event audio_event;      ///< Output Event from GetOffscreenLayerAudioEncodeReadyEvent with autoclear=false.
    TransferMemory tmem;    ///< TransferMemory
    NWindow win;            ///< \ref NWindow
    u64 layer_handle;       ///< LayerHandle
    bool layer_open;        ///< Whether OpenOffscreenLayer was used successfully, indicating that CloseOffscreenLayer should be used during \ref grcMovieMakerClose.
    bool started_flag;      ///< Whether \ref grcMovieMakerStart was used successfully. This is also used by \ref grcMovieMakerAbort.
} GrcMovieMaker;

/// GameMovieId
typedef struct {
    CapsAlbumEntryId album_id;    ///< \ref CapsAlbumEntryId
    u8 reserved[0x28];            ///< Unused, always zero.
} GrcGameMovieId;

/// OffscreenRecordingParameter
typedef struct {
    u8 unk_x0[0x10];              ///< Unknown. Default value is 0.
    u32 unk_x10;                  ///< Unknown. Default value is 0x103.

    s32 video_bitrate;            ///< VideoBitRate. Default value is 8000000.
    s32 video_width;              ///< VideoWidth. Default value is 1280.
    s32 video_height;             ///< VideoHeight. Default value is 720.
    s32 video_framerate;          ///< VideoFrameRate. Default value is 30.
    s32 video_keyFrameInterval;   ///< VideoKeyFrameInterval. Default value is 30.

    s32 audio_bitrate;            ///< AudioBitRate. Default value is 128000 ([5.0.0-5.1.0] 1536000).
    s32 audio_samplerate;         ///< AudioSampleRate. Default value is 48000.
    s32 audio_channel_count;      ///< AudioChannelCount. Default value is 2.
    s32 audio_sample_format;      ///< \ref PcmFormat AudioSampleFormat. Default value is PcmFormat_Int16.

    s32 video_imageOrientation;   ///< \ref AlbumImageOrientation VideoImageOrientation. Default value is ::AlbumImageOrientation_Unknown0.

    u8 unk_x3c[0x44];             ///< Unknown. Default value is 0.
} GrcOffscreenRecordingParameter;

/// Default size for \ref grcCreateMovieMaker, this is the size used by official sw.
#define GRC_MOVIEMAKER_WORKMEMORY_SIZE_DEFAULT 0x6000000

///@name Trimming
///@{

/**
 * @brief Creates a \ref GrcGameMovieTrimmer using \ref appletCreateGameMovieTrimmer, uses the cmds from it to trim the specified video, then closes it.
 * @note See \ref appletCreateGameMovieTrimmer for the requirements for using this.
 * @note This will block until video trimming finishes.
 * @param[out] dst_movieid \ref GrcGameMovieId for the output video.
 * @param[in] src_movieid \ref GrcGameMovieId for the input video.
 * @param[in] tmem_size TransferMemory size. Official sw uses size 0x2000000.
 * @param[in] thumbnail Optional, can be NULL. RGBA8 1280x720 thumbnail image data.
 * @param[in] start Start timestamp in 0.5s units.
 * @param[in] end End timestamp in 0.5s units.
 */
Result grcTrimGameMovie(GrcGameMovieId *dst_movieid, const GrcGameMovieId *src_movieid, size_t tmem_size, const void* thumbnail, s32 start, s32 end);

///@}

///@name IMovieMaker
///@{

/**
 * @brief Creates a \ref GrcOffscreenRecordingParameter with the default values, see \ref GrcOffscreenRecordingParameter for the default values.
 * @param[out] param \ref GrcOffscreenRecordingParameter
 */
void grcCreateOffscreenRecordingParameter(GrcOffscreenRecordingParameter *param);

/**
 * @brief Creates a \ref GrcMovieMaker using \ref appletCreateMovieMaker, and does the required initialization.
 * @note See \ref appletCreateMovieMaker for the requirements for using this.
 * @param[out] m \ref GrcMovieMaker
 * @param[in] size TransferMemory WorkMemory size. See \ref GRC_MOVIEMAKER_WORKMEMORY_SIZE_DEFAULT.
 */
Result grcCreateMovieMaker(GrcMovieMaker *m, size_t size);

/**
 * @brief Closes a \ref GrcMovieMaker.
 * @note This also uses \ref grcMovieMakerAbort.
 * @param m \ref GrcMovieMaker
 */
void grcMovieMakerClose(GrcMovieMaker *m);

/**
 * @brief Gets the \ref NWindow for the specified MovieMaker.
 * @param m \ref GrcMovieMaker
 */
static inline NWindow* grcMovieMakerGetNWindow(GrcMovieMaker *m) {
    return &m->win;
}

/**
 * @brief Starts recording with the specified MovieMaker and \ref GrcOffscreenRecordingParameter.
 * @param m \ref GrcMovieMaker
 * @param[in] param \ref GrcOffscreenRecordingParameter
 */
Result grcMovieMakerStart(GrcMovieMaker *m, const GrcOffscreenRecordingParameter *param);

/**
 * @brief Aborts recording with the specified MovieMaker.
 * @note This is used automatically by \ref grcMovieMakerClose.
 * @note This will throw an error if \ref grcMovieMakerStart was not used previously, with the flag used for this being cleared afterwards on success.
 * @param m \ref GrcMovieMaker
 */
Result grcMovieMakerAbort(GrcMovieMaker *m);

/**
 * @brief Finishes recording with the specified MovieMaker.
 * @note This automatically uses \ref grcMovieMakerAbort on error.
 * @note The recorded video will not be accessible via the Album-applet since it's stored separately from other Album data.
 * @param m \ref GrcMovieMaker
 * @param width Width for the thumbnail, must be 1280.
 * @param height Height for the thumbnail, must be 720.
 * @param[in] appdata UserData input buffer for the JPEG thumbnail. Optional, can be NULL.
 * @param[in] appdata_size Size of the UserData input buffer. Optional, can be 0. Must be <=0x400.
 * @param[in] thumbnail RGBA8 image buffer containing the thumbnail. Optional, can be NULL.
 * @param[in] thumbnail_size Size of the thumbnail buffer. Optional, can be 0.
 * @param[out] entry Output \ref AlbumFileEntry for the recorded video. Optional, can be NULL. Only available on [7.0.0+], if this is not NULL on pre-7.0.0 an error is thrown.
 */
Result grcMovieMakerFinish(GrcMovieMaker *m, s32 width, s32 height, const void* appdata, size_t appdata_size, const void* thumbnail, size_t thumbnail_size, AlbumFileEntry *entry);

/**
 * @brief Gets the recording error with the specified MovieMaker.
 * @param m \ref GrcMovieMaker
 */
Result grcMovieMakerGetError(GrcMovieMaker *m);

/**
 * @brief Encodes audio sample data with the specified MovieMaker.
 * @note This waits on the event and uses the cmd repeatedly until the entire input buffer is handled.
 * @note If you don't use this the recorded video will be missing audio.
 * @param m \ref GrcMovieMaker
 * @param[in] buffer Audio buffer.
 * @param[in] size Size of the buffer.
 */
Result grcMovieMakerEncodeAudioSample(GrcMovieMaker *m, const void* buffer, size_t size);

///@}

///@name grc:d
///@{

/// Initialize grc:d.
Result grcdInitialize(void);

/// Exit grc:d.
void grcdExit(void);

/// Gets the Service for grc:d.
Service* grcdGetServiceSession(void);

/// Begins streaming. This must not be called more than once, even from a different service session: otherwise the sysmodule will assert.
Result grcdBegin(void);

/**
 * @brief Reads a stream, from the video recording being done of the currently running game title.
 * @note This will block until data is available. This will hang if there is no game title running which has video capture enabled.
 * @param[in] stream \ref GrcStream
 * @param[out] buffer Output buffer.
 * @param[in] size Max size of the output buffer.
 * @param[out] unk Unknown.
 * @param[out] data_size Actual output data size.
 * @param[out] timestamp Timestamp?
 */
Result grcdRead(GrcStream stream, void* buffer, size_t size, u32 *unk, u32 *data_size, u64 *timestamp);

///@}
