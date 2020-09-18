#include "player/YiVideojsVideoPlayer.h"

#include "player/YiVideojsVideoPlayerPriv.h"
#include "player/YiVideojsVideoSurface.h"

#include <platform/YiWebBridgeLocator.h>
#include <player/YiPlayReadyDRMConfiguration.h>
#include <player/YiVideoPlayerStateManager.h>
#include <player/YiWidevineModularDRMConfiguration.h>

#define LOG_TAG "CYIVideojsVideoPlayer"

YI_TYPE_DEF(CYIVideojsVideoPlayer, CYIAbstractVideoPlayer);

static const char *VIDEO_PLAYER_CLASS_NAME = "CYIVideojsVideoPlayer";
static const char *VIDEO_PLAYER_INSTANCE_ACCESSOR_NAME = "getInstance";
static const double BITRATE_KBPS_SCALE = 1000.0;

CYIString StreamFormatToString(CYIAbstractVideoPlayer::StreamingFormat streamFormat)
{
    switch (streamFormat)
    {
        case CYIAbstractVideoPlayer::StreamingFormat::HLS:
        {
            return "HLS";
        }
        case CYIAbstractVideoPlayer::StreamingFormat::Smooth:
        {
            return "Smooth";
        }
        case CYIAbstractVideoPlayer::StreamingFormat::DASH:
        {
            return "DASH";
        }
        case CYIAbstractVideoPlayer::StreamingFormat::MP4:
        {
            return "MP4";
        }
    }

    return CYIString::EmptyString();
}

CYIString DRMSchemeToString(CYIAbstractVideoPlayer::DRMScheme drmScheme)
{
    switch (drmScheme)
    {
        case CYIAbstractVideoPlayer::DRMScheme::None:
        {
            return "None";
        }
        case CYIAbstractVideoPlayer::DRMScheme::FairPlay:
        {
            return "FairPlay";
        }
        case CYIAbstractVideoPlayer::DRMScheme::PlayReady:
        {
            return "PlayReady";
        }
        case CYIAbstractVideoPlayer::DRMScheme::WidevineModular:
        case CYIAbstractVideoPlayer::DRMScheme::WidevineModularCustomRequest:
        {
            return "Widevine";
        }
    }

    return CYIString::EmptyString();
}

CYIString CYIVideojsVideoPlayerPriv::PlayerStateToString(PlayerState state)
{
    switch (state)
    {
        case PlayerState::Uninitialized:
        {
            return "Uninitialized";
        }
        case PlayerState::Initialized:
        {
            return "Initialized";
        }
        case PlayerState::Loading:
        {
            return "Loading";
        }
        case PlayerState::Loaded:
        {
            return "Loaded";
        }
        case PlayerState::Paused:
        {
            return "Paused";
        }
        case PlayerState::Playing:
        {
            return "Playing";
        }
        case PlayerState::Complete:
        {
            return "Complete";
        }
    }

    return CYIString::EmptyString();
}

CYIVideojsVideoPlayerPriv::CYIVideojsVideoPlayerPriv(CYIVideojsVideoPlayer *pPub, yi::rapidjson::Document &&playerConfiguration)
    : m_messageHandlersRegistered(false)
    , m_stateBeforeBuffering(CYIAbstractVideoPlayer::PlaybackState::Paused)
    , m_currentTimeMs(0)
    , m_durationMs(0)
    , m_buffering(false)
    , m_isLive(false)
    , m_initialAudioBitrateKbps(-1.0f)
    , m_currentAudioBitrateKbps(-1.0f)
    , m_initialVideoBitrateKbps(-1.0f)
    , m_currentVideoBitrateKbps(-1.0f)
    , m_initialTotalBitrateKbps(-1.0f)
    , m_currentTotalBitrateKbps(-1.0f)
    , m_bufferLengthMs(-1.0f)
    , m_playerConfiguration(std::move(playerConfiguration))
    , m_bitrateChangedEventHandlerId(0)
    , m_bufferingStateChangedEventHandlerId(0)
    , m_liveStatusEventHandlerId(0)
    , m_playerErrorEventHandlerId(0)
    , m_audioTracksChangedEventHandlerId(0)
    , m_videoDurationChangedEventHandlerId(0)
    , m_videoTimeChangedEventHandlerId(0)
    , m_stateChangedEventHandlerId(0)
    , m_textTracksChangedEventHandlerId(0)
    , m_metadataAvailableEventHandlerId(0)
    , m_pPub(pPub)
{
    RegisterEventHandlers();
}

CYIVideojsVideoPlayerPriv::~CYIVideojsVideoPlayerPriv()
{
    DestroyPlayerInstance();
    UnregisterEventHandlers();
}

void CYIVideojsVideoPlayerPriv::SetVideoRectangle(const YI_RECT_REL &videoRectangle)
{
    static const char *FUNCTION_NAME = "setVideoRectangle";

    if(videoRectangle == m_previousVideoRectangle) {
        return;
    }

    m_previousVideoRectangle = videoRectangle;

    yi::rapidjson::Document command(yi::rapidjson::kObjectType);
    yi::rapidjson::MemoryPoolAllocator<yi::rapidjson::CrtAllocator> &allocator = command.GetAllocator();

    yi::rapidjson::Value arguments(yi::rapidjson::kArrayType);
    arguments.PushBack(videoRectangle.x, allocator);
    arguments.PushBack(videoRectangle.y, allocator);
    arguments.PushBack(videoRectangle.width, allocator);
    arguments.PushBack(videoRectangle.height, allocator);

    bool messageSent = false;
    CYIWebMessagingBridge::FutureResponse futureResponse = CallPlayerInstanceFunction(std::move(command), FUNCTION_NAME, std::move(arguments), &messageSent);

    if (!messageSent)
    {
        YI_LOGE(LOG_TAG, "Failed to invoke %s function.", FUNCTION_NAME);
    }
    else
    {
        bool valueAssigned = false;
        CYIWebMessagingBridge::Response response = futureResponse.Take(CYIWebMessagingBridge::DEFAULT_RESPONSE_TIMEOUT_MS, &valueAssigned);

        if (!valueAssigned)
        {
            YI_LOGE(LOG_TAG, "SetVideoRectangle did not receive a response from the web messaging bridge!");
        }
        else if (response.HasError())
        {
            YI_LOGE(LOG_TAG, "%s", response.GetError()->GetMessage().GetData());
        }
    }
}

void CYIVideojsVideoPlayerPriv::Init()
{
    CreatePlayerInstance();
    InitializePlayerInstance();
}

void CYIVideojsVideoPlayerPriv::CreatePlayerInstance()
{
    static const char *FUNCTION_NAME = "createInstance";

    yi::rapidjson::Document command(yi::rapidjson::kObjectType);
    yi::rapidjson::MemoryPoolAllocator<yi::rapidjson::CrtAllocator> &allocator = command.GetAllocator();

    yi::rapidjson::Value arguments(yi::rapidjson::kArrayType);

    arguments.PushBack(m_playerConfiguration, allocator);

    bool messageSent = false;
    CYIWebMessagingBridge::FutureResponse futureResponse = CallStaticPlayerFunction(std::move(command), FUNCTION_NAME, std::move(arguments), &messageSent);

    YI_ASSERT(messageSent, LOG_TAG, "Failed to invoke %s function.", FUNCTION_NAME);

    bool valueAssigned = false;
    CYIWebMessagingBridge::Response response = futureResponse.Take(CYIWebMessagingBridge::DEFAULT_RESPONSE_TIMEOUT_MS, &valueAssigned);

    YI_ASSERT(valueAssigned, LOG_TAG, "Failed to create Video.js video player instance, no response received from web messaging bridge!");
    YI_ASSERT(!response.HasError(), LOG_TAG, "%s", response.GetError()->GetMessage().GetData());
}

void CYIVideojsVideoPlayerPriv::InitializePlayerInstance()
{
    static const char *FUNCTION_NAME = "initialize";

    yi::rapidjson::Document command(yi::rapidjson::kObjectType);

    yi::rapidjson::Value arguments(yi::rapidjson::kArrayType);

    bool messageSent = false;
    CYIWebMessagingBridge::FutureResponse futureResponse = CallPlayerInstanceFunction(std::move(command), FUNCTION_NAME, std::move(arguments), &messageSent);

    YI_ASSERT(messageSent, LOG_TAG, "Failed to invoke %s function.", FUNCTION_NAME);

    bool valueAssigned = false;
    CYIWebMessagingBridge::Response response = futureResponse.Take(CYIWebMessagingBridge::DEFAULT_RESPONSE_TIMEOUT_MS, &valueAssigned);

    YI_ASSERT(valueAssigned, LOG_TAG, "Failed to initialize Video.js video player instance, no response received from web messaging bridge!");
    YI_ASSERT(!response.HasError(), LOG_TAG, "%s", response.GetError()->GetMessage().GetData());
}

void CYIVideojsVideoPlayerPriv::DestroyPlayerInstance()
{
    static const char *FUNCTION_NAME = "destroy";

    CallPlayerInstanceFunction(yi::rapidjson::Document(), FUNCTION_NAME);
}

bool CYIVideojsVideoPlayerPriv::ConvertValueToTrackInfo(const yi::rapidjson::Value &trackValue, CYIAbstractVideoPlayer::TrackInfo &trackData)
{
    static const char *TRACK_ID_ATTRIBUTE_NAME = "id";
    static const char *TRACK_TITLE_ATTRIBUTE_NAME = "title";
    static const char *TRACK_LANGUAGE_ATTRIBUTE_NAME = "language";

    if (!trackValue.IsObject())
    {
        YI_LOGE(LOG_TAG, "ConvertValueToTrackInfo encountered an invalid track value, expected object, received %s.", CYIRapidJSONUtility::TypeToString(trackValue.GetType()).GetData());
        return false;
    }

    CYIParsingError trackIdParsingError;

    int32_t id = 0;

    CYIRapidJSONUtility::GetIntegerField(&trackValue, TRACK_ID_ATTRIBUTE_NAME, &id, trackIdParsingError);

    if (trackIdParsingError.HasError())
    {
        YI_LOGE(LOG_TAG, "ConvertValueToTrackInfo encountered an invalid integer value for track '%s'. JSON string for track value: %s", TRACK_ID_ATTRIBUTE_NAME, CYIRapidJSONUtility::CreateStringFromValue(trackValue).GetData());
        return false;
    }

    CYIParsingError trackTitleParsingError;

    CYIString title;

    CYIRapidJSONUtility::GetStringField(&trackValue, TRACK_TITLE_ATTRIBUTE_NAME, title, trackTitleParsingError);

    if (trackTitleParsingError.HasError())
    {
        if (trackTitleParsingError.GetParsingErrorCode() == CYIParsingError::ErrorType::DataFieldMissing)
        {
            YI_LOGW(LOG_TAG, "ConvertValueToTrackInfo track value is missing '%s' attribute!", TRACK_TITLE_ATTRIBUTE_NAME);
        }
        else
        {
            YI_LOGE(LOG_TAG, "ConvertValueToTrackInfo encountered an invalid string value for track '%s'. JSON string for track value: %s", TRACK_TITLE_ATTRIBUTE_NAME, CYIRapidJSONUtility::CreateStringFromValue(trackValue).GetData());
            return false;
        }
    }

    CYIParsingError trackLanguageParsingError;

    CYIString language;

    CYIRapidJSONUtility::GetStringField(&trackValue, TRACK_LANGUAGE_ATTRIBUTE_NAME, language, trackLanguageParsingError);

    if (trackLanguageParsingError.HasError())
    {
        YI_LOGE(LOG_TAG, "ConvertValueToTrackInfo encountered an invalid string value for track '%s'. JSON string for track value: %s", TRACK_LANGUAGE_ATTRIBUTE_NAME, CYIRapidJSONUtility::CreateStringFromValue(trackValue).GetData());
        return false;
    }

    if (id >= 0)
    {
        trackData.id = static_cast<uint32_t>(id);
        trackData.name = title;
        trackData.language = language;
    }
    else
    {
        YI_LOGW(LOG_TAG, "ConvertValueToTrackInfo encountered an invalid track id: %d", id);
        return false;
    }

    return true;
}

uint64_t CYIVideojsVideoPlayerPriv::RegisterEventHandler(const CYIString &eventName, CYIWebMessagingBridge::EventCallback &&eventCallback)
{
    yi::rapidjson::Document filter(yi::rapidjson::kObjectType);
    yi::rapidjson::MemoryPoolAllocator<yi::rapidjson::CrtAllocator> &allocator = filter.GetAllocator();

    yi::rapidjson::Value contextNameValue(VIDEO_PLAYER_CLASS_NAME, allocator);
    filter.AddMember(yi::rapidjson::StringRef(CYIWebMessagingBridge::EVENT_CONTEXT_ATTRIBUTE_NAME), contextNameValue, allocator);

    yi::rapidjson::Value eventNameValue(eventName.GetData(), allocator);
    filter.AddMember(yi::rapidjson::StringRef(CYIWebMessagingBridge::EVENT_NAME_ATTRIBUTE_NAME), eventNameValue, allocator);

    return CYIWebBridgeLocator::GetWebMessagingBridge()->RegisterEventHandler(std::move(filter), std::move(eventCallback));
}

void CYIVideojsVideoPlayerPriv::UnregisterEventHandler(uint64_t &eventHandlerId)
{
    CYIWebBridgeLocator::GetWebMessagingBridge()->UnregisterEventHandler(eventHandlerId);
    eventHandlerId = 0;
}

void CYIVideojsVideoPlayerPriv::RegisterEventHandlers()
{
    if (m_messageHandlersRegistered)
    {
        return;
    }

    m_bitrateChangedEventHandlerId = RegisterEventHandler("bitrateChanged", std::bind(&CYIVideojsVideoPlayerPriv::OnBitrateChanged, this, std::placeholders::_1));
    m_bufferingStateChangedEventHandlerId = RegisterEventHandler("bufferingStateChanged", std::bind(&CYIVideojsVideoPlayerPriv::OnBufferingStateChanged, this, std::placeholders::_1));
    m_liveStatusEventHandlerId = RegisterEventHandler("liveStatus", std::bind(&CYIVideojsVideoPlayerPriv::OnLiveStatusUpdated, this, std::placeholders::_1));
    m_playerErrorEventHandlerId = RegisterEventHandler("playerError", std::bind(&CYIVideojsVideoPlayerPriv::OnPlayerErrorThrown, this, std::placeholders::_1));
    m_audioTracksChangedEventHandlerId = RegisterEventHandler("audioTracksChanged", std::bind(&CYIVideojsVideoPlayerPriv::OnAudioTracksChanged, this, std::placeholders::_1));
    m_videoDurationChangedEventHandlerId = RegisterEventHandler("videoDurationChanged", std::bind(&CYIVideojsVideoPlayerPriv::OnVideoDurationChanged, this, std::placeholders::_1));
    m_videoTimeChangedEventHandlerId = RegisterEventHandler("videoTimeChanged", std::bind(&CYIVideojsVideoPlayerPriv::OnVideoTimeChanged, this, std::placeholders::_1));
    m_stateChangedEventHandlerId = RegisterEventHandler("stateChanged", std::bind(&CYIVideojsVideoPlayerPriv::OnStateChanged, this, std::placeholders::_1));
    m_textTracksChangedEventHandlerId = RegisterEventHandler("textTracksChanged", std::bind(&CYIVideojsVideoPlayerPriv::OnTextTracksChanged, this, std::placeholders::_1));
    m_metadataAvailableEventHandlerId = RegisterEventHandler("metadataAvailable", std::bind(&CYIVideojsVideoPlayerPriv::OnMetadataAvailable, this, std::placeholders::_1));

    m_messageHandlersRegistered = true;
}

void CYIVideojsVideoPlayerPriv::UnregisterEventHandlers()
{
    if (!m_messageHandlersRegistered)
    {
        return;
    }

    UnregisterEventHandler(m_bitrateChangedEventHandlerId);
    UnregisterEventHandler(m_bufferingStateChangedEventHandlerId);
    UnregisterEventHandler(m_liveStatusEventHandlerId);
    UnregisterEventHandler(m_playerErrorEventHandlerId);
    UnregisterEventHandler(m_audioTracksChangedEventHandlerId);
    UnregisterEventHandler(m_videoDurationChangedEventHandlerId);
    UnregisterEventHandler(m_videoTimeChangedEventHandlerId);
    UnregisterEventHandler(m_stateChangedEventHandlerId);
    UnregisterEventHandler(m_textTracksChangedEventHandlerId);
    UnregisterEventHandler(m_metadataAvailableEventHandlerId);

    m_messageHandlersRegistered = false;
}

CYIWebMessagingBridge::FutureResponse CYIVideojsVideoPlayerPriv::CallStaticPlayerFunction(yi::rapidjson::Document &&message, const CYIString &functionName, yi::rapidjson::Value &&playerFunctionArgumentsValue, bool *pMessageSent) const
{
    return CYIWebBridgeLocator::GetWebMessagingBridge()->CallStaticFunctionWithArgs(std::move(message), VIDEO_PLAYER_CLASS_NAME, functionName, std::move(playerFunctionArgumentsValue), pMessageSent);
}

CYIWebMessagingBridge::FutureResponse CYIVideojsVideoPlayerPriv::CallPlayerInstanceFunction(yi::rapidjson::Document &&message, const CYIString &functionName, yi::rapidjson::Value &&playerFunctionArgumentsValue, bool *pMessageSent) const
{
    return CYIWebBridgeLocator::GetWebMessagingBridge()->CallInstanceFunctionWithArgs(std::move(message), VIDEO_PLAYER_CLASS_NAME, VIDEO_PLAYER_INSTANCE_ACCESSOR_NAME, functionName, std::move(playerFunctionArgumentsValue), yi::rapidjson::Value(yi::rapidjson::kArrayType), pMessageSent);
}

void CYIVideojsVideoPlayerPriv::OnBitrateChanged(const yi::rapidjson::Value &eventValue)
{
    static const char *INITIAL_AUDIO_BITRATE_ATTRIBUTE_NAME = "initialAudioBitrateKbps";
    static const char *CURRENT_AUDIO_BITRATE_ATTRIBUTE_NAME = "currentAudioBitrateKbps";
    static const char *INITIAL_VIDEO_BITRATE_ATTRIBUTE_NAME = "initialVideoBitrateKbps";
    static const char *CURRENT_VIDEO_BITRATE_ATTRIBUTE_NAME = "currentVideoBitrateKbps";
    static const char *INITIAL_TOTAL_BITRATE_ATTRIBUTE_NAME = "initialTotalBitrateKbps";
    static const char *CURRENT_TOTAL_BITRATE_ATTRIBUTE_NAME = "currentTotalBitrateKbps";

    if (!eventValue.IsObject())
    {
        YI_LOGE(LOG_TAG, "OnBitrateChanged encountered an invalid event value, expected object, received %s. JSON string for event: %s", CYIRapidJSONUtility::TypeToString(eventValue.GetType()).GetData(), CYIRapidJSONUtility::CreateStringFromValue(eventValue).GetData());
        return;
    }

    if (!eventValue.HasMember(CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME))
    {
        YI_LOGE(LOG_TAG, "OnBitrateChanged event value is missing '%s' attribute!", CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME);
        return;
    }

    const yi::rapidjson::Value &eventDataValue = eventValue[CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME];

    if (!eventDataValue.IsObject())
    {
        YI_LOGE(LOG_TAG, "OnBitrateChanged expected an object type for event '%s', received %s. JSON string for event '%s': %s", CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME, CYIRapidJSONUtility::TypeToString(eventDataValue.GetType()).GetData(), CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME, CYIRapidJSONUtility::CreateStringFromValue(eventDataValue).GetData());
        return;
    }

    if (eventDataValue.HasMember(INITIAL_AUDIO_BITRATE_ATTRIBUTE_NAME) || eventDataValue.HasMember(CURRENT_AUDIO_BITRATE_ATTRIBUTE_NAME))
    {
        CYIParsingError initialAudioBitrateParsingError;
        float initialAudioBitrateKbps = -1;

        CYIRapidJSONUtility::GetFloatField(&eventDataValue, INITIAL_AUDIO_BITRATE_ATTRIBUTE_NAME, &initialAudioBitrateKbps, initialAudioBitrateParsingError);

        if (initialAudioBitrateParsingError.HasError())
        {
            YI_LOGE(LOG_TAG, "OnBitrateChanged encountered an invalid float value for event data '%s'. JSON string for event data: %s", INITIAL_AUDIO_BITRATE_ATTRIBUTE_NAME, CYIRapidJSONUtility::CreateStringFromValue(eventDataValue).GetData());
            return;
        }

        CYIParsingError currentAudioBitrateParsingError;
        float currentAudioBitrateKbps = -1;

        CYIRapidJSONUtility::GetFloatField(&eventDataValue, CURRENT_AUDIO_BITRATE_ATTRIBUTE_NAME, &m_currentAudioBitrateKbps, currentAudioBitrateParsingError);

        if (currentAudioBitrateParsingError.HasError())
        {
            YI_LOGE(LOG_TAG, "OnBitrateChanged encountered an invalid float value for event data '%s'. JSON string for event data: %s", CURRENT_AUDIO_BITRATE_ATTRIBUTE_NAME, CYIRapidJSONUtility::CreateStringFromValue(eventDataValue).GetData());
            return;
        }

        float previousAudioBitrateKbps = m_currentAudioBitrateKbps;

        m_initialAudioBitrateKbps = initialAudioBitrateKbps;
        m_currentAudioBitrateKbps = currentAudioBitrateKbps;

        if (!YI_FLOAT_EQUAL(previousAudioBitrateKbps, m_currentAudioBitrateKbps))
        {
            m_pPub->AudioBitrateChanged(m_currentAudioBitrateKbps);
        }
    }

    if (eventDataValue.HasMember(INITIAL_VIDEO_BITRATE_ATTRIBUTE_NAME) || eventDataValue.HasMember(CURRENT_VIDEO_BITRATE_ATTRIBUTE_NAME))
    {
        CYIParsingError initialVideoBitrateParsingError;
        float initialVideoBitrateKbps = -1;

        CYIRapidJSONUtility::GetFloatField(&eventDataValue, INITIAL_VIDEO_BITRATE_ATTRIBUTE_NAME, &initialVideoBitrateKbps, initialVideoBitrateParsingError);

        if (initialVideoBitrateParsingError.HasError())
        {
            YI_LOGE(LOG_TAG, "OnBitrateChanged encountered an invalid float value for event data '%s'. JSON string for event data: %s", INITIAL_VIDEO_BITRATE_ATTRIBUTE_NAME, CYIRapidJSONUtility::CreateStringFromValue(eventDataValue).GetData());
            return;
        }

        CYIParsingError currentVideoBitrateParsingError;
        float currentVideoBitrateKbps = -1;

        CYIRapidJSONUtility::GetFloatField(&eventDataValue, CURRENT_VIDEO_BITRATE_ATTRIBUTE_NAME, &currentVideoBitrateKbps, currentVideoBitrateParsingError);

        if (currentVideoBitrateParsingError.HasError())
        {
            YI_LOGE(LOG_TAG, "OnBitrateChanged encountered an invalid float value for event data '%s'. JSON string for event data: %s", CURRENT_VIDEO_BITRATE_ATTRIBUTE_NAME, CYIRapidJSONUtility::CreateStringFromValue(eventDataValue).GetData());
            return;
        }

        float previousVideoBitrateKbps = m_currentVideoBitrateKbps;

        m_initialVideoBitrateKbps = initialVideoBitrateKbps;
        m_currentVideoBitrateKbps = currentVideoBitrateKbps;

        if (!YI_FLOAT_EQUAL(previousVideoBitrateKbps, m_currentVideoBitrateKbps))
        {
            m_pPub->VideoBitrateChanged(m_currentVideoBitrateKbps);
        }
    }

    if (eventDataValue.HasMember(INITIAL_TOTAL_BITRATE_ATTRIBUTE_NAME) || eventDataValue.HasMember(CURRENT_TOTAL_BITRATE_ATTRIBUTE_NAME))
    {
        CYIParsingError initialTotalBitrateParsingError;
        float initialTotalBitrateKbps = -1;

        CYIRapidJSONUtility::GetFloatField(&eventDataValue, INITIAL_TOTAL_BITRATE_ATTRIBUTE_NAME, &initialTotalBitrateKbps, initialTotalBitrateParsingError);

        if (initialTotalBitrateParsingError.HasError())
        {
            YI_LOGE(LOG_TAG, "OnBitrateChanged encountered an invalid float value for event data '%s'. JSON string for event data: %s", INITIAL_TOTAL_BITRATE_ATTRIBUTE_NAME, CYIRapidJSONUtility::CreateStringFromValue(eventDataValue).GetData());
            return;
        }

        CYIParsingError currentTotalBitrateParsingError;
        float currentTotalBitrateKbps = -1;

        CYIRapidJSONUtility::GetFloatField(&eventDataValue, CURRENT_TOTAL_BITRATE_ATTRIBUTE_NAME, &currentTotalBitrateKbps, currentTotalBitrateParsingError);

        if (currentTotalBitrateParsingError.HasError())
        {
            YI_LOGE(LOG_TAG, "OnBitrateChanged encountered an invalid float value for event data '%s'. JSON string for event data: %s", CURRENT_TOTAL_BITRATE_ATTRIBUTE_NAME, CYIRapidJSONUtility::CreateStringFromValue(eventDataValue).GetData());
            return;
        }

        float previousTotalBitrateKbps = m_currentTotalBitrateKbps;

        m_initialTotalBitrateKbps = initialTotalBitrateKbps;
        m_currentTotalBitrateKbps = currentTotalBitrateKbps;

        if (!YI_FLOAT_EQUAL(previousTotalBitrateKbps, m_currentTotalBitrateKbps))
        {
            m_pPub->TotalBitrateChanged(m_currentTotalBitrateKbps);
        }
    }
}

void CYIVideojsVideoPlayerPriv::OnBufferingStateChanged(const yi::rapidjson::Value &eventValue)
{
    if (!eventValue.IsObject())
    {
        YI_LOGE(LOG_TAG, "OnBufferingStateChanged encountered an invalid event value, expected object, received %s. JSON string for event: %s", CYIRapidJSONUtility::TypeToString(eventValue.GetType()).GetData(), CYIRapidJSONUtility::CreateStringFromValue(eventValue).GetData());
        return;
    }

    CYIParsingError parsingError;

    CYIRapidJSONUtility::GetBooleanField(&eventValue, CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME, &m_buffering, parsingError);

    if (parsingError.HasError())
    {
        YI_LOGE(LOG_TAG, "OnBufferingStateChanged event value is does not contain a valid boolean value for '%s'. JSON string for event value: %s", CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME, CYIRapidJSONUtility::CreateStringFromValue(eventValue).GetData());
        return;
    }

    if (m_buffering)
    {
        CYIAbstractVideoPlayer::PlayerState currentState = m_pPub->GetPlayerState();
        if (currentState.mediaState == CYIAbstractVideoPlayer::MediaState::Ready && currentState.playbackState != CYIAbstractVideoPlayer::PlaybackState::Buffering)
        {
            m_stateBeforeBuffering = currentState.playbackState;
            m_pPub->m_pStateManager->TransitionToPlaybackBuffering();
        }
    }
    else
    {
        if (m_pPub->GetPlayerState() == CYIAbstractVideoPlayer::PlaybackState::Buffering)
        {
            if (m_stateBeforeBuffering == CYIAbstractVideoPlayer::PlaybackState::Playing)
            {
                m_pPub->m_pStateManager->TransitionToPlaybackPlaying();
            }
            else if (m_stateBeforeBuffering == CYIAbstractVideoPlayer::PlaybackState::Paused)
            {
                m_pPub->m_pStateManager->TransitionToPlaybackPaused();
            }
        }
    }
}

void CYIVideojsVideoPlayerPriv::OnLiveStatusUpdated(const yi::rapidjson::Value &eventValue)
{
    if (!eventValue.IsObject())
    {
        YI_LOGE(LOG_TAG, "OnLiveStatusUpdated encountered an invalid event value, expected object, received %s. JSON string for event: %s", CYIRapidJSONUtility::TypeToString(eventValue.GetType()).GetData(), CYIRapidJSONUtility::CreateStringFromValue(eventValue).GetData());
        return;
    }

    CYIParsingError parsingError;

    CYIRapidJSONUtility::GetBooleanField(&eventValue, CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME, &m_isLive, parsingError);

    if (parsingError.HasError())
    {
        YI_LOGE(LOG_TAG, "OnLiveStatusUpdated event value is does not contain a valid boolean value for '%s'. JSON string for event value: %s", CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME, CYIRapidJSONUtility::CreateStringFromValue(eventValue).GetData());
        return;
    }
}

void CYIVideojsVideoPlayerPriv::OnPlayerErrorThrown(const yi::rapidjson::Value &eventValue)
{
    static const char *ERROR_CODE_ATTRIBUTE_NAME = "code";
    static const CYIString DEFAULT_ERROR_MESSAGE("Unknown error.");

    CYIString message;
    CYIString code;

    if (!eventValue.IsObject())
    {
        YI_LOGE(LOG_TAG, "OnPlayerErrorThrown encountered an invalid event value, expected object, received %s. JSON string for event: %s", CYIRapidJSONUtility::TypeToString(eventValue.GetType()).GetData(), CYIRapidJSONUtility::CreateStringFromValue(eventValue).GetData());
    }
    else if (!eventValue.HasMember(CYIWebMessagingBridge::ERROR_ATTRIBUTE_NAME))
    {
        YI_LOGE(LOG_TAG, "OnPlayerErrorThrown event value is missing '%s' attribute!", CYIWebMessagingBridge::ERROR_ATTRIBUTE_NAME);
    }
    else
    {
        const yi::rapidjson::Value &eventErrorValue = eventValue[CYIWebMessagingBridge::ERROR_ATTRIBUTE_NAME];

        if (!eventErrorValue.IsObject())
        {
            YI_LOGE(LOG_TAG, "OnPlayerErrorThrown expected an object type for event '%s', received %s. JSON string for event '%s': %s", CYIWebMessagingBridge::ERROR_ATTRIBUTE_NAME, CYIRapidJSONUtility::TypeToString(eventErrorValue.GetType()).GetData(), CYIWebMessagingBridge::ERROR_ATTRIBUTE_NAME, CYIRapidJSONUtility::CreateStringFromValue(eventErrorValue).GetData());
        }
        else
        {
            CYIParsingError parsingError;

            CYIRapidJSONUtility::GetStringField(&eventErrorValue, CYIWebMessagingBridge::ERROR_MESSAGE_ATTRIBUTE_NAME, message, parsingError);

            if (parsingError.HasError())
            {
                YI_LOGE(LOG_TAG, "OnPlayerErrorThrown encountered an invalid string value for event error '%s'. JSON string for event error: %s", CYIWebMessagingBridge::ERROR_MESSAGE_ATTRIBUTE_NAME, CYIRapidJSONUtility::CreateStringFromValue(eventErrorValue).GetData());
            }

            if (eventErrorValue.HasMember(ERROR_CODE_ATTRIBUTE_NAME))
            {
                code = CYIRapidJSONUtility::CreateStringFromValue(eventErrorValue[ERROR_CODE_ATTRIBUTE_NAME]);
            }
        }
    }

    CYIAbstractVideoPlayer::Error error;
    error.errorCode = CYIAbstractVideoPlayer::ErrorCode::PlaybackError;
    error.message = message.IsNotEmpty() ? message : DEFAULT_ERROR_MESSAGE;
    error.nativePlayerErrorCode = code;
    m_pPub->NotifyErrorOccurred(error);
}

void CYIVideojsVideoPlayerPriv::OnAudioTracksChanged(const yi::rapidjson::Value &eventValue)
{
    if (!eventValue.IsObject())
    {
        YI_LOGE(LOG_TAG, "OnAudioTracksChanged encountered an invalid event value, expected object, received %s. JSON string for event: %s", CYIRapidJSONUtility::TypeToString(eventValue.GetType()).GetData(), CYIRapidJSONUtility::CreateStringFromValue(eventValue).GetData());
        return;
    }

    if (!eventValue.HasMember(CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME))
    {
        YI_LOGE(LOG_TAG, "OnAudioTracksChanged event value is missing '%s' attribute!", CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME);
        return;
    }

    const yi::rapidjson::Value &trackListValue = eventValue[CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME];

    if (!trackListValue.IsArray())
    {
        YI_LOGE(LOG_TAG, "OnAudioTracksChanged expected an array type for event '%s', received %s. JSON string for event '%s': %s", CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME, CYIRapidJSONUtility::TypeToString(trackListValue.GetType()).GetData(), CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME, CYIRapidJSONUtility::CreateStringFromValue(trackListValue).GetData());
        return;
    }

    m_audioTracks.clear();

    for (uint32_t i = 0; i < trackListValue.Size(); i++)
    {
        const yi::rapidjson::Value &trackValue = trackListValue[i];

        if (!trackValue.IsObject())
        {
            YI_LOGE(LOG_TAG, "OnAudioTracksChanged expected an object type for track with index %d, received %s. JSON string for current track: %s", i, CYIRapidJSONUtility::TypeToString(trackValue.GetType()).GetData(), CYIRapidJSONUtility::CreateStringFromValue(trackValue).GetData());
            continue;
        }

        CYIAbstractVideoPlayer::AudioTrackInfo audioTrackInfo(0);

        if (ConvertValueToTrackInfo(trackValue, audioTrackInfo))
        {
            m_audioTracks.push_back(audioTrackInfo);
        }
        else
        {
            YI_LOGE(LOG_TAG, "OnAudioTracksChanged encountered an invalid audio track at index %d.", i);
        }
    }
}

void CYIVideojsVideoPlayerPriv::OnVideoDurationChanged(const yi::rapidjson::Value &eventValue)
{
    if (!eventValue.IsObject())
    {
        YI_LOGE(LOG_TAG, "OnVideoDurationChanged encountered an invalid event value, expected object, received %s. JSON string for event: %s", CYIRapidJSONUtility::TypeToString(eventValue.GetType()).GetData(), CYIRapidJSONUtility::CreateStringFromValue(eventValue).GetData());
        return;
    }

    if (!eventValue.HasMember(CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME))
    {
        YI_LOGE(LOG_TAG, "OnVideoDurationChanged event value is missing '%s' attribute!", CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME);
        return;
    }

    const yi::rapidjson::Value &durationSecondsValue = eventValue[CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME];

    double durationSeconds;

    if (!durationSecondsValue.IsNumber())
    {
        YI_LOGE(LOG_TAG, "OnVideoDurationChanged encountered an invalid number value for event data. JSON string for event data: %s", CYIRapidJSONUtility::CreateStringFromValue(durationSecondsValue).GetData());
        return;
    }

    durationSeconds = durationSecondsValue.GetDouble();

    if(durationSeconds < 0) {
        return;
    }

    m_durationMs = static_cast<uint64_t>(durationSeconds * 1000.0f);

    m_pPub->NotifyDurationChanged(m_durationMs);
}

void CYIVideojsVideoPlayerPriv::OnVideoTimeChanged(const yi::rapidjson::Value &eventValue)
{
    static const char *CURRENT_TIME_ATTRIBUTE_NAME = "currentTimeSeconds";
    static const char *BUFFER_LENGTH_ATTRIBUTE_NAME = "bufferLengthMs";

    if (!eventValue.IsObject())
    {
        YI_LOGE(LOG_TAG, "OnVideoTimeChanged encountered an invalid event value, expected object, received %s. JSON string for event: %s", CYIRapidJSONUtility::TypeToString(eventValue.GetType()).GetData(), CYIRapidJSONUtility::CreateStringFromValue(eventValue).GetData());
        return;
    }

    if (!eventValue.HasMember(CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME))
    {
        YI_LOGE(LOG_TAG, "OnVideoTimeChanged event value is missing '%s' attribute!", CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME);
        return;
    }

    const yi::rapidjson::Value &eventDataValue = eventValue[CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME];

    if (!eventDataValue.IsObject())
    {
        YI_LOGE(LOG_TAG, "OnVideoTimeChanged expected an object type for event '%s', received %s. JSON string for event '%s': %s", CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME, CYIRapidJSONUtility::TypeToString(eventDataValue.GetType()).GetData(), CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME, CYIRapidJSONUtility::CreateStringFromValue(eventDataValue).GetData());
        return;
    }

    if (!eventDataValue.HasMember(CURRENT_TIME_ATTRIBUTE_NAME))
    {
        YI_LOGE(LOG_TAG, "OnVideoTimeChanged event data is missing '%s' attribute! JSON string for event data: %s", CURRENT_TIME_ATTRIBUTE_NAME, CYIRapidJSONUtility::CreateStringFromValue(eventDataValue).GetData());
        return;
    }

    const yi::rapidjson::Value &currentTimeValue = eventDataValue[CURRENT_TIME_ATTRIBUTE_NAME];

    double currentTimeSeconds;

    if (!currentTimeValue.IsNumber())
    {
        YI_LOGE(LOG_TAG, "OnVideoTimeChanged encountered an invalid number value for event data '%s'. JSON string for event data: %s", CURRENT_TIME_ATTRIBUTE_NAME, CYIRapidJSONUtility::CreateStringFromValue(eventDataValue).GetData());
        return;
    }

    currentTimeSeconds = currentTimeValue.GetDouble();

    if(currentTimeSeconds < 0) {
        YI_LOGE(LOG_TAG, "OnVideoTimeChanged encountered a negative number value for event data '%s'. JSON string for event data: %s", CURRENT_TIME_ATTRIBUTE_NAME, CYIRapidJSONUtility::CreateStringFromValue(eventDataValue).GetData());
        return;
    }

    m_currentTimeMs = static_cast<uint64_t>(currentTimeSeconds * 1000.0f);

    if (m_pPub->GetPlayerState() == CYIAbstractVideoPlayer::PlaybackState::Paused || m_pPub->GetPlayerState() == CYIAbstractVideoPlayer::PlaybackState::Buffering)
    {
        m_pPub->UpdateCurrentTime();
    }

    CYIParsingError parsingError;

    CYIRapidJSONUtility::GetFloatField(&eventDataValue, BUFFER_LENGTH_ATTRIBUTE_NAME, &m_bufferLengthMs, parsingError);

    if (parsingError.HasError() && parsingError.GetParsingErrorCode() != CYIParsingError::ErrorType::DataFieldMissing)
    {
        YI_LOGE(LOG_TAG, "OnVideoTimeChanged encountered an invalid float value for event data '%s'. JSON string for event data: %s", BUFFER_LENGTH_ATTRIBUTE_NAME, CYIRapidJSONUtility::CreateStringFromValue(eventDataValue).GetData());
        return;
    }
}

void CYIVideojsVideoPlayerPriv::OnStateChanged(const yi::rapidjson::Value &eventValue)
{
    if (!eventValue.IsObject())
    {
        YI_LOGE(LOG_TAG, "OnStateChanged encountered an invalid event value, expected object, received %s. JSON string for event: %s", CYIRapidJSONUtility::TypeToString(eventValue.GetType()).GetData(), CYIRapidJSONUtility::CreateStringFromValue(eventValue).GetData());
        return;
    }

    CYIParsingError parsingError;

    int32_t stateValue = -1;

    CYIRapidJSONUtility::GetIntegerField(&eventValue, CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME, &stateValue, parsingError);

    if (parsingError.HasError())
    {
        YI_LOGE(LOG_TAG, "OnStateChanged event value is does not contain a valid integer value for '%s'. JSON string for event value: %s", CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME, CYIRapidJSONUtility::CreateStringFromValue(eventValue).GetData());
        return;
    }

    PlayerState state = static_cast<PlayerState>(stateValue);

    switch (state)
    {
        case PlayerState::Uninitialized:
        {
            break;
        }
        case PlayerState::Initialized:
        {
            m_pPub->m_pStateManager->TransitionToMediaUnloaded();
            break;
        }
        case PlayerState::Loading:
        {
            m_pPub->m_pStateManager->TransitionToMediaPreparing();
            break;
        }
        case PlayerState::Loaded:
        {
            m_pPub->m_pStateManager->TransitionToMediaReady();
            break;
        }
        case PlayerState::Playing:
        {
            m_pPub->m_pStateManager->TransitionToPlaybackPlaying();
            break;
        }
        case PlayerState::Paused:
        {
            m_pPub->m_pStateManager->TransitionToPlaybackPaused();
            break;
        }
        case PlayerState::Complete:
        {
            m_pPub->NotifyPlaybackComplete();
            break;
        }
        default:
        {
            YI_LOGE(LOG_TAG, "OnStateChanged encountered an unexpected, invalid and unhandled state value: %d", static_cast<int32_t>(state));
            break;
        }
    };
}

void CYIVideojsVideoPlayerPriv::OnTextTracksChanged(const yi::rapidjson::Value &eventValue)
{
    if (!eventValue.IsObject())
    {
        YI_LOGE(LOG_TAG, "OnTextTracksChanged encountered an invalid event value, expected object, received %s. JSON string for event: %s", CYIRapidJSONUtility::TypeToString(eventValue.GetType()).GetData(), CYIRapidJSONUtility::CreateStringFromValue(eventValue).GetData());
        return;
    }

    if (!eventValue.HasMember(CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME))
    {
        YI_LOGE(LOG_TAG, "OnTextTracksChanged event value is missing '%s' attribute!", CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME);
        return;
    }

    const yi::rapidjson::Value &trackListValue = eventValue[CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME];

    if (!trackListValue.IsArray())
    {
        YI_LOGE(LOG_TAG, "OnTextTracksChanged expected an array type for event '%s', received %s. JSON string for event '%s': %s", CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME, CYIRapidJSONUtility::TypeToString(trackListValue.GetType()).GetData(), CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME, CYIRapidJSONUtility::CreateStringFromValue(trackListValue).GetData());
        return;
    }

    m_textTracks.clear();

    for (uint32_t i = 0; i < trackListValue.Size(); i++)
    {
        const yi::rapidjson::Value &trackValue = trackListValue[i];

        if (!trackValue.IsObject())
        {
            YI_LOGE(LOG_TAG, "OnTextTracksChanged expected an object type for track with index %d, received %s. JSON string for current track: %s", i, CYIRapidJSONUtility::TypeToString(trackValue.GetType()).GetData(), CYIRapidJSONUtility::CreateStringFromValue(trackValue).GetData());
            continue;
        }

        CYIAbstractVideoPlayer::ClosedCaptionsTrackInfo textTrackInfo(0);

        if (ConvertValueToTrackInfo(trackValue, textTrackInfo))
        {
            m_textTracks.push_back(textTrackInfo);
        }
        else
        {
            YI_LOGE(LOG_TAG, "OnTextTracksChanged encountered an invalid text track at index %d.", i);
        }
    }
}

void CYIVideojsVideoPlayerPriv::OnMetadataAvailable(const yi::rapidjson::Value &eventValue)
{
    static const char *IDENTIFIER_ATTRIBUTE_NAME = "identifier";
    static const char *VALUE_ATTRIBUTE_NAME = "value";
    static const char *TIMESTAMP_ATTRIBUTE_NAME = "timestamp";
    static const char *DURATION_ATTRIBUTE_NAME = "durationMs";

    if (!eventValue.IsObject())
    {
        YI_LOGE(LOG_TAG, "OnMetadataAvailable encountered an invalid event value, expected object, received %s. JSON string for event: %s", CYIRapidJSONUtility::TypeToString(eventValue.GetType()).GetData(), CYIRapidJSONUtility::CreateStringFromValue(eventValue).GetData());
        return;
    }

    if (!eventValue.HasMember(CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME))
    {
        YI_LOGE(LOG_TAG, "OnMetadataAvailable event value is missing '%s' attribute!", CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME);
        return;
    }

    const yi::rapidjson::Value &eventDataValue = eventValue[CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME];

    if (!eventDataValue.IsObject())
    {
        YI_LOGE(LOG_TAG, "OnMetadataAvailable expected an object type for event '%s', received %s. JSON string for event '%s': %s", CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME, CYIRapidJSONUtility::TypeToString(eventDataValue.GetType()).GetData(), CYIWebMessagingBridge::EVENT_DATA_ATTRIBUTE_NAME, CYIRapidJSONUtility::CreateStringFromValue(eventDataValue).GetData());
        return;
    }

    CYIAbstractVideoPlayer::TimedMetadata timedMetadata;

    CYIParsingError parsingError;

    CYIRapidJSONUtility::GetStringField(&eventDataValue, IDENTIFIER_ATTRIBUTE_NAME, timedMetadata.identifier, parsingError);

    if (parsingError.HasError() || timedMetadata.identifier.IsEmpty())
    {
        YI_LOGE(LOG_TAG, "OnMetadataAvailable encountered an invalid string value for event data '%s'. JSON string for event data: %s", IDENTIFIER_ATTRIBUTE_NAME, CYIRapidJSONUtility::CreateStringFromValue(eventDataValue).GetData());
        return;
    }

    CYIRapidJSONUtility::GetStringField(&eventDataValue, VALUE_ATTRIBUTE_NAME, timedMetadata.value, parsingError);

    if (parsingError.HasError())
    {
        YI_LOGE(LOG_TAG, "OnMetadataAvailable encountered an invalid string value for event data '%s'. JSON string for event data: %s", VALUE_ATTRIBUTE_NAME, CYIRapidJSONUtility::CreateStringFromValue(eventDataValue).GetData());
        return;
    }

    uint64_t timestamp = 0;

    if (!eventDataValue.HasMember(TIMESTAMP_ATTRIBUTE_NAME))
    {
        YI_LOGE(LOG_TAG, "OnMetadataAvailable event data value is missing '%s' attribute!", TIMESTAMP_ATTRIBUTE_NAME);
        return;
    }

    const yi::rapidjson::Value &timestampValue = eventDataValue[TIMESTAMP_ATTRIBUTE_NAME];

    if (timestampValue.IsUint64())
    {
        timestamp = timestampValue.GetUint64();
    }
    else if (timestampValue.IsDouble()) {
        timestamp = static_cast<int64_t>(timestampValue.GetDouble());
    }
    else
    {
        YI_LOGE(LOG_TAG, "OnMetadataAvailable encountered an invalid number value for event data '%s'. JSON string for event data: %s", TIMESTAMP_ATTRIBUTE_NAME, CYIRapidJSONUtility::CreateStringFromValue(timestampValue).GetData());
        return;
    }

    timedMetadata.timestamp = std::chrono::microseconds{std::chrono::milliseconds{timestamp}};

    uint64_t durationMs = 0;

    if (!eventDataValue.HasMember(DURATION_ATTRIBUTE_NAME))
    {
        YI_LOGE(LOG_TAG, "OnMetadataAvailable event data value is missing '%s' attribute!", DURATION_ATTRIBUTE_NAME);
        return;
    }

    const yi::rapidjson::Value &durationSecondsValue = eventDataValue[DURATION_ATTRIBUTE_NAME];

    if (durationSecondsValue.IsUint64())
    {
        durationMs = durationSecondsValue.GetUint64();
    }
    else if (durationSecondsValue.IsDouble()) {
        durationMs = static_cast<int64_t>(durationSecondsValue.GetDouble());
    }
    else
    {
        YI_LOGE(LOG_TAG, "OnMetadataAvailable encountered an invalid number value for event data '%s'. JSON string for event data: %s", DURATION_ATTRIBUTE_NAME, CYIRapidJSONUtility::CreateStringFromValue(durationSecondsValue).GetData());
        return;
    }

    timedMetadata.duration = std::chrono::microseconds{std::chrono::milliseconds{durationMs}};

    MetadataAvailable.Emit(timedMetadata);
}

void CYIVideojsVideoPlayerPriv::AddDRMConfigurationToValue(CYIAbstractVideoPlayer::DRMConfiguration *pDRMConfiguration, yi::rapidjson::Value &value, yi::rapidjson::MemoryPoolAllocator<yi::rapidjson::CrtAllocator> &allocator)
{
    if (!pDRMConfiguration)
    {
        return;
    }

    if (!value.IsObject())
    {
        YI_LOGE(LOG_TAG, "AddDRMConfigurationToValue expected an object value type, received %s. JSON string for value: %s", CYIRapidJSONUtility::TypeToString(value.GetType()).GetData(), CYIRapidJSONUtility::CreateStringFromValue(value).GetData());
        return;
    }

    if (pDRMConfiguration->GetScheme() == CYIAbstractVideoPlayer::DRMScheme::PlayReady || pDRMConfiguration->GetScheme() == CYIAbstractVideoPlayer::DRMScheme::WidevineModular)
    {
        CYILicenseAcquisitionDRMConfiguration *pLicenseAcquisitionDRMConfiguration = dynamic_cast<CYILicenseAcquisitionDRMConfiguration *>(pDRMConfiguration);
        yi::rapidjson::Value drmConfigurationValue(yi::rapidjson::kObjectType);
        const CYIUrl licenseAcquisitionUrl = pLicenseAcquisitionDRMConfiguration->GetLicenseAcquisitionUrl();

        if (!licenseAcquisitionUrl.IsEmpty())
        {
            CYIString url(licenseAcquisitionUrl.ToString());

            yi::rapidjson::Value urlValue(url.GetData(), allocator);
            drmConfigurationValue.AddMember(yi::rapidjson::StringRef("url"), urlValue, allocator);
        }

        if (pDRMConfiguration->GetScheme() != CYIAbstractVideoPlayer::DRMScheme::None)
        {
            yi::rapidjson::Value drmSchemeValue(DRMSchemeToString(pDRMConfiguration->GetScheme()).GetData(), allocator);
            drmConfigurationValue.AddMember(yi::rapidjson::StringRef("type"), drmSchemeValue, allocator);

            if (pDRMConfiguration->GetScheme() == CYIAbstractVideoPlayer::DRMScheme::WidevineModular ||
                pDRMConfiguration->GetScheme() == CYIAbstractVideoPlayer::DRMScheme::WidevineModularCustomRequest)
            {
                drmConfigurationValue.AddMember(yi::rapidjson::StringRef("custom"), yi::rapidjson::Value(pDRMConfiguration->GetScheme() == CYIAbstractVideoPlayer::DRMScheme::WidevineModularCustomRequest), allocator);
            }

            const std::map<CYIString, CYIString> &licenseAcquisitionHeaders = pLicenseAcquisitionDRMConfiguration->GetLicenseAcquisitionHeaders();

            if (!licenseAcquisitionHeaders.empty())
            {
                yi::rapidjson::Value licenseAcquisitionHeadersValue(yi::rapidjson::kObjectType);

                for (const auto &headerPair : licenseAcquisitionHeaders)
                {
                    yi::rapidjson::Value headerNameValue(headerPair.first.GetData(), allocator);
                    yi::rapidjson::Value headerValueValue(headerPair.second.GetData(), allocator);
                    licenseAcquisitionHeadersValue.AddMember(headerNameValue, headerValueValue, allocator);
                }

                drmConfigurationValue.AddMember(yi::rapidjson::StringRef("headers"), licenseAcquisitionHeadersValue, allocator);
            }
        }

        value.AddMember("drmConfiguration", drmConfigurationValue, allocator);
    }
}

CYIString CYIVideojsVideoPlayerPriv::GetName() const
{
    static const char *FUNCTION_NAME = "getType";
    static CYIString type;

    if (type.IsEmpty())
    {
        bool messageSent = false;
        CYIWebMessagingBridge::FutureResponse futureResponse = CallStaticPlayerFunction(yi::rapidjson::Document(), FUNCTION_NAME, yi::rapidjson::Value(yi::rapidjson::kArrayType), &messageSent);

        if (!messageSent)
        {
            YI_LOGE(LOG_TAG, "Failed to invoke %s function.", FUNCTION_NAME);
        }
        else
        {
            bool valueAssigned = false;
            CYIWebMessagingBridge::Response response = futureResponse.Take(CYIWebMessagingBridge::DEFAULT_RESPONSE_TIMEOUT_MS, &valueAssigned);

            if (!valueAssigned)
            {
                YI_LOGE(LOG_TAG, "GetName did not receive a response from the web messaging bridge!");
            }
            else if (response.HasError())
            {
                YI_LOGE(LOG_TAG, "%s", response.GetError()->GetMessage().GetData());
            }
            else
            {
                const yi::rapidjson::Value *pData = response.GetResult();

                if (!pData->IsString())
                {
                    YI_LOGE(LOG_TAG, "GetName expected a string type for result, received %s. JSON string for result: %s", CYIRapidJSONUtility::TypeToString(pData->GetType()).GetData(), CYIRapidJSONUtility::CreateStringFromValue(*pData).GetData());
                }
                else
                {
                    type = pData->GetString();
                }
            }
        }
    }

    return CYIString(type.IsEmpty() ? "Invalid" : type);
}

CYIString CYIVideojsVideoPlayerPriv::GetNickname() const
{
    static const char *FUNCTION_NAME = "getNickname";

    bool messageSent = false;
    CYIWebMessagingBridge::FutureResponse futureResponse = CallPlayerInstanceFunction(yi::rapidjson::Document(), FUNCTION_NAME, yi::rapidjson::Value(yi::rapidjson::kArrayType), &messageSent);

    if (!messageSent)
    {
        YI_LOGE(LOG_TAG, "Failed to invoke %s function.", FUNCTION_NAME);
    }
    else
    {
        bool valueAssigned = false;
        CYIWebMessagingBridge::Response response = futureResponse.Take(CYIWebMessagingBridge::DEFAULT_RESPONSE_TIMEOUT_MS, &valueAssigned);

        if (!valueAssigned)
        {
            YI_LOGE(LOG_TAG, "GetNickname did not receive a response from the web messaging bridge!");
        }
        else if (response.HasError())
        {
            YI_LOGE(LOG_TAG, "%s", response.GetError()->GetMessage().GetData());
        }
        else
        {
            const yi::rapidjson::Value *pData = response.GetResult();

            if (!pData->IsString())
            {
                YI_LOGE(LOG_TAG, "GetNickname expected a string type for result, received %s. JSON string for result: %s", CYIRapidJSONUtility::TypeToString(pData->GetType()).GetData(), CYIRapidJSONUtility::CreateStringFromValue(*pData).GetData());
            }
            else
            {
                return pData->GetString();
            }
        }
    }

    return CYIString::EmptyString();
}

CYIString CYIVideojsVideoPlayerPriv::GetVersion() const
{
    static const char *FUNCTION_NAME = "getVersion";
    static CYIString version;

    if (version.IsEmpty())
    {
        bool messageSent = false;
        CYIWebMessagingBridge::FutureResponse futureResponse = CallStaticPlayerFunction(yi::rapidjson::Document(), FUNCTION_NAME, yi::rapidjson::Value(yi::rapidjson::kArrayType), &messageSent);

        if (!messageSent)
        {
            YI_LOGE(LOG_TAG, "Failed to invoke %s function.", FUNCTION_NAME);
        }
        else
        {
            bool valueAssigned = false;
            CYIWebMessagingBridge::Response response = futureResponse.Take(CYIWebMessagingBridge::DEFAULT_RESPONSE_TIMEOUT_MS, &valueAssigned);

            if (!valueAssigned)
            {
                YI_LOGE(LOG_TAG, "GetVersion did not receive a response from the web messaging bridge!");
            }
            else if (response.HasError())
            {
                YI_LOGE(LOG_TAG, "%s", response.GetError()->GetMessage().GetData());
            }
            else
            {
                const yi::rapidjson::Value *pData = response.GetResult();

                if (!pData->IsString())
                {
                    YI_LOGE(LOG_TAG, "GetVersion expected a string type for result, received %s. JSON string for result: %s", CYIRapidJSONUtility::TypeToString(pData->GetType()).GetData(), CYIRapidJSONUtility::CreateStringFromValue(*pData).GetData());
                }
                else
                {
                    version = pData->GetString();
                }
            }
        }
    }

    return version.IsEmpty() ? "Unknown" : version;
}

void CYIVideojsVideoPlayerPriv::SetNickname(const CYIString &nickname) const
{
    static const char *FUNCTION_NAME = "setNickname";

    yi::rapidjson::Document command(yi::rapidjson::kObjectType);
    yi::rapidjson::MemoryPoolAllocator<yi::rapidjson::CrtAllocator> &allocator = command.GetAllocator();

    yi::rapidjson::Value arguments(yi::rapidjson::kArrayType);
    yi::rapidjson::Value nicknameValue(nickname.GetData(), allocator);
    arguments.PushBack(nicknameValue, allocator);

    bool messageSent = false;
    CYIWebMessagingBridge::FutureResponse futureResponse = CallPlayerInstanceFunction(std::move(command), FUNCTION_NAME, std::move(arguments), &messageSent);

    if (!messageSent)
    {
        YI_LOGE(LOG_TAG, "Failed to invoke %s function.", FUNCTION_NAME);
    }
    else
    {
        bool valueAssigned = false;
        CYIWebMessagingBridge::Response response = futureResponse.Take(CYIWebMessagingBridge::DEFAULT_RESPONSE_TIMEOUT_MS, &valueAssigned);

        if (!valueAssigned)
        {
            YI_LOGE(LOG_TAG, "SetNickname did not receive a response from the web messaging bridge!");
        }
        else if (response.HasError())
        {
            YI_LOGE(LOG_TAG, "%s", response.GetError()->GetMessage().GetData());
        }
    }
}

CYIAbstractVideoPlayer::Statistics CYIVideojsVideoPlayerPriv::GetStatistics() const
{
    CYIAbstractVideoPlayer::Statistics stats;
    stats.isLive = m_isLive;
    stats.audioBitrateKbps = m_currentAudioBitrateKbps;
    stats.defaultAudioBitrateKbps = m_initialVideoBitrateKbps;
    stats.videoBitrateKbps = m_currentVideoBitrateKbps;
    stats.defaultVideoBitrateKbps = m_initialVideoBitrateKbps;
    stats.totalBitrateKbps = m_currentTotalBitrateKbps;
    stats.defaultTotalBitrateKbps = m_initialTotalBitrateKbps;
    stats.bufferLengthMs = m_bufferLengthMs;
    return stats;
}

std::unique_ptr<CYIVideoSurface> CYIVideojsVideoPlayerPriv::CreateSurface()
{
    std::unique_ptr<CYIVideoSurface> pSurface(new CYIVideojsVideoSurface(this));
    return pSurface;
}

bool CYIVideojsVideoPlayerPriv::SupportsFormat(CYIAbstractVideoPlayer::StreamingFormat format, CYIAbstractVideoPlayer::DRMScheme drmScheme) const
{
    static const char *FUNCTION_NAME = "isStreamFormatSupported";

    yi::rapidjson::Document command(yi::rapidjson::kObjectType);
    yi::rapidjson::MemoryPoolAllocator<yi::rapidjson::CrtAllocator> &allocator = command.GetAllocator();

    yi::rapidjson::Value arguments(yi::rapidjson::kArrayType);
    CYIString streamFormatName(StreamFormatToString(format));

    yi::rapidjson::Value streamFormatValue(streamFormatName.GetData(), allocator);
    arguments.PushBack(streamFormatValue, allocator);

    CYIString drmSchemeName(DRMSchemeToString(drmScheme));
    if (drmScheme != CYIAbstractVideoPlayer::DRMScheme::None && !drmSchemeName.IsEmpty())
    {
        yi::rapidjson::Value drmSchemeValue(drmSchemeName.GetData(), allocator);
        arguments.PushBack(drmSchemeValue, allocator);
    }

    bool messageSent = false;
    CYIWebMessagingBridge::FutureResponse futureResponse = CallPlayerInstanceFunction(std::move(command), FUNCTION_NAME, std::move(arguments), &messageSent);

    if (!messageSent)
    {
        YI_LOGE(LOG_TAG, "Failed to invoke %s function.", FUNCTION_NAME);
    }
    else
    {
        bool valueAssigned = false;
        CYIWebMessagingBridge::Response response = futureResponse.Take(CYIWebMessagingBridge::DEFAULT_RESPONSE_TIMEOUT_MS, &valueAssigned);

        if (!valueAssigned)
        {
            YI_LOGE(LOG_TAG, "SupportsFormat did not receive a response from the web messaging bridge!");
        }
        else if (response.HasError())
        {
            YI_LOGE(LOG_TAG, "%s", response.GetError()->GetMessage().GetData());
        }
        else
        {
            const yi::rapidjson::Value *pData = response.GetResult();

            if (!pData->IsBool())
            {
                YI_LOGE(LOG_TAG, "SupportsFormat expected a boolean type for result, received %s. JSON string for result: %s", CYIRapidJSONUtility::TypeToString(pData->GetType()).GetData(), CYIRapidJSONUtility::CreateStringFromValue(*pData).GetData());
            }
            else
            {
                return pData->GetBool();
            }
        }
    }

    return false;
}

void CYIVideojsVideoPlayerPriv::Prepare(const CYIUrl &videoURI, CYIAbstractVideoPlayer::StreamingFormat format)
{
    static const char *FUNCTION_NAME = "prepare";
    static const uint32_t PREPARE_TIMEOUT_MS = 3000;

    yi::rapidjson::Document command(yi::rapidjson::kObjectType);
    yi::rapidjson::MemoryPoolAllocator<yi::rapidjson::CrtAllocator> &allocator = command.GetAllocator();

    yi::rapidjson::Value arguments(yi::rapidjson::kArrayType);
    yi::rapidjson::Value playerConfigurationValue(yi::rapidjson::kObjectType);

    CYIString url(videoURI.ToString());
    yi::rapidjson::Value urlValue(url.GetData(), allocator);
    playerConfigurationValue.AddMember(yi::rapidjson::StringRef("url"), urlValue, allocator);

    CYIString streamFormatName(StreamFormatToString(format));
    yi::rapidjson::Value streamFormatValue(streamFormatName.GetData(), allocator);
    playerConfigurationValue.AddMember(yi::rapidjson::StringRef("format"), streamFormatValue, allocator);

    playerConfigurationValue.AddMember(yi::rapidjson::StringRef("startTimeSeconds"), yi::rapidjson::Value(m_pPub->m_initialStartTimeMs / 1000.0), allocator);

    playerConfigurationValue.AddMember(yi::rapidjson::StringRef("maxBitrateKbps"), yi::rapidjson::Value(static_cast<double>(m_pPub->m_maxBitrate) / BITRATE_KBPS_SCALE), allocator);

    AddDRMConfigurationToValue(m_pPub->m_pDRMConfiguration.get(), playerConfigurationValue, allocator);

    arguments.PushBack(playerConfigurationValue, allocator);

    bool messageSent = false;
    CYIWebMessagingBridge::FutureResponse futureResponse = CallPlayerInstanceFunction(std::move(command), FUNCTION_NAME, std::move(arguments), &messageSent);

    if (!messageSent)
    {
        YI_LOGE(LOG_TAG, "Failed to invoke %s function.", FUNCTION_NAME);
    }
    else
    {
        bool valueAssigned = false;
        CYIWebMessagingBridge::Response response = futureResponse.Take(PREPARE_TIMEOUT_MS, &valueAssigned);

        if (!valueAssigned)
        {
            YI_LOGE(LOG_TAG, "Prepare did not receive a response from the web messaging bridge!");
        }
        else if (response.HasError())
        {
            YI_LOGE(LOG_TAG, "%s", response.GetError()->GetMessage().GetData());
        }
    }
}

void CYIVideojsVideoPlayerPriv::Play()
{
    static const char *FUNCTION_NAME = "play";

    bool messageSent = false;
    CYIWebMessagingBridge::FutureResponse futureResponse = CallPlayerInstanceFunction(yi::rapidjson::Document(), FUNCTION_NAME, yi::rapidjson::Value(yi::rapidjson::kArrayType), &messageSent);

    if (!messageSent)
    {
        YI_LOGE(LOG_TAG, "Failed to invoke %s function.", FUNCTION_NAME);
    }
    else
    {
        bool valueAssigned = false;
        CYIWebMessagingBridge::Response response = futureResponse.Take(CYIWebMessagingBridge::DEFAULT_RESPONSE_TIMEOUT_MS, &valueAssigned);

        if (!valueAssigned)
        {
            YI_LOGE(LOG_TAG, "Play did not receive a response from the web messaging bridge!");
        }
        else if (response.HasError())
        {
            YI_LOGE(LOG_TAG, "%s", response.GetError()->GetMessage().GetData());
        }
    }
}

void CYIVideojsVideoPlayerPriv::Pause()
{
    static const char *FUNCTION_NAME = "pause";

    bool messageSent = false;
    CYIWebMessagingBridge::FutureResponse futureResponse = CallPlayerInstanceFunction(yi::rapidjson::Document(), FUNCTION_NAME, yi::rapidjson::Value(yi::rapidjson::kArrayType), &messageSent);

    if (!messageSent)
    {
        YI_LOGE(LOG_TAG, "Failed to invoke %s function.", FUNCTION_NAME);
    }
    else
    {
        bool valueAssigned = false;
        CYIWebMessagingBridge::Response response = futureResponse.Take(CYIWebMessagingBridge::DEFAULT_RESPONSE_TIMEOUT_MS, &valueAssigned);

        if (!valueAssigned)
        {
            YI_LOGE(LOG_TAG, "Pause did not receive a response from the web messaging bridge!");
        }
        else if (response.HasError())
        {
            YI_LOGE(LOG_TAG, "%s", response.GetError()->GetMessage().GetData());
        }
    }
}

void CYIVideojsVideoPlayerPriv::Stop()
{
    static const char *FUNCTION_NAME = "stop";

    bool messageSent = false;
    CYIWebMessagingBridge::FutureResponse futureResponse = CallPlayerInstanceFunction(yi::rapidjson::Document(), FUNCTION_NAME, yi::rapidjson::Value(yi::rapidjson::kArrayType), &messageSent);

    if (!messageSent)
    {
        YI_LOGE(LOG_TAG, "Failed to invoke %s function.", FUNCTION_NAME);
    }
    else
    {
        bool valueAssigned = false;
        CYIWebMessagingBridge::Response response = futureResponse.Take(CYIWebMessagingBridge::DEFAULT_RESPONSE_TIMEOUT_MS, &valueAssigned);

        if (!valueAssigned)
        {
            YI_LOGE(LOG_TAG, "Stop did not receive a response from the web messaging bridge!");
        }
        else if (response.HasError())
        {
            YI_LOGE(LOG_TAG, "%s", response.GetError()->GetMessage().GetData());
        }
    }

    m_durationMs = 0;
    m_currentTimeMs = 0;
    m_buffering = false;
    m_isLive = false;
    m_currentAudioBitrateKbps = -1.0f;
    m_initialAudioBitrateKbps = -1.0f;
    m_currentVideoBitrateKbps = -1.0f;
    m_initialVideoBitrateKbps = -1.0f;
    m_currentTotalBitrateKbps = -1.0f;
    m_initialTotalBitrateKbps = -1.0f;
    m_bufferLengthMs = -1.0f;
    m_stateBeforeBuffering = CYIAbstractVideoPlayer::PlaybackState::Paused;

    m_audioTracks.clear();
    m_textTracks.clear();
}

uint64_t CYIVideojsVideoPlayerPriv::GetDurationMs() const
{
    return m_durationMs;
}

uint64_t CYIVideojsVideoPlayerPriv::GetCurrentTimeMs() const
{
    return m_currentTimeMs;
}

void CYIVideojsVideoPlayerPriv::Seek(uint64_t seekPositionMS)
{
    static const char *FUNCTION_NAME = "seek";

    yi::rapidjson::Document command(yi::rapidjson::kObjectType);
    yi::rapidjson::MemoryPoolAllocator<yi::rapidjson::CrtAllocator> &allocator = command.GetAllocator();

    yi::rapidjson::Value arguments(yi::rapidjson::kArrayType);
    arguments.PushBack(yi::rapidjson::Value(seekPositionMS / 1000.0), allocator);

    bool messageSent = false;
    CYIWebMessagingBridge::FutureResponse futureResponse = CallPlayerInstanceFunction(std::move(command), FUNCTION_NAME, std::move(arguments), &messageSent);

    if (!messageSent)
    {
        YI_LOGE(LOG_TAG, "Failed to invoke %s function.", FUNCTION_NAME);
    }
    else
    {
        bool valueAssigned = false;
        CYIWebMessagingBridge::Response response = futureResponse.Take(CYIWebMessagingBridge::DEFAULT_RESPONSE_TIMEOUT_MS, &valueAssigned);

        if (!valueAssigned)
        {
            YI_LOGE(LOG_TAG, "Seek did not receive a response from the web messaging bridge!");
        }
        else if (response.HasError())
        {
            YI_LOGE(LOG_TAG, "%s", response.GetError()->GetMessage().GetData());
        }
    }
}

bool CYIVideojsVideoPlayerPriv::SelectAudioTrack(uint32_t id)
{
    static const char *FUNCTION_NAME = "selectAudioTrack";

    yi::rapidjson::Document command(yi::rapidjson::kObjectType);
    yi::rapidjson::MemoryPoolAllocator<yi::rapidjson::CrtAllocator> &allocator = command.GetAllocator();

    yi::rapidjson::Value arguments(yi::rapidjson::kArrayType);
    arguments.PushBack(yi::rapidjson::Value(id), allocator);

    bool messageSent = false;
    CYIWebMessagingBridge::FutureResponse futureResponse = CallPlayerInstanceFunction(std::move(command), FUNCTION_NAME, std::move(arguments), &messageSent);

    if (!messageSent)
    {
        YI_LOGE(LOG_TAG, "Failed to invoke %s function.", FUNCTION_NAME);
    }
    else
    {
        bool valueAssigned = false;
        CYIWebMessagingBridge::Response response = futureResponse.Take(CYIWebMessagingBridge::DEFAULT_RESPONSE_TIMEOUT_MS, &valueAssigned);

        if (!valueAssigned)
        {
            YI_LOGE(LOG_TAG, "SelectAudioTrack did not receive a response from the web messaging bridge!");
        }
        else if (response.HasError())
        {
            YI_LOGE(LOG_TAG, "%s", response.GetError()->GetMessage().GetData());
        }
        else
        {
            const yi::rapidjson::Value *pData = response.GetResult();

            if (!pData->IsBool())
            {
                YI_LOGE(LOG_TAG, "SelectAudioTrack expected a boolean type for result, received %s. JSON string for result: %s", CYIRapidJSONUtility::TypeToString(pData->GetType()).GetData(), CYIRapidJSONUtility::CreateStringFromValue(*pData).GetData());
            }
            else
            {
                return pData->GetBool();
            }
        }
    }

    return false;
}

std::vector<CYIAbstractVideoPlayer::AudioTrackInfo> CYIVideojsVideoPlayerPriv::GetAudioTracks() const
{
    return m_audioTracks;
}

CYIAbstractVideoPlayer::AudioTrackInfo CYIVideojsVideoPlayerPriv::GetActiveAudioTrack() const
{
    static const char *FUNCTION_NAME = "getActiveAudioTrack";

    CYIAbstractVideoPlayer::AudioTrackInfo audioTrackInfo(0);

    bool messageSent = false;
    CYIWebMessagingBridge::FutureResponse futureResponse = CallPlayerInstanceFunction(yi::rapidjson::Document(), FUNCTION_NAME, yi::rapidjson::Value(yi::rapidjson::kArrayType), &messageSent);

    if (!messageSent)
    {
        YI_LOGE(LOG_TAG, "Failed to invoke %s function.", FUNCTION_NAME);
    }
    else
    {
        bool valueAssigned = false;
        CYIWebMessagingBridge::Response response = futureResponse.Take(CYIWebMessagingBridge::DEFAULT_RESPONSE_TIMEOUT_MS, &valueAssigned);

        if (!valueAssigned)
        {
            YI_LOGE(LOG_TAG, "GetActiveAudioTrack did not receive a response from the web messaging bridge!");
        }
        else if (response.HasError())
        {
            YI_LOGE(LOG_TAG, "%s", response.GetError()->GetMessage().GetData());
        }
        else
        {
            const yi::rapidjson::Value *pData = response.GetResult();

            if (!pData->IsObject())
            {
                YI_LOGE(LOG_TAG, "GetActiveAudioTrack expected an object type for result, received %s. JSON string for result: %s", CYIRapidJSONUtility::TypeToString(pData->GetType()).GetData(), CYIRapidJSONUtility::CreateStringFromValue(*pData).GetData());
            }
            else
            {
                if (!ConvertValueToTrackInfo(*pData, audioTrackInfo))
                {
                    YI_LOGW(LOG_TAG, "GetActiveAudioTrack data is invalid. JSON string for track data: %s", CYIRapidJSONUtility::CreateStringFromValue(*pData).GetData());
                }
            }
        }
    }

    return audioTrackInfo;
}

bool CYIVideojsVideoPlayerPriv::IsMuted() const
{
    static const char *FUNCTION_NAME = "isMuted";

    bool messageSent = false;
    CYIWebMessagingBridge::FutureResponse futureResponse = CallPlayerInstanceFunction(yi::rapidjson::Document(), FUNCTION_NAME, yi::rapidjson::Value(yi::rapidjson::kArrayType), &messageSent);

    if (!messageSent)
    {
        YI_LOGE(LOG_TAG, "Failed to invoke %s function.", FUNCTION_NAME);
    }
    else
    {
        bool valueAssigned = false;
        CYIWebMessagingBridge::Response response = futureResponse.Take(CYIWebMessagingBridge::DEFAULT_RESPONSE_TIMEOUT_MS, &valueAssigned);

        if (!valueAssigned)
        {
            YI_LOGE(LOG_TAG, "IsMuted did not receive a response from the web messaging bridge!");
        }
        else if (response.HasError())
        {
            YI_LOGE(LOG_TAG, "%s", response.GetError()->GetMessage().GetData());
        }
        else
        {
            const yi::rapidjson::Value *pData = response.GetResult();

            if (!pData->IsBool())
            {
                YI_LOGE(LOG_TAG, "IsMuted expected a boolean type for result, received %s. JSON string for result: %s", CYIRapidJSONUtility::TypeToString(pData->GetType()).GetData(), CYIRapidJSONUtility::CreateStringFromValue(*pData).GetData());
            }
            else
            {
                return pData->GetBool();
            }
        }
    }

    return false;
}

void CYIVideojsVideoPlayerPriv::Mute(bool mute)
{
    static const char *MUTE_FUNCTION_NAME = "mute";
    static const char *UNMUTE_FUNCTION_NAME = "unmute";

    bool messageSent = false;
    CYIWebMessagingBridge::FutureResponse futureResponse = CallPlayerInstanceFunction(yi::rapidjson::Document(), mute ? MUTE_FUNCTION_NAME: UNMUTE_FUNCTION_NAME, yi::rapidjson::Value(yi::rapidjson::kArrayType), &messageSent);

    if (!messageSent)
    {
        YI_LOGE(LOG_TAG, "Failed to invoke %s function.", mute ? MUTE_FUNCTION_NAME: UNMUTE_FUNCTION_NAME);
    }
    else
    {
        bool valueAssigned = false;
        CYIWebMessagingBridge::Response response = futureResponse.Take(CYIWebMessagingBridge::DEFAULT_RESPONSE_TIMEOUT_MS, &valueAssigned);

        if (!valueAssigned)
        {
            YI_LOGE(LOG_TAG, "DisableTextTrack did not receive a response from the web messaging bridge!");
        }
        else if (response.HasError())
        {
            YI_LOGE(LOG_TAG, "%s", response.GetError()->GetMessage().GetData());
        }
    }
}

bool CYIVideojsVideoPlayerPriv::IsTextTrackEnabled() const
{
    static const char *FUNCTION_NAME = "isTextTrackEnabled";

    bool messageSent = false;
    CYIWebMessagingBridge::FutureResponse futureResponse = CallPlayerInstanceFunction(yi::rapidjson::Document(), FUNCTION_NAME, yi::rapidjson::Value(yi::rapidjson::kArrayType), &messageSent);

    if (!messageSent)
    {
        YI_LOGE(LOG_TAG, "Failed to invoke %s function.", FUNCTION_NAME);
    }
    else
    {
        bool valueAssigned = false;
        CYIWebMessagingBridge::Response response = futureResponse.Take(CYIWebMessagingBridge::DEFAULT_RESPONSE_TIMEOUT_MS, &valueAssigned);

        if (!valueAssigned)
        {
            YI_LOGE(LOG_TAG, "IsTextTrackEnabled did not receive a response from the web messaging bridge!");
        }
        else if (response.HasError())
        {
            YI_LOGE(LOG_TAG, "%s", response.GetError()->GetMessage().GetData());
        }
        else
        {
            const yi::rapidjson::Value *pData = response.GetResult();

            if (!pData->IsBool())
            {
                YI_LOGE(LOG_TAG, "IsTextTrackEnabled expected a boolean type for result, received %s. JSON string for result: %s", CYIRapidJSONUtility::TypeToString(pData->GetType()).GetData(), CYIRapidJSONUtility::CreateStringFromValue(*pData).GetData());
            }
            else
            {
                return pData->GetBool();
            }
        }
    }

    return false;
}

void CYIVideojsVideoPlayerPriv::EnableTextTrack()
{
    static const char *FUNCTION_NAME = "enableTextTrack";

    bool messageSent = false;
    CYIWebMessagingBridge::FutureResponse futureResponse = CallPlayerInstanceFunction(yi::rapidjson::Document(), FUNCTION_NAME, yi::rapidjson::Value(yi::rapidjson::kArrayType), &messageSent);

    if (!messageSent)
    {
        YI_LOGE(LOG_TAG, "Failed to invoke %s function.", FUNCTION_NAME);
    }
    else
    {
        bool valueAssigned = false;
        CYIWebMessagingBridge::Response response = futureResponse.Take(CYIWebMessagingBridge::DEFAULT_RESPONSE_TIMEOUT_MS, &valueAssigned);

        if (!valueAssigned)
        {
            YI_LOGE(LOG_TAG, "EnableTextTrack did not receive a response from the web messaging bridge!");
        }
        else if (response.HasError())
        {
            YI_LOGE(LOG_TAG, "%s", response.GetError()->GetMessage().GetData());
        }
    }
}

void CYIVideojsVideoPlayerPriv::DisableTextTrack()
{
    static const char *FUNCTION_NAME = "disableTextTrack";

    bool messageSent = false;
    CYIWebMessagingBridge::FutureResponse futureResponse = CallPlayerInstanceFunction(yi::rapidjson::Document(), FUNCTION_NAME, yi::rapidjson::Value(yi::rapidjson::kArrayType), &messageSent);

    if (!messageSent)
    {
        YI_LOGE(LOG_TAG, "Failed to invoke %s function.", FUNCTION_NAME);
    }
    else
    {
        bool valueAssigned = false;
        CYIWebMessagingBridge::Response response = futureResponse.Take(CYIWebMessagingBridge::DEFAULT_RESPONSE_TIMEOUT_MS, &valueAssigned);

        if (!valueAssigned)
        {
            YI_LOGE(LOG_TAG, "DisableTextTrack did not receive a response from the web messaging bridge!");
        }
        else if (response.HasError())
        {
            YI_LOGE(LOG_TAG, "%s", response.GetError()->GetMessage().GetData());
        }
    }
}

bool CYIVideojsVideoPlayerPriv::SelectTextTrack(uint32_t id, bool enableTextTrack)
{
    static const char *FUNCTION_NAME = "selectTextTrack";

    yi::rapidjson::Document command(yi::rapidjson::kObjectType);
    yi::rapidjson::MemoryPoolAllocator<yi::rapidjson::CrtAllocator> &allocator = command.GetAllocator();

    yi::rapidjson::Value arguments(yi::rapidjson::kArrayType);
    arguments.PushBack(yi::rapidjson::Value(id), allocator);
    arguments.PushBack(yi::rapidjson::Value().SetBool(enableTextTrack), allocator);

    bool messageSent = false;
    CYIWebMessagingBridge::FutureResponse futureResponse = CallPlayerInstanceFunction(std::move(command), FUNCTION_NAME, std::move(arguments), &messageSent);

    if (!messageSent)
    {
        YI_LOGE(LOG_TAG, "Failed to invoke %s function.", FUNCTION_NAME);
    }
    else
    {
        bool valueAssigned = false;
        CYIWebMessagingBridge::Response response = futureResponse.Take(CYIWebMessagingBridge::DEFAULT_RESPONSE_TIMEOUT_MS, &valueAssigned);

        if (!valueAssigned)
        {
            YI_LOGE(LOG_TAG, "SelectTextTrack did not receive a response from the web messaging bridge!");
        }
        else if (response.HasError())
        {
            YI_LOGE(LOG_TAG, "%s", response.GetError()->GetMessage().GetData());
        }
        else
        {
            const yi::rapidjson::Value *pData = response.GetResult();

            if (!pData->IsBool())
            {
                YI_LOGE(LOG_TAG, "SelectTextTrack expected a boolean type for result, received %s. JSON string for result: %s", CYIRapidJSONUtility::TypeToString(pData->GetType()).GetData(), CYIRapidJSONUtility::CreateStringFromValue(*pData).GetData());
            }
            else
            {
                return pData->GetBool();
            }
        }
    }

    return false;
}

std::vector<CYIAbstractVideoPlayer::ClosedCaptionsTrackInfo> CYIVideojsVideoPlayerPriv::GetTextTracks() const
{
    return m_textTracks;
}

CYIAbstractVideoPlayer::ClosedCaptionsTrackInfo CYIVideojsVideoPlayerPriv::GetActiveTextTrack() const
{
    static const char *FUNCTION_NAME = "getActiveTextTrack";

    CYIAbstractVideoPlayer::ClosedCaptionsTrackInfo textTrackInfo(0);

    bool messageSent = false;
    CYIWebMessagingBridge::FutureResponse futureResponse = CallPlayerInstanceFunction(yi::rapidjson::Document(), FUNCTION_NAME, yi::rapidjson::Value(yi::rapidjson::kArrayType), &messageSent);

    if (!messageSent)
    {
        YI_LOGE(LOG_TAG, "Failed to invoke %s function.", FUNCTION_NAME);
    }
    else
    {
        bool valueAssigned = false;
        CYIWebMessagingBridge::Response response = futureResponse.Take(CYIWebMessagingBridge::DEFAULT_RESPONSE_TIMEOUT_MS, &valueAssigned);

        if (!valueAssigned)
        {
            YI_LOGE(LOG_TAG, "GetActiveTextTrack did not receive a response from the web messaging bridge!");
        }
        else if (response.HasError())
        {
            YI_LOGE(LOG_TAG, "%s", response.GetError()->GetMessage().GetData());
        }
        else
        {
            const yi::rapidjson::Value *pData = response.GetResult();

            if (!pData->IsObject())
            {
                YI_LOGE(LOG_TAG, "GetActiveTextTrack expected an object type for result, received %s. JSON string for result: %s", CYIRapidJSONUtility::TypeToString(pData->GetType()).GetData(), CYIRapidJSONUtility::CreateStringFromValue(*pData).GetData());
            }
            else
            {
                if (!ConvertValueToTrackInfo(*pData, textTrackInfo))
                {
                    YI_LOGW(LOG_TAG, "GetActiveTextTrack data is invalid. JSON string for track data: %s", CYIRapidJSONUtility::CreateStringFromValue(*pData).GetData());
                }
            }
        }
    }

    return textTrackInfo;
}

void CYIVideojsVideoPlayerPriv::AddExternalTextTrack(const CYIString &url, const CYIString &language, const CYIString &label, const CYIString &type, const CYIString &format, bool enable)
{
    static const char *FUNCTION_NAME = "addExternalTextTrack";

    yi::rapidjson::Document command(yi::rapidjson::kObjectType);
    yi::rapidjson::MemoryPoolAllocator<yi::rapidjson::CrtAllocator> &allocator = command.GetAllocator();

    yi::rapidjson::Value arguments(yi::rapidjson::kArrayType);
    yi::rapidjson::Value textTrackDataValue(yi::rapidjson::kObjectType);

    yi::rapidjson::Value urlValue(url.GetData(), allocator);
    yi::rapidjson::Value languageValue(language.GetData(), allocator);
    yi::rapidjson::Value labelValue(label.GetData(), allocator);
    yi::rapidjson::Value typeValue(type.GetData(), allocator);
    yi::rapidjson::Value formatValue(format.GetData(), allocator);

    textTrackDataValue.AddMember(yi::rapidjson::StringRef("url"), urlValue, allocator);
    textTrackDataValue.AddMember(yi::rapidjson::StringRef("language"), languageValue, allocator);
    textTrackDataValue.AddMember(yi::rapidjson::StringRef("label"), labelValue, allocator);
    textTrackDataValue.AddMember(yi::rapidjson::StringRef("type"), typeValue, allocator);
    textTrackDataValue.AddMember(yi::rapidjson::StringRef("format"), formatValue, allocator);
    textTrackDataValue.AddMember(yi::rapidjson::StringRef("enable"), yi::rapidjson::Value(enable), allocator);

    arguments.PushBack(textTrackDataValue, allocator);

    bool messageSent = false;
    CYIWebMessagingBridge::FutureResponse futureResponse = CallPlayerInstanceFunction(std::move(command), FUNCTION_NAME, std::move(arguments), &messageSent);

    if (!messageSent)
    {
        YI_LOGE(LOG_TAG, "Failed to invoke %s function.", FUNCTION_NAME);
    }
    else
    {
        bool valueAssigned = false;
        CYIWebMessagingBridge::Response response = futureResponse.Take(CYIWebMessagingBridge::DEFAULT_RESPONSE_TIMEOUT_MS, &valueAssigned);

        if (!valueAssigned)
        {
            YI_LOGE(LOG_TAG, "AddExternalTextTrack did not receive a response from the web messaging bridge!");
        }
        else if (response.HasError())
        {
            YI_LOGE(LOG_TAG, "%s", response.GetError()->GetMessage().GetData());
        }
    }
}

CYIAbstractVideoPlayer::TimedMetadataInterface *CYIVideojsVideoPlayerPriv::GetTimedMetadataInterface() const
{
    return const_cast<CYIVideojsVideoPlayerPriv *>(this);
}

CYIVideojsVideoPlayer *CYIVideojsVideoPlayer::Create(const std::map<CYIString, CYIString> &playerConfiguration)
{
    yi::rapidjson::Document playerConfigurationDocument(yi::rapidjson::kObjectType);
    yi::rapidjson::MemoryPoolAllocator<yi::rapidjson::CrtAllocator> &allocator = playerConfigurationDocument.GetAllocator();

    for(std::map<CYIString, CYIString>::const_iterator playerConfigIterator = playerConfiguration.begin(); playerConfigIterator != playerConfiguration.end(); playerConfigIterator++)
    {
        if(playerConfigIterator->first.IsEmpty()) {
            continue;
        }

        yi::rapidjson::Value configKeyValue(playerConfigIterator->first.GetData(), allocator);
        yi::rapidjson::Value configValueValue(playerConfigIterator->second.GetData(), allocator);
        playerConfigurationDocument.AddMember(configKeyValue, configValueValue, allocator);
    }

    return CYIVideojsVideoPlayer::Create(std::move(playerConfigurationDocument));
}

CYIVideojsVideoPlayer *CYIVideojsVideoPlayer::Create(yi::rapidjson::Document &&playerConfiguration)
{
    CYIVideojsVideoPlayer *pThis = new CYIVideojsVideoPlayer();
    pThis->m_pPriv = new CYIVideojsVideoPlayerPriv(pThis, std::move(playerConfiguration)); 
    if(!CYIWebBridgeLocator::GetWebMessagingBridge())
    {
        YI_LOGE(LOG_TAG, "CYIVideojsVideoPlayer is not available on this platform or platform configuration.");

        delete pThis;
        pThis = nullptr;
        return nullptr;
    }

    return pThis;
}

CYIVideojsVideoPlayer::~CYIVideojsVideoPlayer()
{
    delete m_pPriv;
    m_pPriv = nullptr;
}

void CYIVideojsVideoPlayer::Init_()
{
    m_pPriv->Init();
}

CYIString CYIVideojsVideoPlayer::GetName_() const
{
    return m_pPriv->GetName();
}

CYIString CYIVideojsVideoPlayer::GetNickname() const
{
    return m_pPriv->GetNickname();
}

CYIString CYIVideojsVideoPlayer::GetVersion_() const
{
    return m_pPriv->GetVersion();
}

CYIAbstractVideoPlayer::Statistics CYIVideojsVideoPlayer::GetStatistics_() const
{
    return m_pPriv->GetStatistics();
}

void CYIVideojsVideoPlayer::SetNickname(const CYIString &nickname) const
{
    m_pPriv->SetNickname(nickname);
}

std::unique_ptr<CYIVideoSurface> CYIVideojsVideoPlayer::CreateSurface_()
{
    return m_pPriv->CreateSurface();
}

bool CYIVideojsVideoPlayer::SupportsFormat_(StreamingFormat streamingFormat, CYIAbstractVideoPlayer::DRMScheme drmScheme) const
{
    return m_pPriv->SupportsFormat(streamingFormat, drmScheme);
}

bool CYIVideojsVideoPlayer::HasNativeStartTimeHandling_() const
{
    return true;
}

bool CYIVideojsVideoPlayer::HasNativeBitrateEventHandling_() const
{
    return true;
}

void CYIVideojsVideoPlayer::Prepare_(const CYIUrl &videoURI, StreamingFormat format)
{
    m_pPriv->Prepare(videoURI, format);
}

void CYIVideojsVideoPlayer::Play_()
{
    m_pPriv->Play();
}

void CYIVideojsVideoPlayer::Pause_()
{
    m_pPriv->Pause();
}

void CYIVideojsVideoPlayer::Stop_()
{
    m_pPriv->Stop();
    Finalized();
}

uint64_t CYIVideojsVideoPlayer::GetDurationMs_() const
{
    return m_pPriv->GetDurationMs();
}

uint64_t CYIVideojsVideoPlayer::GetCurrentTimeMs_() const
{
    return m_pPriv->GetCurrentTimeMs();
}

std::vector<CYIAbstractVideoPlayer::SeekableRange> CYIVideojsVideoPlayer::GetLiveSeekableRanges_() const
{
    // note: not implemented
    return std::vector<CYIAbstractVideoPlayer::SeekableRange>();
}

void CYIVideojsVideoPlayer::Seek_(uint64_t seekPositionMS)
{
    m_pPriv->Seek(seekPositionMS);
}

bool CYIVideojsVideoPlayer::SelectAudioTrack_(uint32_t id)
{
    return m_pPriv->SelectAudioTrack(id);
}

std::vector<CYIAbstractVideoPlayer::AudioTrackInfo> CYIVideojsVideoPlayer::GetAudioTracks_() const
{
    return m_pPriv->GetAudioTracks();
}

CYIAbstractVideoPlayer::AudioTrackInfo CYIVideojsVideoPlayer::GetActiveAudioTrack_() const
{
    return m_pPriv->GetActiveAudioTrack();
}

bool CYIVideojsVideoPlayer::IsMuted_() const
{
    return m_pPriv->IsMuted();
}

void CYIVideojsVideoPlayer::Mute_(bool mute)
{
    m_pPriv->Mute(mute);
}

void CYIVideojsVideoPlayer::DisableClosedCaptions_()
{
    m_pPriv->DisableTextTrack();
}

bool CYIVideojsVideoPlayer::SelectClosedCaptionsTrack_(uint32_t textID)
{
    return m_pPriv->SelectTextTrack(textID);
}

std::vector<CYIAbstractVideoPlayer::ClosedCaptionsTrackInfo> CYIVideojsVideoPlayer::GetClosedCaptionsTracks_() const
{
    return m_pPriv->GetTextTracks();
}

CYIAbstractVideoPlayer::ClosedCaptionsTrackInfo CYIVideojsVideoPlayer::GetActiveClosedCaptionsTrack_() const
{
    return m_pPriv->GetActiveTextTrack();
}

void CYIVideojsVideoPlayer::AddExternalTextTrack(const CYIString &url, const CYIString &language, const CYIString &label, const CYIString &type, const CYIString &format, bool enable)
{
    m_pPriv->AddExternalTextTrack(url, language, label, type, format, enable);
}

void CYIVideojsVideoPlayer::SetMaxBitrate_(uint64_t maxBitrate)
{
    YI_UNUSED(maxBitrate);
    // NOTE: Some MSE/EME video players require the bitrate to be set prior to loading content, so the max bitrate is instead set during prepare
}

CYIAbstractVideoPlayer::TimedMetadataInterface *CYIVideojsVideoPlayer::GetTimedMetadataInterface_() const
{
    return m_pPriv->GetTimedMetadataInterface();
}
