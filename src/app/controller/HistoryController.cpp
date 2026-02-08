#include "app/controller/HistoryController.h"
#include "utils/Logger.h"

HistoryController::HistoryController(History *history, PlaybackController *playbackController)
    : history_(history), playbackController_(playbackController)
{
}

void HistoryController::addToHistory(std::shared_ptr<MediaFile> track)
{
    if (history_ && track)
    {
        history_->addTrack(track);
    }
}

std::vector<std::shared_ptr<MediaFile>> HistoryController::getRecentTracks(size_t count)
{
    if (!history_)
        return {};
    return history_->getRecent(count);
}

bool HistoryController::removeFromHistory(size_t index)
{
    if (!history_)
        return false;
    return history_->removeTrack(index);
}

bool HistoryController::removeFromHistoryByPath(const std::string &filepath)
{
    if (!history_)
        return false;
    return history_->removeTrackByPath(filepath);
}

void HistoryController::clearHistory()
{
    if (history_)
        history_->clear();
}

void HistoryController::playTrack(const std::vector<std::shared_ptr<MediaFile>> &context, size_t index)
{
    if (playbackController_ && index < context.size())
    {
        playbackController_->setCurrentPlaylist(nullptr);
        playbackController_->play(context[index]);
    }
}

void HistoryController::removeTracks(const std::set<std::string> &paths)
{
    if (!history_)
        return;
    for (const auto &path : paths)
    {
        history_->removeTrackByPath(path);
    }
}

void HistoryController::removeTrackByPath(const std::string &path)
{
    if (history_)
        history_->removeTrackByPath(path);
}

void HistoryController::clearAll()
{
    if (history_)
        history_->clear();
}

const std::vector<std::shared_ptr<MediaFile>> &HistoryController::getAllHistory()
{
    static const std::vector<std::shared_ptr<MediaFile>> empty;
    return history_ ? history_->getTracks() : empty;
}
