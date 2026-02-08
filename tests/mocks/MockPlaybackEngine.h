#pragma once
#include "interfaces/IPlaybackEngine.h"
#include <gmock/gmock.h>

class MockPlaybackEngine : public IPlaybackEngine
{
  public:
    MOCK_METHOD(bool, play, (const std::string &), (override));
    MOCK_METHOD(void, pause, (), (override));
    MOCK_METHOD(void, resume, (), (override));
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(void, seek, (double), (override));
    MOCK_METHOD(void, setVolume, (float), (override));
    MOCK_METHOD(PlaybackStatus, getState, (), (const, override));
    MOCK_METHOD(double, getCurrentPosition, (), (const, override));
    MOCK_METHOD(double, getDuration, (), (const, override));
    MOCK_METHOD(float, getVolume, (), (const, override));
    MOCK_METHOD(bool, isFinished, (), (const, override));

    // ISubject methods
    MOCK_METHOD(void, attach, (IObserver *), (override));
    MOCK_METHOD(void, detach, (IObserver *), (override));
    MOCK_METHOD(void, notify, (), (override));

    // Video methods (optional, but good to have if we test them)
    MOCK_METHOD(void *, getVideoTexture, (), (override));
    MOCK_METHOD(void, getVideoSize, (int &, int &), (override));
    MOCK_METHOD(void, updateVideo, (), (override));
};
