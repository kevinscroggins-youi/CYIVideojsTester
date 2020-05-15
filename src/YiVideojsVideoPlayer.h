// © You i Labs Inc. 2000-2019. All rights reserved.

#ifndef _YI_VIDEOJS_VIDEO_PLAYER_H_
#define _YI_VIDEOJS_VIDEO_PLAYER_H_

/*!
 \addtogroup video-player
 @{
 */

#include <player/YiAbstractVideoPlayer.h>

#include <utility/YiRapidJSONUtility.h>

class CYIVideojsVideoPlayerPriv;

/*!
    \brief An abstract video player implementation that provides an interface to an underlying Video.js JavaScript video player
    running in an embedded web view on the target platform.

    \note Only Tizen NaCl is currently supported.
*/
class CYIVideojsVideoPlayer : public CYIAbstractVideoPlayer
{
    friend class CYIVideojsVideoPlayerPriv;

public:
    /*!
        \details Constructs an instance of the CYIVideojsVideoPlayer.

        This creation method requires you to specify the JavaScript player wrapper \a className as an argument
        along with any other custom \a playerConfiguration settings such as any API keys that might be required
        for non-free MSE web players.
    */
    static CYIVideojsVideoPlayer *Create(const std::map<CYIString, CYIString> &playerConfiguration = std::map<CYIString, CYIString>());

    /*!
        \details Constructs an instance of the CYIVideojsVideoPlayer, alternatively using a RapidJSON player configuration object.
    */
    static CYIVideojsVideoPlayer *Create(yi::rapidjson::Document &&playerConfiguration);

    virtual ~CYIVideojsVideoPlayer();

    /*!
        \details Returns the nickname assigned to the current player instance, if any.
    */
    virtual CYIString GetNickname() const;

    /*!
        \details Allows \a nickname to be set on the current player instance to make it easier to identify
        in the log output when debugging.
    */
    virtual void SetNickname(const CYIString &nickname) const;

    /*!
        \details Allows an external text track to be added to the player from a \a url. The \a format is expected
        to be a valid mime type that is supported by the underlying JavaScript Video.js player.

        \note The \a label argument is usually optional, but should be specified to assist so the
        text track is easier to identify to the end user.

        \note If no \a type is provided, it will usually default to a value of \"caption\".
    */
    virtual void AddExternalTextTrack(const CYIString &url, const CYIString &language, const CYIString &label, const CYIString &type, const CYIString &format, bool enable = false);

private:
    CYIVideojsVideoPlayer() = default;
    virtual void Init_() override;
    virtual CYIString GetName_() const override;
    virtual CYIString GetVersion_() const override;
    virtual CYIAbstractVideoPlayer::Statistics GetStatistics_() const override;
    virtual std::unique_ptr<CYIVideoSurface> CreateSurface_() override;
    virtual bool SupportsFormat_(StreamingFormat streamingFormat, CYIAbstractVideoPlayer::DRMScheme drmScheme) const override;
    virtual bool HasNativeStartTimeHandling_() const override;
    virtual bool HasNativeBitrateEventHandling_() const override;
    virtual void Prepare_(const CYIUrl &videoURI, StreamingFormat format) override;
    virtual void Play_() override;
    virtual void Pause_() override;
    virtual void Stop_() override;
    virtual uint64_t GetDurationMs_() const override;
    virtual uint64_t GetCurrentTimeMs_() const override;
    virtual std::vector<CYIAbstractVideoPlayer::SeekableRange> GetLiveSeekableRanges_() const override;
    virtual void Seek_(uint64_t seekPositionMS) override;
    virtual bool SelectAudioTrack_(uint32_t id) override;
    virtual std::vector<AudioTrackInfo> GetAudioTracks_() const override;
    virtual AudioTrackInfo GetActiveAudioTrack_() const override;
    virtual bool IsMuted_() const override;
    virtual void Mute_(bool mute) override;
    virtual void DisableClosedCaptions_() override;
    virtual bool SelectClosedCaptionsTrack_(uint32_t id) override;
    virtual std::vector<ClosedCaptionsTrackInfo> GetClosedCaptionsTracks_() const override;
    virtual ClosedCaptionsTrackInfo GetActiveClosedCaptionsTrack_() const override;
    virtual void SetMaxBitrate_(uint64_t maxBitrate) override;
    virtual CYIAbstractVideoPlayer::TimedMetadataInterface *GetTimedMetadataInterface_() const override;

    CYIVideojsVideoPlayerPriv *m_pPriv;

    YI_TYPE_BASES(CYIVideojsVideoPlayer, CYIAbstractVideoPlayer);
};

/*!
 @}
 */

#endif // _YI_VIDEOJS_VIDEO_PLAYER_H_
