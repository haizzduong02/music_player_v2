#ifndef MOCK_TRACK_LIST_CONTROLLER_H
#define MOCK_TRACK_LIST_CONTROLLER_H

#include <gmock/gmock.h>
#include "interfaces/ITrackListController.h"

class MockTrackListController : public ITrackListController {
public:
    MOCK_METHOD(void, playTrack, (const std::vector<std::shared_ptr<MediaFile>>&, size_t), (override));
    MOCK_METHOD(void, removeTracks, (const std::set<std::string>&), (override));
    MOCK_METHOD(void, removeTrackByPath, (const std::string&), (override));
    MOCK_METHOD(void, clearAll, (), (override));
};

#endif // MOCK_TRACK_LIST_CONTROLLER_H
