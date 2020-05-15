#include "AutomatedPlayerTesterApp.h"

#include <player/YiAbstractVideoPlayer.h>
#include <player/YiDefaultVideoPlayerFactory.h>
#include <signal/YiSignal.h>

#define LOG_TAG "AutomatedPlayerTesterApp"

void AutomatedPlayerTesterApp::PlayerTest::SetupSignals()
{
    if (!m_pVideoPlayer)
    {
        YI_LOGF(PLAYER_TEST_TAG, "The player was null so won't be able to attach signals. Aborting test.");
    }
    m_pVideoPlayer->Ready.Connect(*this, &PlayerTest::OnVideoReady, EYIConnectionType::Async);
    m_pVideoPlayer->Playing.Connect(*this, &PlayerTest::OnVideoPlaying, EYIConnectionType::Async);
    m_pVideoPlayer->Paused.Connect(*this, &PlayerTest::OnVideoPaused, EYIConnectionType::Async);
    m_pVideoPlayer->Finalized.Connect(*this, &PlayerTest::OnVideoStopped, EYIConnectionType::Async);
    m_pVideoPlayer->PlaybackComplete.Connect(*this, &PlayerTest::OnPlaybackComplete, EYIConnectionType::Async);
    m_pVideoPlayer->ErrorOccurred.Connect(*this, &PlayerTest::OnPlayerError, EYIConnectionType::Async);

    this->m_waitTimer.TimedOut.Connect(*this, &PlayerTest::OnWaitTimerEnded, EYIConnectionType::Async);
    this->m_playbackTimer.TimedOut.Connect(*this, &PlayerTest::OnPlaybackTimerEnded, EYIConnectionType::Async);
}

void AutomatedPlayerTesterApp::PlayerTest::CleanUpSignals()
{
    m_pVideoPlayer->Ready.Disconnect(*this);
    m_pVideoPlayer->Playing.Disconnect(*this);
    m_pVideoPlayer->Paused.Disconnect(*this);
    m_pVideoPlayer->Finalized.Disconnect(*this);
    m_pVideoPlayer->PlaybackComplete.Disconnect(*this);
    m_pVideoPlayer->ErrorOccurred.Disconnect(*this);

    this->m_waitTimer.TimedOut.Disconnect(*this);
    this->m_playbackTimer.TimedOut.Disconnect(*this);
}

void AutomatedPlayerTesterApp::PlayerTest::DoStep()
{
    uint16_t actionToPerform = GetCurrentTestStep().action;
    if (actionToPerform == ACTION_PREPARE ||
        actionToPerform == ACTION_PREPARE_START_TIME_10 ||
        actionToPerform == ACTION_PREPARE_START_TIME_30 ||
        actionToPerform == ACTION_PREPARE_START_TIME_END ||
        actionToPerform == ACTION_PREPARE_START_TIME_PAST_DURATION ||
        actionToPerform == ACTION_PREPARE_START_TIME_MATH_MINUS ||
        actionToPerform == ACTION_GET_DURATION)
    {
        uint64_t startTime = 0;
        if (actionToPerform == ACTION_PREPARE_START_TIME_10)
        {
            m_expectedTime = 10000;
            startTime = 10000;
        }
        else if (actionToPerform == ACTION_PREPARE_START_TIME_30)
        {
            m_expectedTime = 30000;
            startTime = 30000;
        }
        else if (actionToPerform == ACTION_PREPARE_START_TIME_END ||
                 actionToPerform == ACTION_PREPARE_START_TIME_PAST_DURATION ||
                 actionToPerform == ACTION_PREPARE_START_TIME_MATH_MINUS)
        {
            // All the above tests result in setting the video to 1ms less than it's full duration.
            m_expectedTime = m_videoDuration - 1;
            if (actionToPerform == ACTION_PREPARE_START_TIME_END)
            {
                startTime = m_videoDuration;
            }
            else if (actionToPerform == ACTION_PREPARE_START_TIME_PAST_DURATION)
            {
                startTime = m_videoDuration + 5000;
            }
            else if (actionToPerform == ACTION_PREPARE_START_TIME_MATH_MINUS)
            {
                startTime = m_expectedTime - 5000;
            }
        }
        playerTester_.PrepareVideo(m_testUrl, startTime);
    }
    else if (actionToPerform == ACTION_PLAY ||
             actionToPerform == ACTION_PLAY_SHORT ||
             actionToPerform == ACTION_PLAY_END)
    {
        if (actionToPerform == ACTION_PLAY_SHORT)
        {
            m_playbackTimer.Start(10000); // 10 seconds
        }
        else if (actionToPerform == ACTION_PLAY_END && GetCurrentTestStep().evaluator == EVALUATOR_EVALUATE)
        {
            // Max is Duration - Current Time in playback and padding 5 seconds for good measure.
            uint64_t playbackTimeout = m_pVideoPlayer->GetDurationMs() + 5000;
            this->ResetTestTimer(playbackTimeout);
            m_playbackTimer.Start(playbackTimeout); // Do I even need this?
        }
        m_pVideoPlayer->Play();
    }
    else if (actionToPerform == ACTION_STOP)
    {
        m_pVideoPlayer->Stop();
    }
    else if (actionToPerform == ACTION_PAUSE)
    {
        m_pVideoPlayer->Pause();
    }
    else if (actionToPerform == ACTION_SEEK_FORWARD ||
             actionToPerform == ACTION_SEEK_BACKWARD ||
             actionToPerform == ACTION_SEEK_END ||
             actionToPerform == ACTION_SEEK_FRONT)
    {
        uint64_t newTime;
        if (actionToPerform == ACTION_SEEK_FORWARD)
        {
            newTime = m_pVideoPlayer->GetCurrentTimeMs() + 5000;
            m_expectedTime = m_expectedTime + 5000;
        }
        else if (actionToPerform == ACTION_SEEK_BACKWARD)
        {
            newTime = m_pVideoPlayer->GetCurrentTimeMs() - 5000;
            m_expectedTime = m_expectedTime - 5000;
        }
        else if (actionToPerform == ACTION_SEEK_END)
        {
            newTime = m_pVideoPlayer->GetDurationMs();
            m_expectedTime = m_pVideoPlayer->GetDurationMs() - 1;
        }
        else // ACTION_SEEK_FRONT
        {
            newTime = 0;
            m_expectedTime = 0;
        }
        m_waitTimer.Start(1000);
        m_pVideoPlayer->Seek(newTime);
    }
    else if (actionToPerform == ACTION_WAIT_SHORT)
    {
        m_waitTimer.Start(5000);
    }
    else if (actionToPerform == ACTION_IS_EQUAL)
    {
        if (GetCurrentTestStep().evaluator == EVALUATOR_DEFAULT_STATISTICS)
        {
            CYIAbstractVideoPlayer::Statistics defaults;
            if (m_pVideoPlayer->GetStatistics() == defaults)
            {
                SetResult(true);
            }
            else
            {
                SetResult(false, "Expected player statistics to equal the defaults, but they did not.");
            }
        }
        StepCompleted.Emit();
    }
    else if (actionToPerform == ACTION_IS_NOT_EQUAL)
    {
        if (GetCurrentTestStep().evaluator == EVALUATOR_DEFAULT_STATISTICS)
        {
            CYIAbstractVideoPlayer::Statistics defaults;
            if (m_pVideoPlayer->GetStatistics() == defaults)
            {
                SetResult(false, "Expected player statistics to not equal the defaults, but they did.");
            }
            else
            {
                SetResult(true);
            }
        }
        StepCompleted.Emit();
    }
    else if (actionToPerform == ACTION_CHECK_LIVE)
    {
        bool currentLiveStat = m_pVideoPlayer->GetStatistics().isLive;
        if ((GetCurrentTestStep().evaluator == EVALUATOR_EXPECTED_TRUE && currentLiveStat) ||
            (GetCurrentTestStep().evaluator == EVALUATOR_EXPECTED_FALSE && !currentLiveStat))
        {
            SetResult(true);
        }
        else
        {
            SetResult(false, "Video stream's IsLive statistic did not match expectations.");
        }
        StepCompleted.Emit();
    }
    else
    {
        //Unknown step
        YI_ASSERT(false, PLAYER_TEST_TAG, "Unknown step passed into DoStep: %i . Test setup is likely incorrect!", actionToPerform);
    }
}

void AutomatedPlayerTesterApp::PlayerTest::OnVideoReady()
{
    AbstractAutoTest::TestStep currentStep = GetCurrentTestStep();
    if (currentStep.action == ACTION_PREPARE && currentStep.evaluator == EVALUATOR_EXPECTED_ERROR)
    {
        // The Prepare Fail tests will prepare a video and expect it to fail.
        SetResult(false, "Expected an error to occur but it did not.");
        StepCompleted.Emit();
    }
    else if (currentStep.action == ACTION_GET_DURATION)
    {
        m_videoDuration = m_pVideoPlayer->GetDurationMs();
        m_pVideoPlayer->Stop();
    }
    else
    {
        StepCompleted.Emit();
    }
}

void AutomatedPlayerTesterApp::PlayerTest::OnVideoPlaying()
{
    if (GetCurrentTestStep().action == ACTION_PLAY_END ||
        GetCurrentTestStep().action == ACTION_PLAY_SHORT)
    {
        // Skip out of this slot because the evaluation/advancement for those two steps are handled elsewhere.
        return;
    }
    if (GetCurrentTestStep().action == ACTION_PLAY)
    {
        if (GetCurrentTestStep().evaluator == EVALUATOR_CURRENT_TIME)
        {
            // Compare with the m_expectedTime
            uint64_t currentTime = m_pVideoPlayer->GetCurrentTimeMs();
            if (currentTime != m_expectedTime)
            {
                // Check if it is in acceptable range.
                CYIString logMessage = CYIString("Current Time: ") + currentTime + " vs. Expected Time: " + m_expectedTime;
                YI_LOGD(PLAYER_TEST_TAG, "%s", logMessage.GetData());
#if defined(YI_UWP)
                uint64_t acceptableUpperBound = m_expectedTime + 50;
                uint64_t acceptableLowerBound = m_expectedTime - 50;
#else
                uint64_t acceptableUpperBound = m_expectedTime + 32;
                uint64_t acceptableLowerBound = m_expectedTime - 32;
#endif
                if (currentTime < acceptableUpperBound && currentTime > acceptableLowerBound)
                {
                    SetResult(true);
                }
                else
                {
                    CYIString message = CYIString("Current time was: ") + currentTime + " while expected time was: " + m_expectedTime;
                    SetResult(false, message);
                }
            }
            else
            {
                SetResult(true);
            }
        }
        else if (GetCurrentTestStep().evaluator == EVALUATOR_EXPECTED_NO_ERROR)
        {
            // Very basic check of making sure the video doesn't throw an error when playing.
            SetResult(true);
        }
        StepCompleted.Emit();
    }
}

void AutomatedPlayerTesterApp::PlayerTest::OnVideoPaused()
{
    if (GetCurrentTestStep().action == ACTION_PAUSE)
    {
        StepCompleted.Emit();
    }
    else
    {
        YI_LOGD(PLAYER_TEST_TAG, "Unexpected Player PAUSE occurred. This may impact test results.");
    }
}

void AutomatedPlayerTesterApp::PlayerTest::OnVideoStopped()
{
    if ((GetCurrentTestStep().action == ACTION_STOP && GetCurrentTestStep().evaluator == EVALUATOR_NONE) ||
        GetCurrentTestStep().action == ACTION_GET_DURATION)
    {
        StepCompleted.Emit();
    }
    else
    {
        YI_LOGD(PLAYER_TEST_TAG, "Unexpected Player STOP occurred. This may impact test results.");
    }
}

void AutomatedPlayerTesterApp::PlayerTest::OnPlayerError(CYIAbstractVideoPlayer::Error error)
{
    m_pVideoPlayer->Stop();
    if (this->GetCurrentTestStep().evaluator == EVALUATOR_EXPECTED_ERROR)
    {
        SetResult(true);
        StepCompleted.Emit();
    }
    else
    {
        CYIString resultMessage = CYIString("Video Player Error: ") + error.message;
        SetResult(false, resultMessage.GetData());
        TestCompleted.Emit();
    }
}

void AutomatedPlayerTesterApp::PlayerTest::OnPlaybackComplete()
{
    if ((this->GetCurrentTestStep().action == ACTION_PLAY_SHORT || this->GetCurrentTestStep().action == ACTION_PLAY_END) && this->GetCurrentTestStep().evaluator == EVALUATOR_EVALUATE)
    {
        m_playbackTimer.Stop();
        if (this->GetCurrentTestStep().action == ACTION_PLAY_SHORT && m_pVideoPlayer->GetDurationMs() > 10000)
        {
            // For ShortPlayback tests we shouldn't hit the PlaybackComplete because we should abort early.
            SetResult(false, "Expected the video to run to 10 seconds but the Playback Complete was fired early.");
        }
        else
        {
            SetResult(true);
        }
        StepCompleted.Emit();
    }
    else if (this->GetCurrentTestStep().action == ACTION_PLAY_END && this->GetCurrentTestStep().evaluator == EVALUATOR_NONE)
    {
        StepCompleted.Emit();
    }
}

void AutomatedPlayerTesterApp::PlayerTest::OnPlaybackTimerEnded()
{
    /* This signal should only fire if we hit the 10 second limit for short playback tests (which is good), if we have a short playback in one of the tests (just an action) or we exceeded the expected duration of a video while doing the full playback tests - which is bad. */
    if (this->GetCurrentTestStep().action == ACTION_PLAY_SHORT && this->GetCurrentTestStep().evaluator == EVALUATOR_EVALUATE)
    {
        // We successfully hit 10 seconds.
        SetResult(true);
    }
    else if (this->GetCurrentTestStep().action == ACTION_PLAY_END)
    {
        // Hitting the timer here means we've been playing for longer than the expected duration.
        SetResult(false, "Expected PlaybackComplete to be fired but instead the playback timer timedout. This means the playback failed or stalled.");
    }
    StepCompleted.Emit();
}

void AutomatedPlayerTesterApp::PlayerTest::OnWaitTimerEnded()
{
    StepCompleted.Emit();
}

/* AUTOMATED_PLAYER_TEST_APP
    Main entry point for the automated test suite building, kick off and finalizing.
 */
bool AutomatedPlayerTesterApp::UserInit()
{
    bool initOk = PlayerTesterApp::UserInit();
    if (initOk)
    {
        m_testIndex = 0;
        AbstractAutomatedTestHarness::Init();
        InitializeTests(); // This needs to be done sooner because we need it for the AutoTest panel.
    }
    return initOk;
}

void AutomatedPlayerTesterApp::InitializeTests()
{
    UrlAndFormat localFileTestUrl;
    UrlAndFormat liveVideoTestUrl;
    UrlAndFormat seekVideoTestUrl;

    bool seekVideoSet = false;
    bool liveVideoSet = false;
    bool localFileSet = false;

    m_PlayerTests.clear();

    std::vector<UrlAndFormat> urlsAvailableOnPlatform = GetPossibleUrls();

    for (const UrlAndFormat testUrl : urlsAvailableOnPlatform)
    {
        if (testUrl.isErrorUrl)
        {
            /* PREPARE FAIL TESTS
             These tests will intentionally load and attempt to prepare bad Urls. These Urls are either incorrect, invalid, point to non-existing streams or contain unsupported video formats for the given platform.
             If any of the videos load correctly, there may be something wrong with our error handling or             the video may be available when it previously was not. Double check the Url and update the UrlAndFormat.isErrorUrl property for that instance or remove the Url if it offers no new             value.
             */
            {
                std::unique_ptr<PlayerTest> temp = std::make_unique<PlayerTest>(*this, "Prepare Fail Test: " + testUrl.name, "PrepareFail");
                temp->m_testUrl = testUrl;
                temp->AddTestStep(PlayerTest::TestStep(ACTION_PREPARE, EVALUATOR_EXPECTED_ERROR));
                m_PlayerTests.push_back(std::move(temp));
            }
        }
        else
        {
            // We need specific attributes on the streams for specific test suites. If they're not available, then those suites will be unavailable on that platform.
            if (testUrl.isLive && !liveVideoSet)
            {
                liveVideoTestUrl = testUrl;
                liveVideoSet = true;
            }
#if !defined(YI_VS2017) && !defined(YI_LINUX)
            if (testUrl.isLocalFile && !localFileSet)
            {
                localFileTestUrl = testUrl;
                localFileSet = true;

                /* STATISTICS TEST 1
                    Tracking expected data for each stream would be cumbersome. This test simply ensures that we set the statistics after preparing the video. We'd expect the valures to not be the defaults.
                 */
                {
                    std::unique_ptr<PlayerTest> statTest = std::make_unique<PlayerTest>(*this, "Statistics Test: Local file unprepared - default stats", "StatisticsTest");
                    statTest->m_testUrl = localFileTestUrl;
                    statTest->AddTestStep(AbstractAutoTest::TestStep(ACTION_STOP, EVALUATOR_NONE));
                    statTest->AddTestStep(AbstractAutoTest::TestStep(ACTION_WAIT_SHORT, EVALUATOR_NONE));
                    statTest->AddTestStep(AbstractAutoTest::TestStep(ACTION_IS_EQUAL, EVALUATOR_DEFAULT_STATISTICS));
                    m_PlayerTests.push_back(std::move(statTest));
                }

                {
                    std::unique_ptr<PlayerTest> statTest = std::make_unique<PlayerTest>(*this, "Statistics Test: Local file prepared - not default stats", "StatisticsTest");
                    statTest->m_testUrl = localFileTestUrl;
                    statTest->AddTestStep(AbstractAutoTest::TestStep(ACTION_PREPARE, EVALUATOR_NONE));
                    statTest->AddTestStep(AbstractAutoTest::TestStep(ACTION_WAIT_SHORT, EVALUATOR_NONE));
                    statTest->AddTestStep(AbstractAutoTest::TestStep(ACTION_IS_NOT_EQUAL, EVALUATOR_DEFAULT_STATISTICS));
                    m_PlayerTests.push_back(std::move(statTest));
                }
            }
#else
            YI_UNUSED(localFileSet);
#endif
            if (testUrl.isSuitableForSeekTest && !seekVideoSet)
            {
                seekVideoTestUrl = testUrl;
                seekVideoSet = true;
            }

            /* SHORT PLAYBACK TESTS
             These tests will cycle through all video streams that are supposed (PlayerTesterApp::m_possibleUrls) to be supported on the current platform and do not have expected errors (UrlAndFormat.isErrorUrl), prepare the stream, play back for 10 seconds (or to completion if they video is less than 10 seconds) and stop the video.
             This is a quick sanity for playback the playback for supported platforms.
             If a video fails to load correctly or the stream is no longer accessible from our intranet, a warning will appear in the result suggesting testers confirm this. The failing stream (if it's no longer accessible) must be removed.
             */
            {
                std::unique_ptr<PlayerTest> temp = std::make_unique<PlayerTest>(*this, "Short Play Test: " + testUrl.name, "ShortPlay");
                temp->m_testUrl = testUrl;
                temp->AddTestStep(PlayerTest::TestStep(ACTION_PREPARE, EVALUATOR_NONE));
                temp->AddTestStep(PlayerTest::TestStep(ACTION_PLAY_SHORT, EVALUATOR_EVALUATE));
                temp->AddTestStep(PlayerTest::TestStep(ACTION_STOP, EVALUATOR_NONE));
                m_PlayerTests.push_back(std::move(temp));
            }

            /* FULL PLAYBACK TESTS
             These tests will cycle through all video streams that are supposed (PlayerTesterApp::m_possibleUrls) to be supported on the current platform and do not have expected errors (UrlAndFormat.isErrorUrl),  prepare the stream and then play them to completion.
             */
            /*
            {
                std::unique_ptr<PlayerTest> temp = std::make_unique<PlayerTest>(*this);
                temp->SetTestData("Full Play Test: " + testUrl.name, "FullPlay");
                temp->m_testUrl = testUrl;
                temp->AddTestStep(PlayerTest::TestStep(ACTION_PREPARE, EVALUATOR_NONE));
                temp->AddTestStep(PlayerTest::TestStep(ACTION_PLAY_END, EVALUATOR_EVALUATE));
                temp->AddTestStep(PlayerTest::TestStep(ACTION_STOP, EVALUATOR_NONE));
                m_PlayerTests.push_back(std::move(temp));
            }
            */

            /* STATISTICS TESTS 2
                Poll the VideoPlayer's statistics to determine if the stream is live or not.
             */
#if !defined(YI_VS2017) && !defined(YI_LINUX)
            if (testUrl.isLive)
            {
                {
                    std::unique_ptr<PlayerTest> liveTest = std::make_unique<PlayerTest>(*this, "Statistics Test: Is Live YES - " + testUrl.name, "StatisticsTest");
                    liveTest->m_testUrl = testUrl;
                    liveTest->AddTestStep(AbstractAutoTest::TestStep(ACTION_PREPARE, EVALUATOR_NONE));
                    liveTest->AddTestStep(AbstractAutoTest::TestStep(ACTION_WAIT_SHORT, EVALUATOR_NONE));
                    liveTest->AddTestStep(AbstractAutoTest::TestStep(ACTION_CHECK_LIVE, EVALUATOR_EXPECTED_TRUE));
                    m_PlayerTests.push_back(std::move(liveTest));
                }
            }
            else
            {
                {
                    std::unique_ptr<PlayerTest> liveTest = std::make_unique<PlayerTest>(*this, "Statistics Test: Is Live NO - " + testUrl.name, "StatisticsTest");
                    liveTest->m_testUrl = testUrl;
                    liveTest->AddTestStep(AbstractAutoTest::TestStep(ACTION_PREPARE, EVALUATOR_NONE));
                    liveTest->AddTestStep(AbstractAutoTest::TestStep(ACTION_WAIT_SHORT, EVALUATOR_NONE));
                    liveTest->AddTestStep(AbstractAutoTest::TestStep(ACTION_CHECK_LIVE, EVALUATOR_EXPECTED_FALSE));
                    m_PlayerTests.push_back(std::move(liveTest));
                }
            }
#endif
        }
    }

    if (seekVideoSet)
    {
        /* START TIME TESTS
         These tests will try out various start time scenarios to ensure they match expectations.
         */
        {
            std::unique_ptr<PlayerTest> startTimeTest = std::make_unique<PlayerTest>(*this, "StartTime Test: Prepare, Play, Evaluate", "StartTime");
            startTimeTest->m_testUrl = seekVideoTestUrl;
            startTimeTest->AddTestStep(PlayerTest::TestStep(ACTION_PREPARE, EVALUATOR_NONE));
            startTimeTest->m_expectedTime = 0;
            startTimeTest->AddTestStep(PlayerTest::TestStep(ACTION_PLAY, EVALUATOR_CURRENT_TIME));
            startTimeTest->AddTestStep(PlayerTest::TestStep(ACTION_STOP, EVALUATOR_NONE));
            m_PlayerTests.push_back(std::move(startTimeTest));
        }

        {
            std::unique_ptr<PlayerTest> startTimeTest = std::make_unique<PlayerTest>(*this, "StartTime Test: Set Start Time to 10s, Prepare, Play, Evaluate", "StartTime");
            startTimeTest->m_testUrl = seekVideoTestUrl;
            startTimeTest->AddTestStep(PlayerTest::TestStep(ACTION_PREPARE_START_TIME_10, EVALUATOR_NONE));
            startTimeTest->m_expectedTime = 10000;
            startTimeTest->AddTestStep(PlayerTest::TestStep(ACTION_PLAY, EVALUATOR_CURRENT_TIME));
            startTimeTest->AddTestStep(PlayerTest::TestStep(ACTION_STOP, EVALUATOR_NONE));
            m_PlayerTests.push_back(std::move(startTimeTest));
        }

        {
            std::unique_ptr<PlayerTest> startTimeTest = std::make_unique<PlayerTest>(*this, "StartTime Test: Set Start Time to 30s, Prepare, Play, Evaluate", "StartTime");
            startTimeTest->m_testUrl = seekVideoTestUrl;
            startTimeTest->AddTestStep(PlayerTest::TestStep(ACTION_PREPARE_START_TIME_30, EVALUATOR_NONE));
            startTimeTest->m_expectedTime = 30000;
            startTimeTest->AddTestStep(PlayerTest::TestStep(ACTION_PLAY, EVALUATOR_CURRENT_TIME));
            startTimeTest->AddTestStep(PlayerTest::TestStep(ACTION_STOP, EVALUATOR_NONE));
            m_PlayerTests.push_back(std::move(startTimeTest));
        }

        {
            std::unique_ptr<PlayerTest> startTimeTest = std::make_unique<PlayerTest>(*this, "StartTime Test: Set Start Time to End, Prepare, Play, Evaluate", "StartTime");
            startTimeTest->m_testUrl = seekVideoTestUrl;
            startTimeTest->AddTestStep(PlayerTest::TestStep(ACTION_GET_DURATION, EVALUATOR_NONE));
            startTimeTest->AddTestStep(PlayerTest::TestStep(ACTION_PREPARE_START_TIME_END, EVALUATOR_NONE));
            startTimeTest->AddTestStep(PlayerTest::TestStep(ACTION_PLAY, EVALUATOR_CURRENT_TIME));
            startTimeTest->AddTestStep(PlayerTest::TestStep(ACTION_STOP, EVALUATOR_NONE));
            m_PlayerTests.push_back(std::move(startTimeTest));
        }

        {
            std::unique_ptr<PlayerTest> startTimeTest = std::make_unique<PlayerTest>(*this, "StartTime Test: Set Start Time to Exceed Duration, Prepare, Play, Evaluate", "StartTime");
            startTimeTest->m_testUrl = seekVideoTestUrl;
            startTimeTest->AddTestStep(PlayerTest::TestStep(ACTION_GET_DURATION, EVALUATOR_NONE));
            startTimeTest->AddTestStep(PlayerTest::TestStep(ACTION_PREPARE_START_TIME_PAST_DURATION, EVALUATOR_NONE));
            startTimeTest->AddTestStep(PlayerTest::TestStep(ACTION_PLAY, EVALUATOR_CURRENT_TIME));
            startTimeTest->AddTestStep(PlayerTest::TestStep(ACTION_STOP, EVALUATOR_NONE));
            m_PlayerTests.push_back(std::move(startTimeTest));
        }

        {
            std::unique_ptr<PlayerTest> startTimeTest = std::make_unique<PlayerTest>(*this, "StartTime Test: Set Start Time to (0-5000), Prepare, Play, Evaluate", "StartTime");
            startTimeTest->m_testUrl = seekVideoTestUrl;
            startTimeTest->AddTestStep(PlayerTest::TestStep(ACTION_GET_DURATION, EVALUATOR_NONE));
            startTimeTest->AddTestStep(PlayerTest::TestStep(ACTION_PREPARE_START_TIME_MATH_MINUS, EVALUATOR_NONE));
            startTimeTest->AddTestStep(PlayerTest::TestStep(ACTION_PLAY, EVALUATOR_EXPECTED_NO_ERROR));
            startTimeTest->AddTestStep(PlayerTest::TestStep(ACTION_STOP, EVALUATOR_NONE));
            m_PlayerTests.push_back(std::move(startTimeTest));
        }

        {
            std::unique_ptr<PlayerTest> startTimeTest = std::make_unique<PlayerTest>(*this, "StartTime Test: Set Start Time to 10s, Prepare, SeekForward, Play, Evaluate", "StartTime");
            startTimeTest->m_testUrl = seekVideoTestUrl;
            startTimeTest->AddTestStep(PlayerTest::TestStep(ACTION_PREPARE_START_TIME_10, EVALUATOR_NONE));
            startTimeTest->AddTestStep(PlayerTest::TestStep(ACTION_SEEK_FORWARD, EVALUATOR_NONE));
            startTimeTest->AddTestStep(PlayerTest::TestStep(ACTION_PLAY, EVALUATOR_CURRENT_TIME));
            startTimeTest->AddTestStep(PlayerTest::TestStep(ACTION_STOP, EVALUATOR_NONE));
            m_PlayerTests.push_back(std::move(startTimeTest));
        }

        {
            std::unique_ptr<PlayerTest> startTimeTest = std::make_unique<PlayerTest>(*this, "StartTime Test: Set Start Time to 30s, Prepare, SeekBackward, Play, Evaluate", "StartTime");
            startTimeTest->m_testUrl = seekVideoTestUrl;
            startTimeTest->AddTestStep(PlayerTest::TestStep(ACTION_PREPARE_START_TIME_30, EVALUATOR_NONE));
            startTimeTest->AddTestStep(PlayerTest::TestStep(ACTION_SEEK_BACKWARD, EVALUATOR_NONE));
            startTimeTest->AddTestStep(PlayerTest::TestStep(ACTION_PLAY, EVALUATOR_CURRENT_TIME));
            startTimeTest->AddTestStep(PlayerTest::TestStep(ACTION_STOP, EVALUATOR_NONE));
            m_PlayerTests.push_back(std::move(startTimeTest));
        }

        /* USER INTERACTION TESTS
         The idea behind these tests is to mimic the use of the player as a user; play, pause and seeking in different directions. We also check that the PlaybackComplete signal will fire correctly. The tests perform a series of actions and will pass so long as no player error is thrown.
         Playback from invalid player states has debug guards in place (asserts) so we cannot test those without triggering them.
         It's important to note that these tests default to 'pass' but the results are not final until the test completes. This allows the result to be changed to a fail if an error occurs.
         */
        {
            std::unique_ptr<PlayerTest> userTest = std::make_unique<PlayerTest>(*this, "User Test: Prepare, Play, Stop", "UserTest");
            userTest->m_testUrl = seekVideoTestUrl;
            userTest->SetResult(true);
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PREPARE, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PLAY, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_STOP, EVALUATOR_NONE));
            m_PlayerTests.push_back(std::move(userTest));
        }

        {
            std::unique_ptr<PlayerTest> userTest = std::make_unique<PlayerTest>(*this, "User Test: Play, Pause x10", "UserTest");
            userTest->m_testUrl = seekVideoTestUrl;
            userTest->SetResult(true);
            userTest->SetTestTimeout(300000);
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PREPARE, EVALUATOR_NONE));
            std::vector<AbstractAutoTest::TestStep> toAdd;
            toAdd.push_back(PlayerTest::TestStep(ACTION_PLAY, EVALUATOR_NONE));
            toAdd.push_back(PlayerTest::TestStep(ACTION_PAUSE, EVALUATOR_NONE));
            userTest->AddTestStepSequences(toAdd, 10);
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_STOP, EVALUATOR_NONE));
            m_PlayerTests.push_back(std::move(userTest));
        }

        {
            std::unique_ptr<PlayerTest> userTest = std::make_unique<PlayerTest>(*this, "User Test: Prepare, Seek-Forward, Stop", "UserTest");
            userTest->m_testUrl = seekVideoTestUrl;
            userTest->SetResult(true);
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PREPARE, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PLAY_SHORT, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_SEEK_FORWARD, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_STOP, EVALUATOR_NONE));
            m_PlayerTests.push_back(std::move(userTest));
        }

        {
            std::unique_ptr<PlayerTest> userTest = std::make_unique<PlayerTest>(*this, "User Test: Prepare, Seek-Forward x3, Short Playback. Stop", "UserTest");
            userTest->m_testUrl = seekVideoTestUrl;
            userTest->SetResult(true);
            userTest->SetTestTimeout(300000);
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PREPARE, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_SEEK_FORWARD, EVALUATOR_NONE), 3);
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PLAY_SHORT, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_STOP, EVALUATOR_NONE));
            m_PlayerTests.push_back(std::move(userTest));
        }

        {
            std::unique_ptr<PlayerTest> userTest = std::make_unique<PlayerTest>(*this, "User Test: Prepare, Seek-Forward x2, Seek-Backward, Short Playback. Stop", "UserTest");
            userTest->m_testUrl = seekVideoTestUrl;
            userTest->SetResult(true);
            userTest->SetTestTimeout(300000);
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PREPARE, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_SEEK_FORWARD, EVALUATOR_NONE), 2);
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_SEEK_BACKWARD, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PLAY_SHORT, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_STOP, EVALUATOR_NONE));
            m_PlayerTests.push_back(std::move(userTest));
        }

        {
            std::unique_ptr<PlayerTest> userTest = std::make_unique<PlayerTest>(*this, "User Test: Prepare, Seek to End, Play to End", "UserTest");
            userTest->m_testUrl = seekVideoTestUrl;
            userTest->SetResult(true);
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PREPARE, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_SEEK_END, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PLAY_END, EVALUATOR_NONE));
            m_PlayerTests.push_back(std::move(userTest));
        }

        {
            std::unique_ptr<PlayerTest> userTest = std::make_unique<PlayerTest>(*this, "User Test: Prepare, Seek to End, Seek-Backward, Play to End", "UserTest");
            userTest->m_testUrl = seekVideoTestUrl;
            userTest->SetResult(true);
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PREPARE, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_SEEK_END, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_SEEK_BACKWARD, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PLAY_END, EVALUATOR_NONE));
            m_PlayerTests.push_back(std::move(userTest));
        }

        {
            std::unique_ptr<PlayerTest> userTest = std::make_unique<PlayerTest>(*this, "User Test: Prepare, Seek to End, Seek to Front, Play Short, Stop", "UserTest");
            userTest->m_testUrl = seekVideoTestUrl;
            userTest->SetResult(true);
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PREPARE, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_SEEK_END, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_SEEK_FRONT, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PLAY_SHORT, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_STOP, EVALUATOR_NONE));
            m_PlayerTests.push_back(std::move(userTest));
        }

        {
            std::unique_ptr<PlayerTest> userTest = std::make_unique<PlayerTest>(*this, "User Test: Prepare, Play, SeekEnd, SeekFront, Wait, Stop", "UserTest");
            userTest->m_testUrl = seekVideoTestUrl;
            userTest->SetResult(true);
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PREPARE, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PLAY, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_SEEK_END, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_SEEK_FRONT, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_WAIT_SHORT, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_STOP, EVALUATOR_NONE));
            m_PlayerTests.push_back(std::move(userTest));
        }

        {
            std::unique_ptr<PlayerTest> userTest = std::make_unique<PlayerTest>(*this, "User Test: Prepare, SeekEnd/SeekFront x5, Play, Pause, SeekEnd, PlayEnd", "UserTest");
            userTest->m_testUrl = seekVideoTestUrl;
            userTest->SetResult(true);
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PREPARE, EVALUATOR_NONE));
            std::vector<AbstractAutoTest::TestStep> toAdd;
            toAdd.push_back(AbstractAutoTest::TestStep(ACTION_SEEK_END, EVALUATOR_NONE));
            toAdd.push_back(AbstractAutoTest::TestStep(ACTION_SEEK_FRONT, EVALUATOR_NONE));
            userTest->AddTestStepSequences(toAdd, 5);
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PLAY, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PAUSE, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_SEEK_END, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PLAY_END, EVALUATOR_NONE));
            m_PlayerTests.push_back(std::move(userTest));
        }

        {
            std::unique_ptr<PlayerTest> userTest = std::make_unique<PlayerTest>(*this, "User Test: Prepare, SeekBackward, Stop", "UserTest");
            userTest->m_testUrl = seekVideoTestUrl;
            userTest->SetResult(true);
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PREPARE, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_SEEK_BACKWARD, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_STOP, EVALUATOR_NONE));
            m_PlayerTests.push_back(std::move(userTest));
        }

        {
            std::unique_ptr<PlayerTest> userTest = std::make_unique<PlayerTest>(*this, "User Test: Prepare, PlayShort, SeekBackward, Stop", "UserTest");
            userTest->m_testUrl = seekVideoTestUrl;
            userTest->SetResult(true);
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PREPARE, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PLAY_SHORT, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_SEEK_BACKWARD, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_STOP, EVALUATOR_NONE));
            m_PlayerTests.push_back(std::move(userTest));
        }

        {
            std::unique_ptr<PlayerTest> userTest = std::make_unique<PlayerTest>(*this, "User Test: Prepare, SeekEnd, SeekForward, Stop", "UserTest");
            userTest->m_testUrl = seekVideoTestUrl;
            userTest->SetResult(true);
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PREPARE, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_SEEK_END, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_SEEK_FORWARD, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_STOP, EVALUATOR_NONE));
            m_PlayerTests.push_back(std::move(userTest));
        }

        {
            std::unique_ptr<PlayerTest> userTest = std::make_unique<PlayerTest>(*this, "User Test: Prepare, SeekForward/SeekBackward x10, PlayShort, Stop", "UserTest");
            userTest->m_testUrl = seekVideoTestUrl;
            userTest->SetResult(true);
            userTest->SetTestTimeout(300000); // A bit of a longer time due to all the seeking.
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PREPARE, EVALUATOR_NONE));
            std::vector<AbstractAutoTest::TestStep> toAdd;
            toAdd.push_back(AbstractAutoTest::TestStep(ACTION_SEEK_FORWARD, EVALUATOR_NONE));
            toAdd.push_back(AbstractAutoTest::TestStep(ACTION_SEEK_BACKWARD, EVALUATOR_NONE));
            userTest->AddTestStepSequences(toAdd, 10);
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PLAY_SHORT, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_STOP, EVALUATOR_NONE));
            m_PlayerTests.push_back(std::move(userTest));
        }

        {
            std::unique_ptr<PlayerTest> userTest = std::make_unique<PlayerTest>(*this, "User Test: Prepare, Play, SeekForward/SeekBackward x10, Wait, Stop", "UserTest");
            userTest->m_testUrl = seekVideoTestUrl;
            userTest->SetResult(true);
            userTest->SetTestTimeout(300000); // A bit of a longer time due to all the seeking.
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PREPARE, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PLAY, EVALUATOR_NONE));
            std::vector<AbstractAutoTest::TestStep> toAdd;
            toAdd.push_back(AbstractAutoTest::TestStep(ACTION_SEEK_FORWARD, EVALUATOR_NONE));
            toAdd.push_back(AbstractAutoTest::TestStep(ACTION_SEEK_BACKWARD, EVALUATOR_NONE));
            userTest->AddTestStepSequences(toAdd, 10);
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_WAIT_SHORT, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_STOP, EVALUATOR_NONE));
            m_PlayerTests.push_back(std::move(userTest));
        }

        {
            std::unique_ptr<PlayerTest> userTest = std::make_unique<PlayerTest>(*this, "User Test: Prepare, Play, Pause, SeekForward/SeekBackward x10, Play Short, Stop", "UserTest");
            userTest->m_testUrl = seekVideoTestUrl;
            userTest->SetResult(true);
            userTest->SetTestTimeout(300000); // A bit of a longer time due to all the seeking.
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PREPARE, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PLAY, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PAUSE, EVALUATOR_NONE));
            std::vector<AbstractAutoTest::TestStep> toAdd;
            toAdd.push_back(AbstractAutoTest::TestStep(ACTION_SEEK_FORWARD, EVALUATOR_NONE));
            toAdd.push_back(AbstractAutoTest::TestStep(ACTION_SEEK_BACKWARD, EVALUATOR_NONE));
            userTest->AddTestStepSequences(toAdd, 10);
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_PLAY_SHORT, EVALUATOR_NONE));
            userTest->AddTestStep(PlayerTest::TestStep(ACTION_STOP, EVALUATOR_NONE));
            m_PlayerTests.push_back(std::move(userTest));
        }
    }

    /* LIVE STREAM TESTS
     The Live tests will test user interactions on a Live stream while also checking some on the available Live stream data points and APIs.
     The Live Stream UrlAndFormat will be picked during the ShortPlaybackTest. If no supported Live feed streams are found, the test will be skipped.
     */
}

void AutomatedPlayerTesterApp::StartAutomatedTestSuite()
{
    if (m_testsCompleted)
    {
        ClearTestResults();
        InitializeTests();
    }
    // This would be an entry point for executing tests, we'll need to have an arg to say what to run.
    m_testsCompleted = false;
    m_isTestRunning = true;
    m_testIndex = 0;
    PerformTest();
}

void AutomatedPlayerTesterApp::EndAutomatedTestSuite()
{
    m_testsCompleted = true;
    m_isTestRunning = false;
}

void AutomatedPlayerTesterApp::PerformTest()
{
    if (!m_testsCompleted)
    {
        if (m_testIndex >= m_PlayerTests.size())
        {
            m_testIndex = 0;
            m_testsCompleted = true;
            PerformTest();
        }
        else
        {
            m_PlayerTests[m_testIndex]->TestCompleted.Connect(*this, &AutomatedPlayerTesterApp::OnPlayerTestTestCompleted, EYIConnectionType::Async);
            YI_LOGD(PLAYER_TEST_TAG, "STARTING to execute: %s .", m_PlayerTests[m_testIndex]->GetTestId().GetData());
            m_PlayerTests[m_testIndex]->StartTest();
        }
    }
    else
    {
        EndAutomatedTestSuite();
        return;
    }
}

void AutomatedPlayerTesterApp::OnPlayerTestTestCompleted()
{
    AddTestResult(m_PlayerTests[m_testIndex]->GetResult());
    m_PlayerTests[m_testIndex]->TestCompleted.Disconnect(*this);
    YI_LOGD(PLAYER_TEST_TAG, "DONE executing: %s .", m_PlayerTests[m_testIndex]->GetTestGroup().GetData());
    m_testIndex++;
    PerformTest();
}
