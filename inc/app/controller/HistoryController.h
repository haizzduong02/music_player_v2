#ifndef HISTORY_CONTROLLER_H
#define HISTORY_CONTROLLER_H

#include "../model/History.h"
#include <memory>

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
class HistoryController {
public:
    /**
     * @brief Constructor with dependency injection
     * @param history History model
     */
    explicit HistoryController(History* history);
    
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
    
private:
    History* history_;
};

#endif // HISTORY_CONTROLLER_H
