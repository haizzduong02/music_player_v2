#ifndef I_TRACK_LIST_CONTROLLER_H
#define I_TRACK_LIST_CONTROLLER_H

#include <string>
#include <vector>
#include <set>
#include <memory>
#include "../app/model/MediaFile.h"

/**
 * @brief Interface for track list management operations.
 */
class ITrackListController {
public:
    virtual ~ITrackListController() = default;

    /**
     * @brief Play a track from a specific context
     */
    virtual void playTrack(const std::vector<std::shared_ptr<MediaFile>>& context, size_t index) = 0;

    /**
     * @brief Remove specific tracks by their paths
     */
    virtual void removeTracks(const std::set<std::string>& paths) = 0;

    /**
     * @brief Remove a single track by path
     */
    virtual void removeTrackByPath(const std::string& path) = 0;

    /**
     * @brief Clear all tracks in this list
     */
    virtual void clearAll() = 0;
};

#endif // I_TRACK_LIST_CONTROLLER_H
