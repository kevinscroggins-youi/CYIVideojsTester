#include "PlayerTesterApp.h"

#include "IStreamPlanetFairPlayHandler.h"

#include <animation/YiTimeline.h>
#include <asset/YiAssetLoader.h>
#include <asset/YiAssetVideo.h>
#include <asset/YiAssetViewTemplate.h>
#include <event/YiKeyEvent.h>
#include <framework/YiFramework.h>
#include <import/YiViewTemplate.h>
#include <network/YiHTTPRequest.h>
#include <network/YiHTTPService.h>
#include <platform/YiDeviceBridgeLocator.h>
#include <platform/YiDevicePowerManagementBridge.h>
#include <player/YiAbstractVideoPlayer.h>
#include <player/YiBlueskyVideoPlayer.h>
#include <player/YiCastLabsVideoPlayer.h>
#include <player/YiDefaultVideoPlayerFactory.h>
#include <player/YiPlayReadyDRMConfiguration.h>
#include <player/YiVideoSurface.h>
#include <player/YiVideoSurfaceView.h>
#include <player/YiWidevineModularCustomRequestDRMConfiguration.h>
#include <player/YiWidevineModularDRMConfiguration.h>
#include <scenetree/YiSceneManager.h>
#include <scenetree/YiTextSceneNode.h>
#include <utility/YiColor.h>
#include <utility/YiCondition.h>
#include <utility/YiSceneNodeUtility.h>
#include <utility/YiString.h>
#include <utility/YiTimeConversion.h>
#include <utility/YiTimer.h>
#include <view/YiActivityIndicatorView.h>
#include <view/YiPushButtonView.h>
#include <view/YiSceneView.h>
#include <view/YiTextEditView.h>

#if defined(YI_TIZEN_NACL)
#    include "YiTizenNaClRemoteLoggerSink.h"
#    include "YiVideojsVideoPlayer.h"
#    include <player/YiTizenNaClVideoPlayer.h>
#endif

#define LOG_TAG "PlayerTesterApp"

YI_TYPE_DEF_INST(PlayerTesterApp, TestApp)

class BufferingController : public CYISignalHandler
{
public:
    uint32_t m_minimumTime;
    CYITimer m_minimumBufferingTimer;
    CYIActivityIndicatorView *m_pView;
    CYIConditionEvaluator m_endingEvaluator;
    CYICondition m_bufferingStopped;
    CYICondition m_timerCompleted;

    BufferingController(CYIAbstractVideoPlayer *pPlayer, CYIActivityIndicatorView *pView, uint32_t minimumTime)
        : m_minimumTime(minimumTime)
        , m_pView(pView)
    {
        m_endingEvaluator.AddCondition(&m_bufferingStopped);
        m_endingEvaluator.AddCondition(&m_timerCompleted);
        m_endingEvaluator.Success.Connect(*this, &BufferingController::OnBothConditions);
        m_minimumBufferingTimer.TimedOut.Connect(*this, &BufferingController::OnTimeout);
        pPlayer->BufferingStarted.Connect(*this, &BufferingController::OnBufferingStarted, EYIConnectionType::Async);
        pPlayer->BufferingEnded.Connect(*this, &BufferingController::OnBufferingStopped, EYIConnectionType::Async);
    }

    void OnBufferingStarted()
    {
        m_endingEvaluator.Reset();
        m_minimumBufferingTimer.Start(m_minimumTime);
        m_pView->Start();
    }

    void OnBufferingStopped()
    {
        m_bufferingStopped.Set();
    }

    void OnTimeout()
    {
        m_timerCompleted.Set();
    }

    void OnBothConditions()
    {
        m_pView->Stop();
    }
};

static void ConfigureCapabilities(CYIVideoSurface *pSurface, CYITextSceneNode *pTextNode)
{
    CYIString text;
    if (Any(pSurface->GetCapabilities() & CYIVideoSurface::Capabilities::RenderToTexture))
    {
        text += "Render to texture\n";
    }
    if (Any(pSurface->GetCapabilities() & CYIVideoSurface::Capabilities::MultipleViews))
    {
        text += "Multiple views\n";
    }
    if (Any(pSurface->GetCapabilities() & CYIVideoSurface::Capabilities::Translate))
    {
        text += "Translate\n";
    }
    if (Any(pSurface->GetCapabilities() & CYIVideoSurface::Capabilities::Scale))
    {
        text += "Scale\n";
    }
    if (Any(pSurface->GetCapabilities() & CYIVideoSurface::Capabilities::FreeTransform))
    {
        text += "Free transform\n";
    }
    if (Any(pSurface->GetCapabilities() & CYIVideoSurface::Capabilities::Opacity))
    {
        text += "Opacity\n";
    }

    pTextNode->SetText(text);
}

PlayerTesterApp::PlayerTesterApp()
    : m_pPlayer(nullptr)
    , m_pPlayerSurfaceView(nullptr)
    , m_pPlayerSurfaceMiniView(nullptr)
    , m_playerIsMini(false)
    , m_pErrorView(nullptr)
    , m_pBufferingController(nullptr)
    , m_pAnimateVideoTimeline(nullptr)
    , m_pShowVideoSelectorTimeline(nullptr)
    , m_pVideoSelectorView(nullptr)
    , m_pStartButton(nullptr)
    , m_pStartLowResButton(nullptr)
    , m_pPlayButton(nullptr)
    , m_pPauseButton(nullptr)
    , m_pStopButton(nullptr)
    , m_pShowCCButton(nullptr)
    , m_pHideCCButton(nullptr)
    , m_pSeekForwardButton(nullptr)
    , m_pSeekReverseButton(nullptr)
    , m_pSeekButton(nullptr)
    , m_pMaxBitrateButton(nullptr)
    , m_pMinBufferLengthButton(nullptr)
    , m_pMaxBufferLengthButton(nullptr)
    , m_pStartTimeButton(nullptr)
    , m_pMinSeekButton(nullptr)
    , m_pMaxSeekButton(nullptr)
    , m_pChangeURLButton(nullptr)
    , m_pSwitchToMiniViewButton(nullptr)
    , m_pDurationText(nullptr)
    , m_pCurrentTimeText(nullptr)
    , m_pStatusText(nullptr)
    , m_pIsLiveText(nullptr)
    , m_pTotalBitrateText(nullptr)
    , m_pDefaultTotalBitrateText(nullptr)
    , m_pVideoBitrateText(nullptr)
    , m_pDefaultVideoBitrateText(nullptr)
    , m_pAudioBitrateText(nullptr)
    , m_pDefaultAudioBitrateText(nullptr)
    , m_pBufferLengthText(nullptr)
    , m_pMinBufferLengthStatText(nullptr)
    , m_pMaxBufferLengthStatText(nullptr)
    , m_pFPSText(nullptr)
    , m_pCurrentUrlText(nullptr)
    , m_pCurrentFormatText(nullptr)
    , m_pSeekText(nullptr)
    , m_pMaxBitrateText(nullptr)
    , m_pMinBufferLengthText(nullptr)
    , m_pMaxBufferLengthText(nullptr)
    , m_pStartTimeText(nullptr)
    , m_pUserAgentButton(nullptr)
    , m_pUserAgentText(nullptr)
    , m_pActiveFormat(nullptr)
#if defined(YI_IOS)
    , m_RoutePicker()
#endif
{
//#if defined(TIZEN_NACL)
    CYILogger::AddSink(std::make_shared<CYITizenNaClRemoteLoggerSink>());
//#endif // TIZEN_NACL
}

PlayerTesterApp::~PlayerTesterApp()
{
    GetMasterAppSceneManager()->RemoveScene("Main");
    m_pPlayer.reset();

    delete m_pBufferingController;
    m_pBufferingController = nullptr;
#if defined(YI_IOS)
    m_RoutePicker.SetVisible(false);
#endif
}

enum class SeekButtonIndex
{
    Forward = 0,
    Reverse = 1,
    General = 2
};

bool PlayerTesterApp::UserInit()
{
    CYIEventDispatcher::GetDefaultDispatcher()->RegisterEventHandler(this);

    std::unique_ptr<CYISceneView> pOwnedMainComposition = GetMasterAppSceneManager()->LoadScene("PlayerTester_MainComp.layout", CYISceneManager::ScaleType::Fit, CYISceneManager::VerticalAlignmentType::Center, CYISceneManager::HorizontalAlignmentType::Center);

    CYISceneView *pMainComposition = pOwnedMainComposition.get();
    if (!pMainComposition)
    {
        YI_LOGE(LOG_TAG, "Loading scene has failed");
        return false;
    }

    if (!GetMasterAppSceneManager()->AddScene("Main", std::move(pOwnedMainComposition), 0, CYISceneManager::LayerType::Opaque))
    {
        YI_LOGE(LOG_TAG, "Loading main scene has failed");
        return false;
    }

    GetMasterAppSceneManager()->StageScene("Main");

    // we can't instansiate the player in the constructor because on Android the CYIActivity is not available yet
#if defined(YI_TIZEN_NACL)
    m_pPlayer = std::unique_ptr<CYIVideojsVideoPlayer>(CYIVideojsVideoPlayer::Create());
#else
    m_pPlayer = CYIDefaultVideoPlayerFactory::Create();
#endif // YI_TIZEN_NACL
    m_pPlayer->Init();
    m_pPlayer->ErrorOccurred.Connect(*this, &PlayerTesterApp::ErrorOccured);
    m_pPlayer->Preparing.Connect(*this, &PlayerTesterApp::VideoPreparing);
    m_pPlayer->Ready.Connect(*this, &PlayerTesterApp::VideoReady);
    m_pPlayer->CurrentTimeUpdated.Connect(*this, &PlayerTesterApp::CurrentTimeUpdated);
    m_pPlayer->DurationChanged.Connect(*this, &PlayerTesterApp::VideoDurationChanged);
    m_pPlayer->Paused.Connect(*this, &PlayerTesterApp::VideoPaused);
    m_pPlayer->Playing.Connect(*this, &PlayerTesterApp::VideoPlaying);
    m_pPlayer->PlaybackComplete.Connect(*this, &PlayerTesterApp::PlaybackComplete);
    m_pPlayer->PlayerStateChanged.Connect(*this, &PlayerTesterApp::StateChanged);
    m_pPlayer->TotalBitrateChanged.Connect(*this, &PlayerTesterApp::OnTotalBitrateChanged);
    m_pPlayer->VideoBitrateChanged.Connect(*this, &PlayerTesterApp::OnVideoBitrateChanged);
    m_pPlayer->AudioBitrateChanged.Connect(*this, &PlayerTesterApp::OnAudioBitrateChanged);

    CYIAbstractVideoPlayer::Statistics statistics = m_pPlayer->GetStatistics();

    pMainComposition->FindNode(m_pPlayerSurfaceView, "VideoSurfaceView", CYISceneNode::FetchType::Mandatory, LOG_TAG);
    if (!m_pPlayerSurfaceView)
    {
        YI_LOGE(LOG_TAG, "VideoSurfaceView not found or is not a CYIVideoSurfaceView");
        return false;
    }
    m_pPlayerSurfaceView->SetVideoSurface(m_pPlayer->GetSurface());
    m_playerIsMini = false;

    pMainComposition->FindNode(m_pPlayerSurfaceMiniView, "VideoSurfaceViewMini", CYISceneNode::FetchType::Mandatory, LOG_TAG);
    if (!m_pPlayerSurfaceMiniView)
    {
        YI_LOGE(LOG_TAG, "VideoSurfaceView not found or is not a CYIVideoSurfaceView");
        return false;
    }

    m_pMuteButton = pMainComposition->GetNode<CYIPushButtonView>("MuteButton");
    YI_ASSERT(m_pMuteButton, LOG_TAG, "Could not find 'MuteButton'");
    m_pMuteButton->ButtonClicked.Connect(*this, &PlayerTesterApp::OnMuteButtonPressed);

    m_pStartButton = pMainComposition->GetNode<CYIPushButtonView>("StartButton");
    YI_ASSERT(m_pStartButton, LOG_TAG, "Could not find 'StartButton'");
    m_pStartButton->ButtonClicked.Connect(*this, &PlayerTesterApp::OnStartButtonPressed);

    m_pStartLowResButton = pMainComposition->GetNode<CYIPushButtonView>("StartSmallButton");
    YI_ASSERT(m_pStartLowResButton, LOG_TAG, "Could not find 'StartSmallButton'");
    m_pStartLowResButton->ButtonClicked.Connect(*this, &PlayerTesterApp::OnStartLowResButtonPressed);

    m_pPlayButton = pMainComposition->GetNode<CYIPushButtonView>("PlayButton");
    YI_ASSERT(m_pPlayButton, LOG_TAG, "Could not find 'PlayButton'");
    m_pPlayButton->ButtonClicked.Connect(*this, &PlayerTesterApp::OnPlayButtonPressed);
    m_pPlayButton->Disable();

    m_pPauseButton = pMainComposition->GetNode<CYIPushButtonView>("PauseButton");
    YI_ASSERT(m_pPauseButton, LOG_TAG, "Could not find 'PauseButton'");
    m_pPauseButton->ButtonClicked.Connect(*this, &PlayerTesterApp::OnPauseButtonPressed);
    m_pPauseButton->Disable();

    m_pStopButton = pMainComposition->GetNode<CYIPushButtonView>("StopButton");
    YI_ASSERT(m_pStopButton, LOG_TAG, "Could not find 'StopButton'");
    m_pStopButton->ButtonClicked.Connect(*this, &PlayerTesterApp::OnStopButtonPressed);

    m_pShowCCButton = pMainComposition->GetNode<CYIPushButtonView>("ShowCCButton");
    YI_ASSERT(m_pShowCCButton, LOG_TAG, "Could not find 'ShowCCButton'");
    m_pShowCCButton->ButtonClicked.Connect(*this, &PlayerTesterApp::OnShowCCButtonPressed);
    m_pShowCCButton->Disable();

    m_pHideCCButton = pMainComposition->GetNode<CYIPushButtonView>("HideCCButton");
    YI_ASSERT(m_pHideCCButton, LOG_TAG, "Could not find 'HideCCButton'");
    m_pHideCCButton->ButtonClicked.Connect(*this, &PlayerTesterApp::OnHideCCButtonPressed);
    m_pHideCCButton->Disable();

    m_pSeekForwardButton = pMainComposition->GetNode<CYIPushButtonView>("SeekForwardButton");
    YI_ASSERT(m_pSeekForwardButton, LOG_TAG, "Could not find 'SeekForwardButton'");
    m_pSeekForwardButton->ButtonClicked.Connect(*this, &PlayerTesterApp::OnSeekButtonPressed);
    m_pSeekForwardButton->Disable();
    m_pSeekForwardButton->SetButtonID((int32_t)SeekButtonIndex::Forward);

    m_pSeekReverseButton = pMainComposition->GetNode<CYIPushButtonView>("SeekReverseButton");
    YI_ASSERT(m_pSeekReverseButton, LOG_TAG, "Could not find 'SeekReverseButton'");
    m_pSeekReverseButton->ButtonClicked.Connect(*this, &PlayerTesterApp::OnSeekButtonPressed);
    m_pSeekReverseButton->Disable();
    m_pSeekReverseButton->SetButtonID((int32_t)SeekButtonIndex::Reverse);

    m_pSwitchToMiniViewButton = pMainComposition->GetNode<CYIPushButtonView>("MiniButton");
    YI_ASSERT(m_pSwitchToMiniViewButton, LOG_TAG, "Could not find 'MiniButton'");
    m_pSwitchToMiniViewButton->ButtonClicked.Connect(*this, &PlayerTesterApp::OnSwitchToMiniViewButtonPressed);

    CYIPushButtonView *pAnimateButton = pMainComposition->GetNode<CYIPushButtonView>("AnimateButton");
    YI_ASSERT(pAnimateButton, LOG_TAG, "Could not find 'AnimateButton'");
    if ((m_pPlayer->GetSurface()->GetCapabilities() & (CYIVideoSurface::Capabilities::Translate | CYIVideoSurface::Capabilities::Scale)) == (CYIVideoSurface::Capabilities::Translate | CYIVideoSurface::Capabilities::Scale))
    {
        pAnimateButton->ButtonClicked.Connect(*this, &PlayerTesterApp::OnAnimateButtonPressed);
    }
    else
    {
        // Disable the animate button if the surface does not support translate and scale used by the animation.
        pAnimateButton->Disable();
    }

    m_pDurationText = pMainComposition->GetNode<CYITextSceneNode>("placeholder-duration");
    YI_ASSERT(m_pDurationText, LOG_TAG, "Could not find 'placeholder-duration'");
    m_pDurationText->SetText("00:00:00");

    m_pCurrentTimeText = pMainComposition->GetNode<CYITextSceneNode>("placeholder-currentTime");
    YI_ASSERT(m_pCurrentTimeText, LOG_TAG, "Could not find 'placeholder-currentTime'");
    m_pCurrentTimeText->SetText("00:00:00");

    m_pStatusText = pMainComposition->GetNode<CYITextSceneNode>("placeholder-status");
    YI_ASSERT(m_pStatusText, LOG_TAG, "Could not find 'placeholder-status'");
    m_pStatusText->SetText("Idle");

    m_pIsLiveText = pMainComposition->GetNode<CYITextSceneNode>("placeholder-islive");
    YI_ASSERT(m_pIsLiveText, LOG_TAG, "Could not find 'placeholder-islive'");

    m_pTotalBitrateText = pMainComposition->GetNode<CYITextSceneNode>("placeholder-totalbitrate");
    YI_ASSERT(m_pTotalBitrateText, LOG_TAG, "Could not find 'placeholder-totalbitrate'");
    m_pTotalBitrateText->SetText(CYIString::FromFloat(statistics.totalBitrateKbps));

    m_pDefaultTotalBitrateText = pMainComposition->GetNode<CYITextSceneNode>("placeholder-defaulttotalbitrate");
    YI_ASSERT(m_pDefaultTotalBitrateText, LOG_TAG, "Could not find 'placeholder-defaulttotalbitrate'");

    m_pVideoBitrateText = pMainComposition->GetNode<CYITextSceneNode>("placeholder-videobitrate");
    YI_ASSERT(m_pVideoBitrateText, LOG_TAG, "Could not find 'placeholder-videobitrate'");
    m_pVideoBitrateText->SetText(CYIString::FromFloat(statistics.videoBitrateKbps));

    m_pDefaultVideoBitrateText = pMainComposition->GetNode<CYITextSceneNode>("placeholder-defaultvideobitrate");
    YI_ASSERT(m_pDefaultVideoBitrateText, LOG_TAG, "Could not find 'placeholder-defaultvideobitrate'");

    m_pAudioBitrateText = pMainComposition->GetNode<CYITextSceneNode>("placeholder-audiobitrate");
    YI_ASSERT(m_pAudioBitrateText, LOG_TAG, "Could not find 'placeholder-bitrate'");
    m_pAudioBitrateText->SetText(CYIString::FromFloat(statistics.audioBitrateKbps));

    m_pDefaultAudioBitrateText = pMainComposition->GetNode<CYITextSceneNode>("placeholder-defaultaudiobitrate");
    YI_ASSERT(m_pDefaultAudioBitrateText, LOG_TAG, "Could not find 'placeholder-defaultaudiobitrate'");

    m_pBufferLengthText = pMainComposition->GetNode<CYITextSceneNode>("placeholder-bufferlength");
    YI_ASSERT(m_pBufferLengthText, LOG_TAG, "Could not find 'placeholder-bufferlength'");

    m_pMinBufferLengthStatText = pMainComposition->GetNode<CYITextSceneNode>("placeholder-minbufferlength");
    YI_ASSERT(m_pMinBufferLengthStatText, LOG_TAG, "Could not find 'placeholder-minbufferlength'");

    m_pMaxBufferLengthStatText = pMainComposition->GetNode<CYITextSceneNode>("placeholder-maxbufferlength");
    YI_ASSERT(m_pMaxBufferLengthStatText, LOG_TAG, "Could not find 'placeholder-maxbufferlength'");

    m_pFPSText = pMainComposition->GetNode<CYITextSceneNode>("placeholder-fps");
    YI_ASSERT(m_pFPSText, LOG_TAG, "Could not find 'placeholder-fps'");

    UpdatePlayerStats(CYIAbstractVideoPlayer::Statistics());
    UpdateBufferRate(CYIAbstractVideoPlayer::BufferLength());

    CYIActivityIndicatorView *pBufferingView = pMainComposition->GetNode<CYIActivityIndicatorView>("Buffering");
    YI_ASSERT(pBufferingView, LOG_TAG, "Could not find 'Buffering'");
    m_pBufferingController = new BufferingController(m_pPlayer.get(), pBufferingView, 700);

    m_pErrorView = pMainComposition->GetNode<CYISceneView>("Error");
    YI_ASSERT(m_pErrorView, LOG_TAG, "Could not find 'Error'");
    m_pErrorView->Hide();

    m_pAnimateVideoTimeline = pMainComposition->GetTimeline("AnimateVideo");
    m_pShowVideoSelectorTimeline = pMainComposition->GetTimeline("ShowVideoSelector");
    m_pShowVideoSelectorTimeline->CompletedReverse.Connect(*this, &PlayerTesterApp::VideoSelectorHideAnimationCompleted);

    CYITextSceneNode *pCapabilitiesText = pMainComposition->GetNode("Capabilities")->GetNode<CYITextSceneNode>("placeholder-text");
    ConfigureCapabilities(m_pPlayer->GetSurface(), pCapabilitiesText);

    m_pChangeURLButton = pMainComposition->GetNode<CYIPushButtonView>("ChangeURLButton");
    YI_ASSERT(m_pChangeURLButton, LOG_TAG, "Could not find 'ChangeURLButton'");
    m_pChangeURLButton->ButtonClicked.Connect(*this, &PlayerTesterApp::ChangeButtonPressed);

    m_pCurrentUrlText = pMainComposition->GetNode<CYITextEditView>("placeholder-url");
    YI_ASSERT(m_pCurrentUrlText, LOG_TAG, "Could not find 'placeholder-url'");

    m_pCurrentFormatText = pMainComposition->GetNode<CYITextEditView>("placeholder-format");
    YI_ASSERT(m_pCurrentFormatText, LOG_TAG, "Could not find 'placeholder-format'");

    CYITextSceneNode *pPlayerInfoText = nullptr;
    pPlayerInfoText = pMainComposition->GetNode<CYITextSceneNode>("placeholder-playerinfo");
    YI_ASSERT(pPlayerInfoText, LOG_TAG, "Could not find 'placeholder-playerinfo");
    pPlayerInfoText->SetText(m_pPlayer->GetName() + " - " + m_pPlayer->GetVersion());

    m_pSeekText = pMainComposition->GetNode<CYITextEditView>("SeekTextEdit");
    YI_ASSERT(m_pSeekText, LOG_TAG, "Could not find 'SeekTextEdit'");
    m_pSeekText->Hide();
    m_pSeekText->ReturnKeyPressed.Connect(*this, &PlayerTesterApp::OnSeekTextReturnPressed);

    m_pSeekButton = pMainComposition->GetNode<CYIPushButtonView>("SeekButton");
    YI_ASSERT(m_pSeekButton, LOG_TAG, "Could not find 'SeekButton'");
    m_pSeekButton->SetButtonID((int32_t)SeekButtonIndex::General);
    m_pSeekButton->ButtonClicked.Connect(*this, &PlayerTesterApp::OnSeekButtonPressed);
    m_pSeekButton->Disable();

    m_pMaxBitrateText = pMainComposition->GetNode<CYITextEditView>("MaxBitrateTextEdit");
    YI_ASSERT(m_pMaxBitrateText, LOG_TAG, "Could not find 'MaxBitrateTextEdit'");
    m_pMaxBitrateText->Hide();
    m_pMaxBitrateText->ReturnKeyPressed.Connect(*this, &PlayerTesterApp::OnMaxBitrateReturnPressed);

    m_pMaxBitrateButton = pMainComposition->GetNode<CYIPushButtonView>("MaxBitrateButton");
    YI_ASSERT(m_pMaxBitrateButton, LOG_TAG, "Could not find 'MaxBitrateButton'");
    m_pMaxBitrateButton->ButtonClicked.Connect(*this, &PlayerTesterApp::OnMaxBitrateButtonPressed);

    m_pMinBufferLengthText = pMainComposition->GetNode<CYITextEditView>("MinBufferLengthTextEdit");
    YI_ASSERT(m_pMinBufferLengthText, LOG_TAG, "Could not find 'MinBufferLengthTextEdit'");
    m_pMinBufferLengthText->SetValue("-1");
    m_pMinBufferLengthText->Hide();
    m_pMinBufferLengthText->ReturnKeyPressed.Connect(*this, &PlayerTesterApp::OnMinBufferLengthReturnPressed);

    m_pMinBufferLengthButton = pMainComposition->GetNode<CYIPushButtonView>("MinBufferLengthButton");
    YI_ASSERT(m_pMinBufferLengthButton, LOG_TAG, "Could not find 'MinBufferLengthButton'");
    m_pMinBufferLengthButton->ButtonClicked.Connect(*this, &PlayerTesterApp::OnMinBufferLengthButtonPressed);

    m_pMaxBufferLengthText = pMainComposition->GetNode<CYITextEditView>("MaxBufferLengthTextEdit");
    YI_ASSERT(m_pMaxBufferLengthText, LOG_TAG, "Could not find 'MaxBufferLengthTextEdit'");
    m_pMaxBufferLengthText->SetValue("-1");
    m_pMaxBufferLengthText->Hide();
    m_pMaxBufferLengthText->ReturnKeyPressed.Connect(*this, &PlayerTesterApp::OnMaxBufferLengthReturnPressed);

    m_pMaxBufferLengthButton = pMainComposition->GetNode<CYIPushButtonView>("MaxBufferLengthButton");
    YI_ASSERT(m_pMaxBufferLengthButton, LOG_TAG, "Could not find 'MaxBufferLengthButton'");
    m_pMaxBufferLengthButton->ButtonClicked.Connect(*this, &PlayerTesterApp::OnMaxBufferLengthButtonPressed);

    m_pStartTimeText = pMainComposition->GetNode<CYITextEditView>("StartTimeTextEdit");
    YI_ASSERT(m_pStartTimeText, LOG_TAG, "Could not find 'StartTimeTextEdit'");
    m_pStartTimeText->SetValue("0");
    m_pStartTimeText->Hide();
    m_pStartTimeText->ReturnKeyPressed.Connect(*this, &PlayerTesterApp::OnStartTimeReturnPressed);

    m_pMinSeekButton = pMainComposition->GetNode<CYIPushButtonView>("MinSeekButton");
    YI_ASSERT(m_pMinSeekButton, LOG_TAG, "Could not find 'MinSeekButton'");
    m_pMinSeekButton->ButtonClicked.Connect(*this, &PlayerTesterApp::OnMinSeekButtonPressed);
    m_pMinSeekButton->Disable();

    m_pMaxSeekButton = pMainComposition->GetNode<CYIPushButtonView>("MaxSeekButton");
    YI_ASSERT(m_pMaxSeekButton, LOG_TAG, "Could not find 'MaxSeekButton'");
    m_pMaxSeekButton->ButtonClicked.Connect(*this, &PlayerTesterApp::OnMaxSeekButtonPressed);
    m_pMaxSeekButton->Disable();

    m_pStartTimeButton = pMainComposition->GetNode<CYIPushButtonView>("StartTimeButton");
    YI_ASSERT(m_pStartTimeButton, LOG_TAG, "Could not find 'StartTimeButton'");
    m_pStartTimeButton->ButtonClicked.Connect(*this, &PlayerTesterApp::OnStartTimeButtonPressed);

    m_pUserAgentButton = pMainComposition->GetNode<CYIPushButtonView>("UserAgentButton");
    YI_ASSERT(m_pUserAgentButton, LOG_TAG, "Could not find 'UserAgentButton'");
    m_pUserAgentButton->ButtonClicked.Connect(*this, &PlayerTesterApp::OnUserAgentButtonPressed);

    m_pUserAgentText = pMainComposition->GetNode<CYITextEditView>("UserAgentTextEdit");
    YI_ASSERT(m_pUserAgentText, LOG_TAG, "Could not find 'UserAgentTextEdit'");
    m_pUserAgentText->SetValue("PlayerTester/...");
    m_pUserAgentText->Hide();
    m_pUserAgentText->ReturnKeyPressed.Connect(*this, &PlayerTesterApp::OnUserAgentReturnPressed);

    CYIPushButtonView *pBackgroundPlaybackButton = pMainComposition->GetNode<CYIPushButtonView>("BackgroundPlaybackButton");
    YI_ASSERT(pBackgroundPlaybackButton, LOG_TAG, "Could not find 'BackgroundPlaybackButton'");

    CYIPushButtonView *pHeadphoneJackPauseButton = pMainComposition->GetNode<CYIPushButtonView>("HeadphoneJackPauseButton");
    YI_ASSERT(pHeadphoneJackPauseButton, LOG_TAG, "Could not find 'HeadphoneJackPauseButton'");

    CYIAbstractVideoPlayer::BackgroundPlaybackInterface *pBackgroundPlaybackInterface = m_pPlayer->GetBackgroundPlaybackInterface();
    if (pBackgroundPlaybackInterface)
    {
        pBackgroundPlaybackInterface->SetMetadata("Notification Title Test", "Notification Text Test", CYIUrl("http://www.digitaltvnews.net/wp-content/uploads/logos/youitv.png"));
        pBackgroundPlaybackInterface->SetRewindIncrementMs(std::chrono::milliseconds(10000));
        pBackgroundPlaybackInterface->SetFastForwardIncrementMs(std::chrono::milliseconds(20000));
        pBackgroundPlaybackInterface->SetNotificationColor(CYIColor::Named().LightCoral);

        pBackgroundPlaybackButton->SetText("BG Play Off");
        pBackgroundPlaybackButton->ButtonPressed.Connect([this, pBackgroundPlaybackButton]() {
            CYIAbstractVideoPlayer::BackgroundPlaybackInterface *pBackgroundPlayback = m_pPlayer->GetBackgroundPlaybackInterface();
            if (pBackgroundPlayback)
            {
                if (pBackgroundPlayback->IsBackgroundPlaybackEnabled())
                {
                    pBackgroundPlaybackButton->SetText("BG Play Off");
                    pBackgroundPlayback->EnableBackgroundPlayback(false);
                }
                else
                {
                    pBackgroundPlaybackButton->SetText("BG Play On");
                    pBackgroundPlayback->EnableBackgroundPlayback(true);
                }
            }
        });
    }
    else
    {
        pBackgroundPlaybackButton->Disable();
    }

    bool headphoneJackInterfaceAvailable = m_pPlayer->GetHeadphoneJackInterface() != nullptr;
    if (headphoneJackInterfaceAvailable)
    {
        pHeadphoneJackPauseButton->SetText("HJ Pause Off");
        pHeadphoneJackPauseButton->ButtonPressed.Connect([this, pHeadphoneJackPauseButton]() {
            CYIAbstractVideoPlayer::HeadphoneJackInterface *pHeadphoneJackInterface = m_pPlayer->GetHeadphoneJackInterface();
            if (pHeadphoneJackInterface)
            {
                if (pHeadphoneJackInterface->IsPausingOnHeadphonesUnplugged())
                {
                    pHeadphoneJackPauseButton->SetText("HJ Pause Off");
                    pHeadphoneJackInterface->EnablePauseOnHeadphonesUnplugged(false);
                }
                else
                {
                    pHeadphoneJackPauseButton->SetText("HJ Pause On");
                    pHeadphoneJackInterface->EnablePauseOnHeadphonesUnplugged(true);
                }
            }
        });
    }
    else
    {
        pHeadphoneJackPauseButton->Disable();
    }

    InitializeVideoSelector(pMainComposition);

    return true;
}

#if defined(YI_IOS)
void PlayerTesterApp::UpdateRouteButton()
{
    CYIAABB startButtonScreenSpace = CYISceneNodeUtility::WorldToScreenSpace(m_pStartTimeButton);
    glm::vec3 routePickerPosition = startButtonScreenSpace.GetBottomRight();
    routePickerPosition.y -= startButtonScreenSpace.GetHeight();
    routePickerPosition.x += startButtonScreenSpace.GetHalfDimensions().x;
    m_RoutePicker.UpdateLocation(routePickerPosition);
}
#endif

void PlayerTesterApp::ChangeButtonPressed()
{
#if defined(YI_IOS)
    m_RoutePicker.SetVisible(false);
#endif
    m_pVideoSelectorView->Show();
    m_pShowVideoSelectorTimeline->ContinueForward();
    m_pVideoSelectorView->RequestFocus();
}

void PlayerTesterApp::VideoSelectorHideAnimationCompleted()
{
#if defined(YI_IOS)
    m_RoutePicker.SetVisible(true);
#endif
    m_pVideoSelectorView->Hide();
}

bool PlayerTesterApp::UserStart()
{
    CYIString sslRootCertFilePath = GetAssetsPath() + "/cacert.pem";
    CYIHTTPService::GetInstance()->SetSSLRootCertificate(sslRootCertFilePath);
    CYIHTTPService::GetInstance()->Start();
    return true;
}

void PlayerTesterApp::UserUpdate()
{
    if (m_pPlayer)
    {
        UpdatePlayerStats(m_pPlayer->GetStatistics());

        CYIAbstractVideoPlayer::BufferingInterface *pBufferingInterface = m_pPlayer->GetBufferingInterface();
        if (pBufferingInterface)
        {
            UpdateBufferRate(pBufferingInterface->GetBufferLength());
        }
    }
#if defined(YI_IOS)
    UpdateRouteButton();
#endif
}

static CYIString FormatToString(CYIAbstractVideoPlayer::StreamingFormat format)
{
    switch (format)
    {
        case CYIAbstractVideoPlayer::StreamingFormat::HLS:
            return "HLS";
        case CYIAbstractVideoPlayer::StreamingFormat::Smooth:
            return "SMOOTH";
        case CYIAbstractVideoPlayer::StreamingFormat::MP4:
            return "MP4";
        case CYIAbstractVideoPlayer::StreamingFormat::DASH:
            return "DASH";
    }

    return "UNRECOGNIZED";
}

static CYIAbstractVideoPlayer::StreamingFormat StringToFormat(CYIString format)
{
    CYIAbstractVideoPlayer::StreamingFormat currentFormat = CYIAbstractVideoPlayer::StreamingFormat::HLS;

    format = format.ToUpperASCII();

    if (format == "HLS")
    {
        currentFormat = CYIAbstractVideoPlayer::StreamingFormat::HLS;
    }
    else if (format == "SMOOTH")
    {
        currentFormat = CYIAbstractVideoPlayer::StreamingFormat::Smooth;
    }
    else if (format == "MP4")
    {
        currentFormat = CYIAbstractVideoPlayer::StreamingFormat::MP4;
    }
    else if (format == "DASH")
    {
        currentFormat = CYIAbstractVideoPlayer::StreamingFormat::DASH;
    }
    else
    {
        YI_ASSERT(false, LOG_TAG, "Unsupported Format: %s", format.ToStdString().c_str());
    }

    return currentFormat;
}

void PlayerTesterApp::InitializeVideoSelector(CYISceneView *pMainComposition)
{
    {
        UrlAndFormat temp;
        temp.name = "LATEST DTV Fairplay Stream";
        temp.url = "https://dtv-latam-abc.akamaized.net/hls/live/2003011-b/dtv/dtv-latam-boomerang/master.m3u8";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::HLS;
        temp.dRMType = DRMType::IStreamPlanetFairplay;
        temp.drmScheme = CYIAbstractVideoPlayer::DRMScheme::FairPlay;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "DASH (Turner)";
        temp.url = "https://d2rghg6apqy7xc.cloudfront.net/dvp/test_dash/005/unencrypted/montereypop_16_min_2265099.mpd";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::DASH;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "DASH + PlayReady (Turner)";
        temp.url = "https://edc-test.cdn.turner.com/DASH_MontereyPop_0011/3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c000014d4/master.mpd";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::DASH;
        temp.dRMType = DRMType::PlayReadyNoConfig;
        temp.drmScheme = CYIAbstractVideoPlayer::DRMScheme::PlayReady;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "DASH + PlayReady 2.0 (Tears)";
        temp.url = "https://profficialsite.origin.mediaservices.windows.net/c51358ea-9a5e-4322-8951-897d640fdfd7/tearsofsteel_4k.ism/manifest(format=mpd-time-csf)";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::DASH;
        temp.dRMType = DRMType::MicrosoftPlayReadyTestServer;
        temp.drmScheme = CYIAbstractVideoPlayer::DRMScheme::PlayReady;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "DASH + PlayReady 2.0 (Xbox Ad)";
        temp.url = "https://profficialsite.origin.mediaservices.windows.net/9cc5e871-68ec-42c2-9fc7-fda95521f17d/dayoneplayready.ism/manifest(format=mpd-time-csf)";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::DASH;
        temp.dRMType = DRMType::MicrosoftPlayReadyTestServer;
        temp.drmScheme = CYIAbstractVideoPlayer::DRMScheme::PlayReady;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "Super Speedway (PlayReady 2.0)";
        temp.url = "http://playready.directtaps.net/smoothstreaming/SSWSS720H264PR/SuperSpeedway_720.ism/Manifest";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::Smooth;
        temp.dRMType = DRMType::MicrosoftPlayReadyTestServer;
        temp.drmScheme = CYIAbstractVideoPlayer::DRMScheme::PlayReady;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "DASH + Widevine";
        temp.url = "https://bitmovin-a.akamaihd.net/content/art-of-motion_drm/mpds/11331.mpd";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::DASH;
        temp.dRMType = DRMType::WideVineBitmovin;
        temp.drmScheme = CYIAbstractVideoPlayer::DRMScheme::WidevineModular;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "DASH + Widevine (Custom Request)";
        temp.url = "https://bitmovin-a.akamaihd.net/content/art-of-motion_drm/mpds/11331.mpd";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::DASH;
        temp.dRMType = DRMType::WideVineBitmovinCustomRequest;
        temp.drmScheme = CYIAbstractVideoPlayer::DRMScheme::WidevineModularCustomRequest;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "SCTE markers";
        temp.url = "https://s3.amazonaws.com/roku-anthony.van.alphen-test/video/portugal-vs-mexico/index.m3u8";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::HLS;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "Metadata - overide";
        temp.url = "http://adultswim-vodlive-qa.cdn.turner.com/live/blackout_testing/smp_blkout_overide-new/new/stream.m3u8";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::HLS;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "Metadata - program";
        temp.url = "http://adultswim-vodlive-qa.cdn.turner.com/live/blackout_testing/smp_blkout_program-new/new/stream.m3u8";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::HLS;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "Metadata - back2back";
        temp.url = "http://adultswim-vodlive-qa.cdn.turner.com/live/blackout_testing/smp_blkout_back2back-new/new/stream.m3u8";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::HLS;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "Metadata - BipBop";
        temp.url = "https://devstreaming-cdn.apple.com/videos/streaming/examples/bipbop_16x9/bipbop_16x9_variant.m3u8";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::HLS;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "Webinar 5";
        temp.url = "http://demo.cf.castlabs.com/media/viisights/webinars/5.mp4";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::MP4;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "Japanese Movie Trailer";
        temp.url = "https://trailers.mubi.com/1024/t-kikujiro_en_us_1280_720_1489650171.mp4";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::MP4;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "Red Bull Parkor";
        temp.url = "https://bitdash-a.akamaihd.net/content/MI201109210084_1/m3u8s/f08e80da-bf1d-4e3d-8899-f0f6155f6efa.m3u8";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::HLS;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "Sintel";
        temp.url = "https://bitdash-a.akamaihd.net/content/sintel/hls/playlist.m3u8";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::HLS;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "Big Buck Bunny";
        temp.url = "http://184.72.239.149/vod/smil:BigBuckBunny.smil/playlist.m3u8";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::HLS;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "Oceans (AES encrypted)";
        temp.url = "http://playertest.longtailvideo.com/adaptive/oceans_aes/oceans_aes.m3u8";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::HLS;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "arte";
        temp.url = "http://www.streambox.fr/playlists/test_001/stream.m3u8";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::HLS;
        temp.isErrorUrl = true;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "Bip Bop";
        temp.url = "http://devimages.apple.com/iphone/samples/bipbop/bipbopall.m3u8";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::HLS;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "Counter live stream (DASH)";
        temp.url = "http://livesim.dashif.org/livesim/testpic_2s/Manifest.mpd";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::DASH;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "Bip Bop (4x3, CEA-608)";
        temp.url = "https://devstreaming-cdn.apple.com/videos/streaming/examples/bipbop_4x3/bipbop_4x3_variant.m3u8";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::HLS;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "Satellite Stream";
        temp.url = "http://iphone-streaming.ustream.tv/uhls/9408562/streams/live/iphone/playlist.m3u8";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::HLS;
        temp.isLive = true;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        // may need local hosting
        temp.name = "Tears of Steel (Multi-BitRate)";
        temp.url = "http://www.bok.net/dash/tears_of_steel/cleartext/stream.mpd";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::DASH;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        // The following urls may cause errors
        temp.name = "Bad URL";
        temp.url = "http://notarealurl.m38u";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::MP4;
        temp.isErrorUrl = true;
        temp.isSuitableForSeekTest = false;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "Cannot Parse Error";
        temp.url = "http://192.168.1.45/Demo_Sub_Account_2/993/911/Sunrise-many-audio-tracks.m3u8";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::HLS;
        temp.isErrorUrl = true;
        temp.isSuitableForSeekTest = false;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "The operation could not be completed Error";
        temp.url = "http://coolvideos.com/plausiblevids/listview_refactor_livestream.m3u8";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::HLS;
        temp.isErrorUrl = true;
        temp.isSuitableForSeekTest = false;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "Azure Demo - Channel 1 (Live)";
        temp.url = "http://b028.wpc.azureedge.net/80B028/Samples/a38e6323-95e9-4f1f-9b38-75eba91704e4/5f2ce531-d508-49fb-8152-647eba422aec.ism/Manifest(format=m3u8-aapl-v3)";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::HLS;
        temp.isLive = true;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "Azure Demo - Channel 1 (Live)";
        temp.url = "http://b028.wpc.azureedge.net/80B028/Samples/a38e6323-95e9-4f1f-9b38-75eba91704e4/5f2ce531-d508-49fb-8152-647eba422aec.ism/Manifest(format=mpd-time-csf)";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::DASH;
        temp.isLive = true;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "Azure Demo - Channel 1 (Live)";
        temp.url = "http://b028.wpc.azureedge.net/80B028/Samples/a38e6323-95e9-4f1f-9b38-75eba91704e4/5f2ce531-d508-49fb-8152-647eba422aec.ism/Manifest";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::Smooth;
        temp.isLive = true;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "Azure Demo - Channel 2 (Live)";
        temp.url = "http://b028.wpc.azureedge.net/80B028/SampleStream/595d6b9a-d98e-4381-86a3-cb93664479c2/b722b983-af65-4bb3-950a-18dded2b7c9b.ism/Manifest(format=m3u8-aapl-v3)";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::HLS;
        temp.isLive = true;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "Azure Demo - Channel 2 (Live)";
        temp.url = "http://b028.wpc.azureedge.net/80B028/SampleStream/595d6b9a-d98e-4381-86a3-cb93664479c2/b722b983-af65-4bb3-950a-18dded2b7c9b.ism/Manifest(format=mpd-time-csf)";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::DASH;
        temp.isLive = true;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "Azure Demo - Channel 2 (Live)";
        temp.url = "http://b028.wpc.azureedge.net/80B028/SampleStream/595d6b9a-d98e-4381-86a3-cb93664479c2/b722b983-af65-4bb3-950a-18dded2b7c9b.ism/Manifest";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::Smooth;
        temp.isLive = true;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "Azure Demo - Channel 3 (AES/Live)";
        temp.url = "http://b028.wpc.azureedge.net/80B028/Samples/0e8848ca-1db7-41a3-8867-fe911144c045/d34d8807-5597-47a1-8408-52ec5fc99027.ism/Manifest(format=m3u8-aapl-v3)";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::HLS;
        temp.isLive = true;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "Azure Demo - Channel 3 (AES/Live)";
        temp.url = "http://b028.wpc.azureedge.net/80B028/Samples/0e8848ca-1db7-41a3-8867-fe911144c045/d34d8807-5597-47a1-8408-52ec5fc99027.ism/Manifest(format=mpd-time-csf)";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::DASH;
        temp.isLive = true;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "Azure Demo - Channel 3 (AES/Live)";
        temp.url = "http://b028.wpc.azureedge.net/80B028/Samples/0e8848ca-1db7-41a3-8867-fe911144c045/d34d8807-5597-47a1-8408-52ec5fc99027.ism/Manifest";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::Smooth;
        temp.isLive = true;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "Cannot Parse Error";
        temp.url = "http://vevoplaylist-live.hls.adaptive.level3.net/vevo/ch1/appleman.m3u8";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::HLS;
        temp.isErrorUrl = true;
        temp.isSuitableForSeekTest = false;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "SuperSpeedway (Clear)";
        temp.url = "http://playready.directtaps.net/smoothstreaming/SSWSS720H264/SuperSpeedway_720.ism/Manifest";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::Smooth;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "???";
        temp.url = "http://b028.wpc.azureedge.net/80B028/Samples/a38e6323-95e9-4f1f-9b38-75eba91704e4/5f2ce531-d508-49fb-8152-647eba422aec.ism/manifest";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::Smooth;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "HTTP Request Test";
        temp.url = "https://youihttptest.requestcatcher.com/PathTest1/PathTest2?QueryTest1=QueryValue1&QueryTest2=QueryValue2";
        temp.customHeaders.push_back(CYIHTTPHeader("HeaderTest1", "HeaderValue1"));
        temp.customHeaders.push_back(CYIHTTPHeader("HeaderTest2", "HeaderValue2"));

        // Add one asset to test the HTTP request. The first format which is supported is sufficient, because all formats shoudl behave identically.
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::HLS;
        bool httpTestAdded = AppendUrlIfSupported(temp);

        if (!httpTestAdded)
        {
            temp.format = CYIAbstractVideoPlayer::StreamingFormat::Smooth;
            httpTestAdded = AppendUrlIfSupported(temp);
        }

        if (!httpTestAdded)
        {
            temp.format = CYIAbstractVideoPlayer::StreamingFormat::DASH;
            httpTestAdded = AppendUrlIfSupported(temp);
        }

        if (!httpTestAdded)
        {
            temp.format = CYIAbstractVideoPlayer::StreamingFormat::MP4;
            httpTestAdded = AppendUrlIfSupported(temp);
        }
    }

    {
        UrlAndFormat temp;
        temp.name = "Invalid Local MP4.";
        temp.url = "file://" + GetAssetsPath() + "invalid_file_not_found.mp4";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::MP4;
        temp.isErrorUrl = true;
        temp.isSuitableForSeekTest = false;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "DASH Live Stream 1";
        temp.url = "https://storage.googleapis.com/shaka-live-assets/player-source.mpd";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::DASH;
        temp.isSuitableForSeekTest = false;
        temp.isLive = true;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "DASH Live Stream 2";
        temp.url = "https://akamai-axtest.akamaized.net/routes/lapd-v1-acceptance/www_c4/Manifest.mpd";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::DASH;
        temp.isSuitableForSeekTest = false;
        temp.isLive = true;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "HLS Live Stream 1";
        temp.url = "https://storage.googleapis.com/shaka-live-assets/player-source.m3u8";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::HLS;
        temp.isSuitableForSeekTest = false;
        temp.isLive = true;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "HLS Live Stream 2";
        temp.url = "https://akamai-axtest.akamaized.net/routes/lapd-v1-acceptance/www_c4/Manifest.m3u8";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::HLS;
        temp.isSuitableForSeekTest = false;
        temp.isLive = true;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "[S] DASH (Turner)";
        temp.url = "https://d2rghg6apqy7xc.cloudfront.net/dvp/test_dash/005/unencrypted/montereypop_16_min_2265099.mpd";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::DASH;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "[S] BBB Dark Truths";
        temp.url = "https://storage.googleapis.com/shaka-demo-assets/bbb-dark-truths-hls/hls.m3u8";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::HLS;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "[S] Angel One (WebVTT)";
        temp.url = "https://storage.googleapis.com/shaka-demo-assets/angel-one/dash.mpd";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::DASH;
        AppendUrlIfSupported(temp);
    }

    {
        UrlAndFormat temp;
        temp.name = "[S] Angel One";
        temp.url = "https://storage.googleapis.com/shaka-demo-assets/angel-one-hls/hls.m3u8";
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::HLS;
        AppendUrlIfSupported(temp);
    }

    {
        CYIAssetLoader *pAL = CYIFramework::GetInstance()->GetAssetLoader();
        std::shared_ptr<CYIAssetVideo> pLocalVideo = YiDynamicCast<CYIAssetVideo>(pAL->Load(CYIAssetVideo::GetClassTypeInfo(), "small.mp4", nullptr));
        YI_ASSERT(pLocalVideo, LOG_TAG, "Could not locate 'local' mp4 file.");

        UrlAndFormat temp;
        temp.name = "Local MP4";
        temp.url = pLocalVideo->GetUrl().ToString();
        temp.format = CYIAbstractVideoPlayer::StreamingFormat::MP4;
        temp.isLocalFile = true;
        temp.isSuitableForSeekTest = false;
        AppendUrlIfSupported(temp);
    }

    m_pVideoSelectorView = pMainComposition->GetNode<CYISceneView>("VideoSelector");
    YI_ASSERT(m_pVideoSelectorView, LOG_TAG, "Could not find 'VideoSelector'");
    m_pVideoSelectorView->SetIsFocusRoot(true);
    m_pVideoSelectorView->Hide();

    //now use possibleURLs to create buttons and add them to the overlay screen
    size_t items = m_possibleURLs.size();
    for (size_t i = 0; i < items; i++)
    {
        std::shared_ptr<CYIAssetViewTemplate> pTemplate = CYIViewTemplate::GetViewTemplate("PlayerTester_Button");
        std::unique_ptr<CYISceneView> pView = pTemplate->BuildView(GetMasterAppSceneManager());
        std::unique_ptr<CYIPushButtonView> pButtonView(YiDynamicCast<CYIPushButtonView>(pView.release()));
        pButtonView->SetScale(glm::vec3(0.5, 0.5, 1.0));
        pButtonView->SetButtonID(static_cast<int32_t>(i));
        pButtonView->Init();
        pButtonView->SetText(m_possibleURLs[i].name + " [" + FormatToString(m_possibleURLs[i].format) + "] : " + m_possibleURLs[i].url);
        pButtonView->ButtonClicked.Connect(*this, &PlayerTesterApp::OnURLSelected);

        m_pVideoSelectorView->AddChild(std::move(pButtonView));
    }

    OnURLSelected(0);
}

void PlayerTesterApp::UpdatePlayerStats(const CYIAbstractVideoPlayer::Statistics &stats)
{
    m_pIsLiveText->SetText(stats.isLive ? "YES" : "NO");

    // Total, video, and audio bitrate text will be updated in the listeners for the bitrate changed signals.
    m_pDefaultTotalBitrateText->SetText(CYIString::FromFloat(stats.defaultTotalBitrateKbps));
    m_pDefaultVideoBitrateText->SetText(CYIString::FromFloat(stats.defaultVideoBitrateKbps));
    m_pDefaultAudioBitrateText->SetText(CYIString::FromFloat(stats.defaultAudioBitrateKbps));

    m_pBufferLengthText->SetText(CYIString::FromFloat(stats.bufferLengthMs));
    m_pMinBufferLengthStatText->SetText(CYIString::FromFloat(stats.minimumBufferLengthMs));
    m_pFPSText->SetText(CYIString::FromFloat(stats.renderedFramesPerSecond));
}

void PlayerTesterApp::OnTotalBitrateChanged(float bitrate)
{
    m_pTotalBitrateText->SetText(CYIString::FromFloat(bitrate));
}

void PlayerTesterApp::OnVideoBitrateChanged(float bitrate)
{
    m_pVideoBitrateText->SetText(CYIString::FromFloat(bitrate));
}

void PlayerTesterApp::OnAudioBitrateChanged(float bitrate)
{
    m_pAudioBitrateText->SetText(CYIString::FromFloat(bitrate));
}

void PlayerTesterApp::UpdateBufferRate(const CYIAbstractVideoPlayer::BufferLength &bufferLength)
{
    // Not updating min buffer length here, because it is already updated in UpdatePlayerStats.
    m_pMaxBufferLengthStatText->SetText(CYIString::FromValue(bufferLength.max.count()));
}

void PlayerTesterApp::OnStartButtonPressed()
{
    m_pErrorView->GetNode<CYITextSceneNode>()->SetText("");
    ResetStatisticsLabelsToDefault();

    DisableStartButtons();
    m_pShowCCButton->Enable();
    m_pHideCCButton->Disable();

    uint64_t startTime = m_pStartTimeText->GetValue().To<uint64_t>();
    PrepareVideo(*m_pActiveFormat, startTime);
}

void PlayerTesterApp::OnStartLowResButtonPressed()
{
    CYICastLabsVideoPlayer *pCastLabsPlayer = YiDynamicCast<CYICastLabsVideoPlayer>(m_pPlayer.get());
    if (pCastLabsPlayer)
    {
        pCastLabsPlayer->SetMaxResolution(glm::ivec2(640, 480));
    }
    OnStartButtonPressed();
}

void PlayerTesterApp::OnPlayButtonPressed()
{
    m_pPlayer->Play();
}

void PlayerTesterApp::OnPauseButtonPressed()
{
    m_pPlayer->Pause();
}

void PlayerTesterApp::OnStopButtonPressed()
{
    m_pPlayer->Stop();
    m_pPlayButton->Disable();
    m_pPauseButton->Disable();
    m_pShowCCButton->Disable();
    m_pHideCCButton->Disable();
    m_pSeekForwardButton->Disable();
    m_pSeekReverseButton->Disable();
    m_pSeekButton->Disable();
    m_pMinSeekButton->Disable();
    m_pMaxSeekButton->Disable();
    m_pChangeURLButton->Enable();
    m_pErrorView->Hide();
    m_pStartButton->Enable();
    m_pCurrentFormatText->SetFocusable(true);
    m_pCurrentUrlText->SetFocusable(true);
}

void PlayerTesterApp::OnShowCCButtonPressed()
{
    m_pShowCCButton->Disable();
    m_pHideCCButton->Enable();
    if (m_pPlayer->AreClosedCaptionsTracksAvailable())
    {
        std::vector<CYIAbstractVideoPlayer::ClosedCaptionsTrackInfo> captionsTracks = m_pPlayer->GetClosedCaptionsTracks();
        uint32_t trackToSelect = captionsTracks[0].id;
        // ExoPlayer always adds a 608 track for HLS whether or not the stream contains 608 captions. So selecting the first track may do nothing on Android.
        // Here if it is HLS and there is more than one CC track we skip the first one to avoid confusion as this tester does not display or let you select a track.
        if (m_pActiveFormat->format == CYIAbstractVideoPlayer::StreamingFormat::HLS && captionsTracks.size() > 1)
        {
            trackToSelect = captionsTracks[1].id;
        }
        m_pPlayer->SelectClosedCaptionsTrack(trackToSelect);
    }
}

void PlayerTesterApp::OnHideCCButtonPressed()
{
    m_pShowCCButton->Enable();
    m_pHideCCButton->Disable();

    m_pPlayer->SelectClosedCaptionsTrack(CYIAbstractVideoPlayer::ClosedCaptionsTrackInfo::OFF_TRACK_ID);
}

void PlayerTesterApp::OnSeekButtonPressed(int32_t id)
{
    if (id == (int32_t)SeekButtonIndex::Forward)
    {
        HandleSeek(m_pPlayer->GetCurrentTimeMs() + (1000 * 10)); //10 secs
    }
    else if (id == (int32_t)SeekButtonIndex::Reverse)
    {
        uint64_t currentTimeMs = m_pPlayer->GetCurrentTimeMs();
        HandleSeek((currentTimeMs > 10000) ? currentTimeMs - (1000 * 10) : 0u); //10 secs
    }
    else if (id == (int32_t)SeekButtonIndex::General)
    {
        m_pSeekButton->Hide();
        m_pSeekText->Show();
        m_pSeekText->RequestFocus();
    }
}

void PlayerTesterApp::OnSeekTextReturnPressed()
{
    m_pSeekButton->Show();
    m_pSeekText->Hide();
    uint64_t seekSec = m_pSeekText->GetValue().To<uint64_t>();
    HandleSeek(seekSec * 1000);
}

void PlayerTesterApp::HandleSeek(uint64_t seekPositionMS)
{
    if (m_pPlayer->GetStatistics().isLive)
    {
        // If this is a live stream, then first make sure that we seek to a time within one of the seekable ranges.
        std::vector<CYIAbstractVideoPlayer::SeekableRange> seekableRanges = m_pPlayer->GetLiveSeekableRanges();

        int64_t targetTimeMs = (int64_t)seekPositionMS;
        int64_t closestBackwardSeekPosition = std::numeric_limits<int64_t>::min();
        int64_t closestForwardSeekPosition = std::numeric_limits<int64_t>::max();
        for (auto it = seekableRanges.begin(); it != seekableRanges.end(); ++it)
        {
            int64_t rangeStartTime = (int64_t)it->startTimeMs;
            int64_t rangeEndTime = (int64_t)it->endTimeMs;

            if (targetTimeMs >= rangeStartTime && targetTimeMs <= rangeEndTime)
            {
                // The target time is within the seekable range. Seek to the target time, and stop searching.
                m_pPlayer->Seek(seekPositionMS);
                return;
            }
            else if (targetTimeMs < rangeStartTime)
            {
                // Target time is before the start of the range. Earliest forward seek position is the minimum of the range end, and previously found value.
                closestForwardSeekPosition = std::min(rangeStartTime, closestForwardSeekPosition);
            }
            else // targetTimeMs > rangeEndTime
            {
                // Target time is after the end of the range. Latest backward seek position is the maximum of the range start, and previously found value.
                closestBackwardSeekPosition = std::min(rangeEndTime, closestBackwardSeekPosition);
            }
        }

        if (closestBackwardSeekPosition > std::numeric_limits<int64_t>::min())
        {
            // Prioritize seeking earlier than the target time. Seek to the closest seekable time earlier than the target time.
            YI_ASSERT(closestBackwardSeekPosition >= 0, LOG_TAG, "PlayerTesterApp::HandleSeek resulted in a negative seek time.");
            m_pPlayer->Seek((uint64_t)closestBackwardSeekPosition);
            return;
        }
        else if (closestForwardSeekPosition < std::numeric_limits<int64_t>::max())
        {
            // If no earlier time is found, seek to the closest later seekable time.
            YI_ASSERT(closestForwardSeekPosition >= 0, LOG_TAG, "PlayerTesterApp::HandleSeek resulted in a negative seek time.");
            m_pPlayer->Seek(closestForwardSeekPosition);
            return;
        }

        // If we reach here, there were no live seekable ranges. Fall out, and try a regular seek with the target time.
    }

    m_pPlayer->Seek(seekPositionMS);
}

void PlayerTesterApp::OnMaxBitrateButtonPressed()
{
    m_pMaxBitrateButton->Hide();
    m_pMaxBitrateText->Show();
    m_pMaxBitrateText->RequestFocus();
}

void PlayerTesterApp::OnMaxBitrateReturnPressed()
{
    m_pMaxBitrateButton->Show();
    m_pMaxBitrateText->Hide();
    uint64_t maxBitrate = m_pMaxBitrateText->GetValue().To<uint64_t>();
    m_pPlayer->SetMaxBitrate(maxBitrate);
}

void PlayerTesterApp::OnMinBufferLengthButtonPressed()
{
    m_pMinBufferLengthButton->Hide();
    m_pMinBufferLengthText->Show();
    m_pMinBufferLengthText->RequestFocus();
}

void PlayerTesterApp::OnMinBufferLengthReturnPressed()
{
    m_pMinBufferLengthButton->Show();
    m_pMinBufferLengthText->Hide();
    uint32_t minBufferLength = m_pMinBufferLengthText->GetValue().To<uint32_t>();
    uint32_t maxBufferLength = m_pMaxBufferLengthText->GetValue().To<uint32_t>();
    CYIAbstractVideoPlayer::BufferingInterface *pBufferingInterface = m_pPlayer->GetBufferingInterface();
    if (pBufferingInterface)
    {
        pBufferingInterface->SetBufferLength(CYIAbstractVideoPlayer::BufferLength(std::chrono::milliseconds(minBufferLength), std::chrono::milliseconds(maxBufferLength)));
    }
}

void PlayerTesterApp::OnMaxBufferLengthButtonPressed()
{
    m_pMaxBufferLengthButton->Hide();
    m_pMaxBufferLengthText->Show();
    m_pMaxBufferLengthText->RequestFocus();
}

void PlayerTesterApp::OnMaxBufferLengthReturnPressed()
{
    m_pMaxBufferLengthButton->Show();
    m_pMaxBufferLengthText->Hide();
    uint32_t minBufferLength = m_pMinBufferLengthText->GetValue().To<uint32_t>();
    uint32_t maxBufferLength = m_pMaxBufferLengthText->GetValue().To<uint32_t>();
    CYIAbstractVideoPlayer::BufferingInterface *pBufferingInterface = m_pPlayer->GetBufferingInterface();
    if (pBufferingInterface)
    {
        pBufferingInterface->SetBufferLength(CYIAbstractVideoPlayer::BufferLength(std::chrono::milliseconds(minBufferLength), std::chrono::milliseconds(maxBufferLength)));
    }
}

void PlayerTesterApp::OnStartTimeButtonPressed()
{
    m_pStartTimeButton->Hide();
    m_pStartTimeText->Show();
    m_pStartTimeText->RequestFocus();
}

void PlayerTesterApp::OnStartTimeReturnPressed()
{
    m_pStartTimeButton->Show();
    m_pStartTimeText->Hide();
    // The start time will be fetched from the the textbox when the video is prepared.
}

void PlayerTesterApp::OnMinSeekButtonPressed()
{
    std::vector<CYIAbstractVideoPlayer::SeekableRange> ranges = m_pPlayer->GetLiveSeekableRanges();

    if (!ranges.empty())
    {
        //Search the SeekableRanges to find the smallest start time
        size_t index = 0;
        uint64_t smallest = UINT64_MAX;
        for (auto it = ranges.begin(); it != ranges.end(); ++it)
        {
            if (it->startTimeMs < smallest)
            {
                index = it - ranges.begin();
                smallest = it->startTimeMs;
            }
        }

        m_pPlayer->Seek(ranges[index].startTimeMs);
    }
    else if (!m_pPlayer->GetStatistics().isLive)
    {
        m_pPlayer->Seek(0);
    }
}

void PlayerTesterApp::OnMuteButtonPressed()
{
    bool mute = !m_pPlayer->IsMuted();
    m_pPlayer->Mute(mute);
    m_pMuteButton->SetText(m_pPlayer->IsMuted() ? "Unmute" : "Mute");
}

void PlayerTesterApp::OnMaxSeekButtonPressed()
{
    std::vector<CYIAbstractVideoPlayer::SeekableRange> ranges = m_pPlayer->GetLiveSeekableRanges();

    if (!ranges.empty())
    {
        //Search the SeekableRanges to find the largest end time
        size_t index = 0;
        uint64_t largest = 0;
        for (auto it = ranges.begin(); it != ranges.end(); ++it)
        {
            if (it->endTimeMs > largest)
            {
                index = it - ranges.begin();
                largest = it->endTimeMs;
            }
        }

        m_pPlayer->Seek(ranges[index].endTimeMs);
    }
    else if (!m_pPlayer->GetStatistics().isLive)
    {
        m_pPlayer->Seek(m_pPlayer->GetDurationMs() - 1);
    }
}

void PlayerTesterApp::DisableStartButtons()
{
    m_pCurrentUrlText->SetFocusable(false);
    m_pCurrentFormatText->SetFocusable(false);
    m_pStartButton->Disable();
    m_pStartLowResButton->Disable();
    m_pChangeURLButton->Disable();
}

void PlayerTesterApp::OnAnimateButtonPressed()
{
    m_pAnimateVideoTimeline->StartForward();
}

void PlayerTesterApp::OnSwitchToMiniViewButtonPressed()
{
    if (m_playerIsMini)
    {
        m_pPlayerSurfaceMiniView->SetVideoSurface(nullptr);
        m_pPlayerSurfaceView->SetVideoSurface(m_pPlayer->GetSurface());
        m_playerIsMini = false;
    }
    else
    {
        m_pPlayerSurfaceMiniView->SetVideoSurface(m_pPlayer->GetSurface());
        m_pPlayerSurfaceView->SetVideoSurface(nullptr);
        m_playerIsMini = true;
    }
}

void PlayerTesterApp::OnURLSelected(int32_t buttonID)
{
    m_pActiveFormat = &m_possibleURLs[buttonID];
    m_pCurrentFormatText->SetValue(FormatToString(m_pActiveFormat->format));
    m_pCurrentUrlText->SetValue(m_pActiveFormat->url);
    m_pShowVideoSelectorTimeline->StartReverse();
    m_pStartButton->Enable();
    m_pStartLowResButton->Enable();
    m_pErrorView->Hide();
}

void PlayerTesterApp::OnUserAgentButtonPressed()
{
    m_pUserAgentButton->Hide();
    m_pUserAgentText->Show();
    m_pUserAgentText->RequestFocus();
}

void PlayerTesterApp::OnUserAgentReturnPressed()
{
    m_pUserAgentButton->Show();
    m_pUserAgentText->Hide();
    CYIString userAgent = m_pUserAgentText->GetValue();
    m_pPlayer->SetUserAgent(userAgent);
}

void PlayerTesterApp::ErrorOccured(CYIAbstractVideoPlayer::Error error)
{
    YI_ASSERT(error.errorCode != CYIAbstractVideoPlayer::ErrorCode::Unknown, LOG_TAG, "Error code not populated.");
    YI_LOGD(LOG_TAG, "Error: %s, Native Player Error Code: %s", error.message.GetData(), error.nativePlayerErrorCode.GetData());
    m_pErrorView->Show();
    m_pPlayButton->Disable();
    m_pPauseButton->Disable();
    m_pChangeURLButton->Enable();
    m_pShowCCButton->Disable();
    m_pHideCCButton->Disable();
    m_pErrorView->GetNode<CYITextSceneNode>()->SetText(CYIString("Message: ") + error.message + "\nNative Player Code: " + error.nativePlayerErrorCode);
}

void PlayerTesterApp::VideoPreparing()
{
    YI_LOGD(LOG_TAG, "Video Preparing");
    m_pStatusText->SetText("Preparing");
}

void PlayerTesterApp::VideoReady()
{
    m_pPlayButton->Enable();

    YI_LOGD(LOG_TAG, "Video Ready");
    m_pStatusText->SetText("Ready");
}

void PlayerTesterApp::VideoPlaying()
{
    YI_LOGD(LOG_TAG, "Video playing");
    m_pStatusText->SetText("Playing");

    m_pPlayButton->Disable();
    m_pPauseButton->Enable();
    m_pSeekForwardButton->Enable();
    m_pSeekReverseButton->Enable();
    m_pSeekButton->Enable();
    m_pMinSeekButton->Enable();
    m_pMaxSeekButton->Enable();

    CYIDevicePowerManagementBridge *pBridge = CYIDeviceBridgeLocator::GetDevicePowerManagementBridge();
    if (pBridge)
    {
        pBridge->KeepDeviceScreenOn(true);
    }
}

void PlayerTesterApp::VideoPaused()
{
    m_pPlayButton->Enable();
    m_pPauseButton->Disable();
    YI_LOGD(LOG_TAG, "Video paused");
    m_pStatusText->SetText("Paused");

    CYIDevicePowerManagementBridge *pBridge = CYIDeviceBridgeLocator::GetDevicePowerManagementBridge();
    if (pBridge)
    {
        pBridge->KeepDeviceScreenOn(false);
    }
}

void PlayerTesterApp::PlaybackComplete()
{
    YI_LOGD(LOG_TAG, "Playback complete");
    m_pStatusText->SetText("Complete");

    m_pCurrentFormatText->SetFocusable(true);
    m_pCurrentUrlText->SetFocusable(true);
    m_pStartButton->Enable();
    m_pStartLowResButton->Enable();
    m_pPlayButton->Disable();
    m_pPauseButton->Disable();
    m_pShowCCButton->Disable();
    m_pHideCCButton->Disable();
    m_pSeekForwardButton->Disable();
    m_pSeekReverseButton->Disable();
    m_pSeekButton->Disable();
    m_pMinSeekButton->Disable();
    m_pMaxSeekButton->Disable();
    m_pChangeURLButton->Enable();
}

void PlayerTesterApp::StateChanged(const CYIAbstractVideoPlayer::PlayerState &state)
{
    if (state == CYIAbstractVideoPlayer::MediaState::Unloaded)
    {
        m_pIStreamPlanetFairPlayHandler.reset();
        m_pStatusText->SetText("Unloaded");
    }
    // On Tizen changing the surface size is only supported when the media is unloaded.
#if defined(YI_TIZEN_NACL)
    state == CYIAbstractVideoPlayer::MediaState::Unloaded ? m_pSwitchToMiniViewButton->Enable() : m_pSwitchToMiniViewButton->Disable();
#endif
}

void PlayerTesterApp::CurrentTimeUpdated(uint64_t currentTimeMs)
{
    CYIString current = CYITimeConversion::TimeIntervalToString(currentTimeMs / 1000, "%hh%:%mm%:%ss%");
    m_pCurrentTimeText->SetText(current);
}

void PlayerTesterApp::VideoDurationChanged(uint64_t durationMs)
{
    CYIString duration = CYITimeConversion::TimeIntervalToString(durationMs / 1000, "%hh%:%mm%:%ss%");
    m_pDurationText->SetText(duration);
}

bool PlayerTesterApp::AppendUrlIfSupported(UrlAndFormat urlAndFormat)
{
    static bool unsupportedUrlAdded = false;
    if (m_pPlayer->SupportsFormat(urlAndFormat.format, urlAndFormat.drmScheme))
    {
        m_possibleURLs.push_back(urlAndFormat);
        return true;
    }
    else if (!unsupportedUrlAdded)
    {
        urlAndFormat.name = "Unsupported Format - Should cause an error";
        urlAndFormat.isErrorUrl = true;
        m_possibleURLs.push_back(urlAndFormat);
        unsupportedUrlAdded = true;
        return true;
    }

    return false;
}

void PlayerTesterApp::ResetStatisticsLabelsToDefault()
{
    // reset statistics labels to default
    m_pIsLiveText->SetText("NO");
    m_pTotalBitrateText->SetText(CYIString::FromFloat(-1));
    m_pVideoBitrateText->SetText(CYIString::FromFloat(-1));
    m_pAudioBitrateText->SetText(CYIString::FromFloat(-1));
    m_pDefaultTotalBitrateText->SetText(CYIString::FromFloat(-1));
    m_pDefaultVideoBitrateText->SetText(CYIString::FromFloat(-1));
    m_pDefaultAudioBitrateText->SetText(CYIString::FromFloat(-1));
    m_pBufferLengthText->SetText(CYIString::FromFloat(-1));
    m_pMinBufferLengthStatText->SetText(CYIString::FromFloat(-1));
    m_pFPSText->SetText(CYIString::FromFloat(-1));
}

bool PlayerTesterApp::HandleEvent(const std::shared_ptr<CYIEventDispatcher> &pDispatcher, CYIEvent *pEvent)
{
    YI_UNUSED(pDispatcher);

    bool handled = false;
    CYIEvent::Type type = pEvent->GetType();

    if (pEvent->IsKeyEvent())
    {
        CYIKeyEvent *pKeyEvent = dynamic_cast<CYIKeyEvent *>(pEvent);

        if (type == CYIEvent::Type::KeyDown)
        {
            switch (pKeyEvent->m_keyCode)
            {
                case CYIKeyEvent::KeyCode::MediaRewind:
                    if (m_pPlayer->GetPlayerState() == CYIAbstractVideoPlayer::MediaState::Ready)
                    {
                        OnSeekButtonPressed(static_cast<int32_t>(SeekButtonIndex::Reverse));
                    }
                    handled = true;
                    break;
                case CYIKeyEvent::KeyCode::MediaFastForward:
                    if (m_pPlayer->GetPlayerState() == CYIAbstractVideoPlayer::MediaState::Ready)
                    {
                        OnSeekButtonPressed(static_cast<int32_t>(SeekButtonIndex::Forward));
                    }
                    handled = true;
                    break;
                case CYIKeyEvent::KeyCode::MediaStop:
                    if (m_pPlayer->GetPlayerState() == CYIAbstractVideoPlayer::MediaState::Ready)
                    {
                        OnStopButtonPressed();
                    }
                    handled = true;
                    break;
                case CYIKeyEvent::KeyCode::Pause:
                case CYIKeyEvent::KeyCode::MediaPause:
                    if (m_pPlayer->GetPlayerState() == CYIAbstractVideoPlayer::PlaybackState::Playing)
                    {
                        OnPauseButtonPressed();
                    }
                    handled = true;
                    break;
                case CYIKeyEvent::KeyCode::MediaPlay:
                    if (m_pPlayer->GetPlayerState() == CYIAbstractVideoPlayer::PlaybackState::Paused)
                    {
                        OnPlayButtonPressed();
                    }
                    handled = true;
                    break;
                case CYIKeyEvent::KeyCode::MediaPlayPause:
                    if (m_pPlayer->GetPlayerState() == CYIAbstractVideoPlayer::PlaybackState::Playing)
                    {
                        OnPauseButtonPressed();
                    }
                    else if (m_pPlayer->GetPlayerState() == CYIAbstractVideoPlayer::PlaybackState::Paused)
                    {
                        OnPlayButtonPressed();
                    }
                    handled = true;
                    break;
                default:
                    break;
            }
        }
    }

    return handled;
}

void PlayerTesterApp::PrepareVideo(UrlAndFormat toPrepare, uint64_t startTime)
{
    std::unique_ptr<CYIAbstractVideoPlayer::DRMConfiguration> pDRMConfiguration;
    if (toPrepare.dRMType == DRMType::IStreamPlanetFairplay)
    {
        m_pIStreamPlanetFairPlayHandler = std::unique_ptr<IStreamPlanetFairPlayHandler>(new IStreamPlanetFairPlayHandler(m_pPlayer.get()));
        pDRMConfiguration = m_pIStreamPlanetFairPlayHandler->CreateDRMConfiguration();
    }
    else if (toPrepare.dRMType == DRMType::MicrosoftPlayReadyTestServer || toPrepare.dRMType == DRMType::PlayReadyNoConfig)
    {
        std::unique_ptr<CYIPlayReadyDRMConfiguration> pPlayReadyDRMConfiguration(new CYIPlayReadyDRMConfiguration());
        if (toPrepare.dRMType == DRMType::MicrosoftPlayReadyTestServer)
        {
            pPlayReadyDRMConfiguration->SetLicenseAcquisitionUrl(CYIUrl("https://test.playready.microsoft.com/service/rightsmanager.asmx?cfg=(persist:false,sl:150)"));
        }
        pDRMConfiguration = std::move(pPlayReadyDRMConfiguration);
#if defined(YI_TIZEN_NACL)
        // On Tizen DRM may need to be reconfigured when restoring from the background. It won't expire for us but just set the same DRM configuration information
        // to ensure the restore works properly with a new DRM config. For non-drm content the restore will just work with previous values.
        CYITizenNaClVideoPlayer *pTizenPlayer = YiDynamicCast<CYITizenNaClVideoPlayer>(m_pPlayer.get());
        if (pTizenPlayer)
        {
            DRMType drmType = toPrepare.dRMType;
            pTizenPlayer->SetPlaybackRestoreConfigurationProvider([this, drmType]() {
                CYITizenNaClVideoPlayer::PlaybackRestoreConfiguration restoreConfiguration;
                std::unique_ptr<CYIPlayReadyDRMConfiguration> pPlayReadyDRMConfiguration(new CYIPlayReadyDRMConfiguration());
                if (drmType == DRMType::MicrosoftPlayReadyTestServer)
                {
                    pPlayReadyDRMConfiguration->SetLicenseAcquisitionUrl(CYIUrl("https://test.playready.microsoft.com/service/rightsmanager.asmx?cfg=(persist:false,sl:150)"));
                }
                restoreConfiguration.pDRMConfiguration = std::move(pPlayReadyDRMConfiguration);
                return restoreConfiguration;
            });
        }
#endif
    }
    else if (toPrepare.dRMType == DRMType::WideVineBitmovin)
    {
        std::unique_ptr<CYIWidevineModularDRMConfiguration> pWidevineDRMConfiguration(new CYIWidevineModularDRMConfiguration());
        pWidevineDRMConfiguration->SetLicenseAcquisitionUrl(CYIUrl("https://widevine-proxy.appspot.com/proxy"));
        pDRMConfiguration = std::move(pWidevineDRMConfiguration);
    }
    else if (toPrepare.dRMType == DRMType::WideVineBitmovinCustomRequest)
    {
        std::unique_ptr<CYIWidevineModularCustomRequestDRMConfiguration> pWidevineDRMCustomRequestConfiguration(new CYIWidevineModularCustomRequestDRMConfiguration());
        pWidevineDRMCustomRequestConfiguration->SetLicenseAcquisitionUrl(CYIUrl("https://notagoodurl.com"));

        auto pWidevineDRMCustomRequestConfigurationRaw = pWidevineDRMCustomRequestConfiguration.get();
        pWidevineDRMCustomRequestConfiguration->DRMPostRequestAvailable.Connect([pWidevineDRMCustomRequestConfigurationRaw](const CYIString &url, const std::vector<char> &postData, const std::vector<std::pair<CYIString, CYIString>> &headers) {
            auto pRequest = std::shared_ptr<CYIHTTPRequest>(new CYIHTTPRequest());

            // Correct the license URL here
            pRequest->SetURL(CYIUrl("https://widevine-proxy.appspot.com/proxy"));
            pRequest->SetNetworkTimeoutMs(5000);
            pRequest->SetConnectionTimeoutMs(5000);
            pRequest->SetMethod(CYIHTTPRequest::Method::POST);

            pRequest->SetPostData(postData);

            for (auto it = headers.begin(); it != headers.end(); ++it)
            {
                pRequest->AddHeader(it->first, it->second);
            }

            pRequest->NotifyResponse.Connect([pWidevineDRMCustomRequestConfigurationRaw](const std::shared_ptr<CYIHTTPRequest> &, const std::shared_ptr<CYIHTTPResponse> &httpResponse, bool) {
                pWidevineDRMCustomRequestConfigurationRaw->NotifySuccess(httpResponse->GetRawData());
            });
            pRequest->NotifyError.Connect([pWidevineDRMCustomRequestConfigurationRaw](const std::shared_ptr<CYIHTTPRequest> &, const CYIHTTPService::HTTPStatusCode, const CYIString &errorMessage) {
                pWidevineDRMCustomRequestConfigurationRaw->NotifyFailure();
            });

            CYIHTTPService::GetInstance()->EnqueueRequest(pRequest);
        });

        pDRMConfiguration = std::move(pWidevineDRMCustomRequestConfiguration);
    }

    CYIAbstractVideoPlayer::VideoRequestHTTPHeadersInterface *pCustomHeadersInterface = m_pPlayer->GetVideoRequestHTTPHeadersInterface();
    if (pCustomHeadersInterface)
    {
        pCustomHeadersInterface->SetHeaders(toPrepare.customHeaders);
    }

    m_pPlayer->Prepare(CYIUrl(toPrepare.url), toPrepare.format, CYIAbstractVideoPlayer::PlaybackState::Paused, std::move(pDRMConfiguration), startTime);
}
