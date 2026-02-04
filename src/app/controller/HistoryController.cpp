#include "../../../inc/app/controller/HistoryController.h"
#include "../../../inc/utils/Logger.h"

HistoryController::HistoryController(History* history) : history_(history) {
}

void HistoryController::addToHistory(std::shared_ptr<MediaFile> track) {
    if (history_ && track) {
        history_->addTrack(track);
    }
}

std::vector<std::shared_ptr<MediaFile>> HistoryController::getRecentTracks(size_t count) {
    return history_->getRecent(count);
}

bool HistoryController::removeFromHistory(size_t index) {
    return history_->removeTrack(index);
}

bool HistoryController::removeFromHistoryByPath(const std::string& filepath) {
    return history_->removeTrackByPath(filepath);
}

void HistoryController::clearHistory() {
    history_->clear();
}


