#ifndef _YI_VIDEOJS_VIDEO_PLAYER_PRIV_H_
#define _YI_VIDEOJS_VIDEO_PLAYER_PRIV_H_

#include "player/YiVideojsVideoPlayer.h"
#include "player/YiVideojsVideoSurface.h"

#include <platform/YiWebMessagingBridge.h>
#include <utility/YiRapidJSONUtility.h>

class CYIVideojsVideoPlayer;

class CYIVideojsVideoPlayerPriv : public CYISignalHandler,
                                  public CYIAbstractVideoPlayer::TimedMetadataInterface
{
public:
    CYIVideojsVideoPlayerPriv(CYIVideojsVideoPlayer *pPub);
    CYIVideojsVideoPlayerPriv(CYIVideojsVideoPlayer *pPub, yi::rapidjson::Document &&playerConfiguration);
    virtual ~CYIVideojsVideoPlayerPriv();

    void SetVideoRectangle(const YI_RECT_REL &videoRectangle);
    void Init();
    CYIString GetName() const;
    CYIString GetNickname() const;
    CYIString GetVersion() const;
    CYIAbstractVideoPlayer::Statistics GetStatistics() const;
    void SetNickname(const CYIString &nickname) const;
    std::unique_ptr<CYIVideoSurface> CreateSurface();
    bool SupportsFormat(CYIAbstractVideoPlayer::StreamingFormat format, CYIAbstractVideoPlayer::DRMScheme drmScheme = CYIAbstractVideoPlayer::DRMScheme::None) const;
    void Prepare(const CYIUrl &videoURI, CYIAbstractVideoPlayer::StreamingFormat format);
    void Play();
    void Pause();
    void Stop();
    uint64_t GetDurationMs() const;
    uint64_t GetCurrentTimeMs() const;
    void Seek(uint64_t seekPositionMS);
    bool SelectAudioTrack(uint32_t id);
    std::vector<CYIAbstractVideoPlayer::AudioTrackInfo> GetAudioTracks() const;
    CYIAbstractVideoPlayer::AudioTrackInfo GetActiveAudioTrack() const;
    bool IsMuted() const;
    void Mute(bool mute);
    bool IsTextTrackEnabled() const;
    void EnableTextTrack();
    void DisableTextTrack();
    bool SelectTextTrack(uint32_t textID, bool enableTextTrack = true);
    std::vector<CYIAbstractVideoPlayer::ClosedCaptionsTrackInfo> GetTextTracks() const;
    CYIAbstractVideoPlayer::ClosedCaptionsTrackInfo GetActiveTextTrack() const;
    void AddExternalTextTrack(const CYIString &url, const CYIString &language, const CYIString &label, const CYIString &type, const CYIString &format, bool enable);
    CYIAbstractVideoPlayer::TimedMetadataInterface *GetTimedMetadataInterface() const;

protected:
    CYIWebMessagingBridge::FutureResponse CallStaticPlayerFunction(yi::rapidjson::Document &&commandDocument, const CYIString &functionName, yi::rapidjson::Value &&playerFunctionArgumentsValue = yi::rapidjson::Value(yi::rapidjson::kArrayType), bool *pMessageSent = nullptr) const;
    CYIWebMessagingBridge::FutureResponse CallPlayerInstanceFunction(yi::rapidjson::Document &&commandDocument, const CYIString &functionName, yi::rapidjson::Value &&playerFunctionArgumentsValue = yi::rapidjson::Value(yi::rapidjson::kArrayType), bool *pMessageSent = nullptr) const;
    void CreatePlayerInstance();
    void InitializePlayerInstance();
    void DestroyPlayerInstance();

    static void AddDRMConfigurationToValue(CYIAbstractVideoPlayer::DRMConfiguration *pDRMConfiguration, yi::rapidjson::Value &value, yi::rapidjson::MemoryPoolAllocator<yi::rapidjson::CrtAllocator> &allocator);
    static bool ConvertValueToTrackInfo(const yi::rapidjson::Value &trackValue, CYIAbstractVideoPlayer::TrackInfo &trackData);

private:
    enum class PlayerState
    {
        Uninitialized,
        Initialized,
        Loading,
        Loaded,
        Paused,
        Playing,
        Complete
    };

    uint64_t RegisterEventHandler(const CYIString &eventName, CYIWebMessagingBridge::EventCallback &&eventCallback);
    void UnregisterEventHandler(uint64_t &eventHandlerId);
    void RegisterEventHandlers();
    void UnregisterEventHandlers();
    void OnBitrateChanged(const yi::rapidjson::Value &eventValue);
    void OnLiveStatusUpdated(const yi::rapidjson::Value &eventValue);
    void OnBufferingStateChanged(const yi::rapidjson::Value &eventValue);
    void OnPlayerErrorThrown(const yi::rapidjson::Value &eventValue);
    void OnAudioTracksChanged(const yi::rapidjson::Value &eventValue);
    void OnVideoDurationChanged(const yi::rapidjson::Value &eventValue);
    void OnVideoTimeChanged(const yi::rapidjson::Value &eventValue);
    void OnStateChanged(const yi::rapidjson::Value &eventValue);
    void OnTextTracksChanged(const yi::rapidjson::Value &eventValue);
    void OnMetadataAvailable(const yi::rapidjson::Value &eventValue);

    static CYIString PlayerStateToString(PlayerState state);

    bool m_messageHandlersRegistered;
    YI_RECT_REL m_previousVideoRectangle;
    CYIAbstractVideoPlayer::PlaybackState m_stateBeforeBuffering;
    uint64_t m_currentTimeMs;
    uint64_t m_durationMs;
    bool m_buffering;
    bool m_isLive;
    float m_initialAudioBitrateKbps;
    float m_currentAudioBitrateKbps;
    float m_initialVideoBitrateKbps;
    float m_currentVideoBitrateKbps;
    float m_initialTotalBitrateKbps;
    float m_currentTotalBitrateKbps;
    float m_bufferLengthMs;
    yi::rapidjson::Document m_playerConfiguration;

    uint64_t m_bitrateChangedEventHandlerId;
    uint64_t m_bufferingStateChangedEventHandlerId;
    uint64_t m_liveStatusEventHandlerId;
    uint64_t m_playerErrorEventHandlerId;
    uint64_t m_audioTracksChangedEventHandlerId;
    uint64_t m_videoDurationChangedEventHandlerId;
    uint64_t m_videoTimeChangedEventHandlerId;
    uint64_t m_stateChangedEventHandlerId;
    uint64_t m_textTracksChangedEventHandlerId;
    uint64_t m_metadataAvailableEventHandlerId;

    std::vector<CYIAbstractVideoPlayer::AudioTrackInfo> m_audioTracks;
    std::vector<CYIAbstractVideoPlayer::ClosedCaptionsTrackInfo> m_textTracks;

    CYIVideojsVideoPlayer *m_pPub;
};

#endif // _YI_VIDEOJS_VIDEO_PLAYER_PRIV_H_
