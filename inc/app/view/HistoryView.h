#ifndef HISTORY_VIEW_H
#define HISTORY_VIEW_H

#include "TrackListView.h"
#include "../controller/HistoryController.h"
#include "../controller/PlaybackController.h"
#include "../model/History.h"
#include "../model/PlaylistManager.h"
#include <memory>

/**
 * @file HistoryView.h
 * @brief History view using ImGui
 * 
 * Displays playback history.
 * Observes History model for automatic updates.
 */

/**
 * @brief History view class
 * 
 * ImGui-based view for playback history.
 * Shows list of recently played tracks.
 */
class HistoryView : public TrackListView {
public:
    /**
     * @brief Constructor with dependency injection
     * @param controller History controller
     * @param history History model to observe
     */
    HistoryView(HistoryController* controller, History* history, PlaybackController* playbackController, PlaylistManager* playlistManager);
    
    /**
     * @brief Destructor - detaches from history
     */
    ~HistoryView() override;
    
    void render() override;
    void handleInput() override;
    void update(void* subject) override;
    
    History* getHistory() const { return history_; }
    
protected:
    
private:
    HistoryController* historyController_;
    History* history_;
    
    // UI state
    int selectedIndex_;
    bool showClearConfirmDialog_;
    
    /**
     * @brief Render history list
     */
    void renderHistoryList();
    
    /**
     * @brief Render history controls (clear button, etc.)
     */
    void renderHistoryControls();
    
    /**
     * @brief Render clear confirmation dialog
     */
    void renderClearConfirmDialog();
    
    /**
     * @brief Render context menu
     */
    void renderContextMenu();
};

#endif // HISTORY_VIEW_H
