#ifndef _AUTOMATEDPLAYERTESTERAPP_H_
#define _AUTOMATEDPLAYERTESTERAPP_H_

#define PLAYER_TEST_TAG "PlayerTest"

#include "PlayerTesterApp.h"

#include <AbstractAutoTest.h>
#include <AbstractAutomatedTestHarness.h>

// Test Actions - not enums for due to weird inheritance
static const uint16_t ACTION_PREPARE = 0;
static const uint16_t ACTION_PREPARE_START_TIME_10 = 14;
static const uint16_t ACTION_PREPARE_START_TIME_30 = 15;
static const uint16_t ACTION_PREPARE_START_TIME_END = 16;
static const uint16_t ACTION_PREPARE_START_TIME_MATH_MINUS = 17;
static const uint16_t ACTION_PREPARE_START_TIME_PAST_DURATION = 18;
static const uint16_t ACTION_PLAY = 1;
static const uint16_t ACTION_STOP = 2;
static const uint16_t ACTION_PAUSE = 3;
static const uint16_t ACTION_PLAY_SHORT = 4;
static const uint16_t ACTION_PLAY_END = 5;
static const uint16_t ACTION_SEEK_FORWARD = 6;
static const uint16_t ACTION_SEEK_BACKWARD = 7;
static const uint16_t ACTION_SEEK_END = 8;
static const uint16_t ACTION_SEEK_FRONT = 9;
static const uint16_t ACTION_WAIT_SHORT = 10;
static const uint16_t ACTION_IS_EQUAL = 11;
static const uint16_t ACTION_IS_NOT_EQUAL = 12;
static const uint16_t ACTION_CHECK_LIVE = 13;
static const uint16_t ACTION_GET_DURATION = 19;

static const uint16_t EVALUATOR_NONE = 0; // For actions that can be done immediately, then move on to next
static const uint16_t EVALUATOR_EXPECTED_ERROR = 1;
static const uint16_t EVALUATOR_EXPECTED_NO_ERROR = 7;
static const uint16_t EVALUATOR_EXPECTED_TRUE = 2;
static const uint16_t EVALUATOR_EXPECTED_FALSE = 3;
static const uint16_t EVALUATOR_EVALUATE = 4; // Start special case evaluates after 10
static const uint16_t EVALUATOR_CURRENT_TIME = 5;
static const uint16_t EVALUATOR_DEFAULT_STATISTICS = 6;

class AutomatedPlayerTesterApp : public AbstractAutomatedTestHarness, public PlayerTesterApp
{
public:
    class PlayerTest : public AbstractAutoTest
    {
        // Every player test needs to be able to access the PlayerTesterApp's instance of the Player
        AutomatedPlayerTesterApp &playerTester_;

    public:
        PlayerTest(AutomatedPlayerTesterApp &playerTester, CYIString testID, CYIString testGroup = CYIString::EmptyString(), uint64_t testTimeoutInMS = 60000)
            : playerTester_(playerTester)
        {
            m_pVideoPlayer = playerTester_.GetVideoPlayer();
            m_testId = testID;
            m_testGroup = testGroup;
            m_testResult = TestResult(m_testId, false, CYIString::EmptyString());
        }

        PlayerTesterApp::UrlAndFormat m_testUrl;
        CYIAbstractVideoPlayer *m_pVideoPlayer;
        uint64_t m_expectedTime;
        uint64_t m_videoDuration;

        CYITimer m_waitTimer; // To be used for Wait actions
        CYITimer m_playbackTimer; // To be used for short or full playback actions

        void SetupSignals() override;
        void CleanUpSignals() override;
        void DoStep() override;

        void OnVideoReady();
        void OnVideoPlaying();
        void OnVideoPaused();
        void OnVideoStopped();
        void OnPlayerError(CYIAbstractVideoPlayer::Error error);
        void OnPlaybackComplete();
        void OnPlaybackTimerEnded();
        void OnWaitTimerEnded();
    };

protected:
    virtual bool UserInit() override;
    virtual void StartAutomatedTestSuite() override;
    virtual void EndAutomatedTestSuite() override;
    void TestTimerTimedOut();

private:
    void PerformTest();
    void InitializeTests();
    void OnPlayerTestTestCompleted();

    std::vector<std::unique_ptr<PlayerTest>> m_PlayerTests;
    UrlAndFormat m_currentTestUrl;
    bool m_testsCompleted;
    uint32_t m_testIndex;
};

#endif
