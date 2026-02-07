#ifndef HISTORY_CONTROLLER_H
#define HISTORY_CONTROLLER_H

#include "../model/History.h"
#include "../controller/PlaybackController.h"
#include <memory>
#include "../interfaces/ITrackListController.h"

/**
 * @file HistoryController.h
 * @brief Controller for history operations
 * 
 * Handles business logic for history management.
 * Simple controller following Single Responsibility Principle.
 */

/**
 * @brief History controller class
 * 
 * Orchestrates history operations.
 */
class HistoryController : public ITrackListController {
public:
    /**
     * @brief Constructor with dependency injection
     * @param history History model
     */
    explicit HistoryController(History* history, PlaybackController* playbackController);
    
    /**
     * @brief Add track to history
     * @param track Track to add
     */
    void addToHistory(std::shared_ptr<MediaFile> track);
    
    /**
     * @brief Remove track from history
     * @param index Index to remove
     * @return true if removed successfully
     */
    bool removeFromHistory(size_t index);
    
    /**
     * @brief Remove track by filepath
     * @param filepath Path to remove
     * @return true if removed successfully
     */
    bool removeFromHistoryByPath(const std::string& filepath);
    
    /**
     * @brief Clear all history
     */
    void clearHistory();
    
    /**
     * @brief Get recent tracks
     * @param count Number of tracks to get
     * @return Vector of recent tracks
     */
    std::vector<std::shared_ptr<MediaFile>> getRecentTracks(size_t count);
    
    /**
     * @brief Get all history
     * @return Vector of all tracks in history
     */
    const std::vector<std::shared_ptr<MediaFile>>& getAllHistory();

    // ITrackListController implementation
    void playTrack(const std::vector<std::shared_ptr<MediaFile>>& context, size_t index) override;
    void removeTracks(const std::set<std::string>& paths) override;
    void removeTrackByPath(const std::string& path) override;
    void clearAll() override;
    
private:
    History* history_;
    PlaybackController* playbackController_;
};

#endif // HISTORY_CONTROLLER_H
