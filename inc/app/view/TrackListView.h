#ifndef TRACK_LIST_VIEW_H
#define TRACK_LIST_VIEW_H

#include "BaseView.h"
#include "../model/MediaFile.h"
#include <string>
#include <vector>
#include <set>
#include <memory>

/**
 * @brief Base class for views displaying a list of tracks with management capabilities.
 */
class TrackListView : public BaseView {
public:
    TrackListView() : isEditMode_(false) {}
    virtual ~TrackListView() = default;

protected:
    // Shared State
    bool isEditMode_;
    std::string searchQuery_;
    std::set<std::string> selectedPaths_;
    
    // UI Helpers (to be implemented/standardized in render loops)
    virtual void renderEditToolbar(const std::vector<std::shared_ptr<MediaFile>>& tracks) {
        // Shared implementation for Edit, Select All, Remove Selected buttons
    }
    
    void toggleEditMode() {
        isEditMode_ = !isEditMode_;
        if (!isEditMode_) {
            selectedPaths_.clear();
        }
    }
    
    void selectAll(const std::vector<std::shared_ptr<MediaFile>>& tracks) {
        for (const auto& track : tracks) {
            if (track) selectedPaths_.insert(track->getPath());
        }
    }
    
    bool isSelected(const std::string& path) const {
        return selectedPaths_.find(path) != selectedPaths_.end();
    }
    
    void toggleSelection(const std::string& path) {
        if (isSelected(path)) {
            selectedPaths_.erase(path);
        } else {
            selectedPaths_.insert(path);
        }
    }

    // Abstract removal - each view knows how to remove from its specific model
    virtual void removeSelectedTracks() = 0;
};

#endif // TRACK_LIST_VIEW_H
