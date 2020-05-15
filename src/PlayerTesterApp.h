#ifndef _PLAYERTESTERAPP_H_
#define _PLAYERTESTERAPP_H_

#include <TestApp.h>
#include <event/YiEventHandler.h>
#include <framework/YiApp.h>
#include <network/YiHTTPHeader.h>
#include <player/YiAbstractVideoPlayer.h>
#include <signal/YiSignalHandler.h>

#if defined(YI_IOS)
#    include "AirplayRoutePicker.h"
#endif

class BufferingController;
class IStreamPlanetFairPlayHandler;

class CYIAbstractTimeline;
class CYIPushButtonView;
class CYISceneView;
class CYITextSceneNode;
class CYITextEditView;
class CYIVideoSurfaceView;

class PlayerTesterApp : public TestApp, public CYISignalHandler, public CYIEventHandler
{
public:
    PlayerTesterApp();
    virtual ~PlayerTesterApp();

    enum class DRMType
    {
        None,
        IStreamPlanetFairplay,
        MicrosoftPlayReadyTestServer,
        PlayReadyNoConfig,
        WideVineBitmovin,
        WideVineBitmovinCustomRequest
    };

    struct UrlAndFormat
    {
        UrlAndFormat()
            : drmScheme(CYIAbstractVideoPlayer::DRMScheme::None)
            , dRMType(DRMType::None)
        {
        }

        CYIString name;
        CYIString url;
        CYIAbstractVideoPlayer::StreamingFormat format;
        CYIAbstractVideoPlayer::DRMScheme drmScheme;
        DRMType dRMType;
        std::vector<CYIHTTPHeader> customHeaders;
        bool isErrorUrl = false;
        bool isLocalFile = false;
        bool isSuitableForSeekTest = true;
        bool isLive = false;
    };

    std::vector<UrlAndFormat> GetPossibleUrls() { return m_possibleURLs; };
    CYIAbstractVideoPlayer *GetVideoPlayer() { return m_pPlayer.get(); };
    void PrepareVideo(UrlAndFormat toPrepare, uint64_t startTime = 0);

protected:
    virtual bool UserInit() override;
    virtual bool UserStart() override;
    virtual void UserUpdate() override;

    void InitializeVideoSelector(CYISceneView *pMainComposition);

    void UpdatePlayerStats(const CYIAbstractVideoPlayer::Statistics &stats);
    void OnTotalBitrateChanged(float bitrate);
    void OnVideoBitrateChanged(float bitrate);
    void OnAudioBitrateChanged(float bitrate);
    void UpdateBufferRate(const CYIAbstractVideoPlayer::BufferLength &bufferLength);
    //video control hooks
    void OnStartButtonPressed();
    void OnStartLowResButtonPressed();
    void OnPlayButtonPressed();
    void OnPauseButtonPressed();
    void OnStopButtonPressed();
    void OnShowCCButtonPressed();
    void OnHideCCButtonPressed();
    void OnSeekButtonPressed(int32_t id);
    void OnSeekTextReturnPressed();
    void OnMaxBitrateButtonPressed();
    void OnMaxBitrateReturnPressed();
    void OnMinBufferLengthButtonPressed();
    void OnMinBufferLengthReturnPressed();
    void OnMaxBufferLengthButtonPressed();
    void OnMaxBufferLengthReturnPressed();
    void OnStartTimeButtonPressed();
    void OnStartTimeReturnPressed();
    void OnMinSeekButtonPressed();
    void OnMaxSeekButtonPressed();
    void OnMuteButtonPressed();
    void DisableStartButtons();
    void OnAnimateButtonPressed();
    void OnSwitchToMiniViewButtonPressed();
    void OnUserAgentButtonPressed();
    void OnUserAgentReturnPressed();

    //notifications from the player
    void ErrorOccured(CYIAbstractVideoPlayer::Error error);
    void VideoPreparing();
    void VideoReady();
    void VideoPlaying();
    void VideoPaused();
    void PlaybackComplete();
    void StateChanged(const CYIAbstractVideoPlayer::PlayerState &state);
    void CurrentTimeUpdated(uint64_t currentTimeMs);
    void VideoDurationChanged(uint64_t durationMs);
    void ChangeButtonPressed();
    void VideoSelectorHideAnimationCompleted();

    virtual bool HandleEvent(const std::shared_ptr<CYIEventDispatcher> &pDispatcher, CYIEvent *pEvent) override;

private:
    bool AppendUrlIfSupported(UrlAndFormat urlAndFormat);
    void OnURLSelected(int32_t buttonID);

    void HandleSeek(uint64_t seekPositionMS);

    std::unique_ptr<CYIAbstractVideoPlayer> m_pPlayer;
    CYIVideoSurfaceView *m_pPlayerSurfaceView;
    CYIVideoSurfaceView *m_pPlayerSurfaceMiniView;
    bool m_playerIsMini;
    CYISceneView *m_pErrorView;

    BufferingController *m_pBufferingController;

    CYIAbstractTimeline *m_pAnimateVideoTimeline;
    CYIAbstractTimeline *m_pShowVideoSelectorTimeline;
    CYISceneView *m_pVideoSelectorView;

    //playback buttons
    CYIPushButtonView *m_pStartButton;
    CYIPushButtonView *m_pStartLowResButton;
    CYIPushButtonView *m_pPlayButton;
    CYIPushButtonView *m_pPauseButton;
    CYIPushButtonView *m_pStopButton;
    CYIPushButtonView *m_pShowCCButton;
    CYIPushButtonView *m_pHideCCButton;
    CYIPushButtonView *m_pSeekForwardButton;
    CYIPushButtonView *m_pSeekReverseButton;
    CYIPushButtonView *m_pSeekButton;
    CYIPushButtonView *m_pMaxBitrateButton;
    CYIPushButtonView *m_pMinBufferLengthButton;
    CYIPushButtonView *m_pMaxBufferLengthButton;
    CYIPushButtonView *m_pStartTimeButton;
    CYIPushButtonView *m_pMinSeekButton;
    CYIPushButtonView *m_pMaxSeekButton;
    CYIPushButtonView *m_pMuteButton;
    CYIPushButtonView *m_pChangeURLButton;
    CYIPushButtonView *m_pSwitchToMiniViewButton;

    //video stats
    CYITextSceneNode *m_pDurationText;
    CYITextSceneNode *m_pCurrentTimeText;
    CYITextSceneNode *m_pStatusText;
    CYITextSceneNode *m_pIsLiveText;
    CYITextSceneNode *m_pTotalBitrateText;
    CYITextSceneNode *m_pDefaultTotalBitrateText;
    CYITextSceneNode *m_pVideoBitrateText;
    CYITextSceneNode *m_pDefaultVideoBitrateText;
    CYITextSceneNode *m_pAudioBitrateText;
    CYITextSceneNode *m_pDefaultAudioBitrateText;
    CYITextSceneNode *m_pBufferLengthText;
    CYITextSceneNode *m_pMinBufferLengthStatText;
    CYITextSceneNode *m_pMaxBufferLengthStatText;
    CYITextSceneNode *m_pFPSText;
    CYITextEditView *m_pCurrentUrlText;
    CYITextEditView *m_pCurrentFormatText;
    CYITextEditView *m_pSeekText;
    CYITextEditView *m_pMaxBitrateText;
    CYITextEditView *m_pMinBufferLengthText;
    CYITextEditView *m_pMaxBufferLengthText;
    CYITextEditView *m_pStartTimeText;

    //player information
    CYIPushButtonView *m_pUserAgentButton;
    CYITextEditView *m_pUserAgentText;

    UrlAndFormat *m_pActiveFormat;
    std::unique_ptr<IStreamPlanetFairPlayHandler> m_pIStreamPlanetFairPlayHandler;
    std::vector<UrlAndFormat> m_possibleURLs;

    YI_TYPE_BASES(PlayerTesterApp, TestApp)
#if defined(YI_IOS)
    AirplayRoutePicker m_RoutePicker;
    void UpdateRouteButton();
#endif

    void ResetStatisticsLabelsToDefault();
};

#endif
