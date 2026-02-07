#ifndef HISTORY_H
#define HISTORY_H

#include "app/model/MediaFile.h"
#include "interfaces/IPersistence.h"
#include "interfaces/ITrackCollection.h"
#include "utils/Subject.h"
#include <deque>
#include <memory>
#include <unordered_set>
#include <vector>

/**
 * @file History.h
 * @brief Playback history model
 *
 * Maintains a history of recently played tracks.
 * Automatically moves duplicates to the top.
 */

/**
 * @brief History class
 *
 * Manages playback history with a maximum size limit.
 * Prevents duplicates by moving existing entries to the top.
 * Notifies observers when history changes.
 */
class History : public Subject, public ITrackCollection
{
  public:
    /**
     * @brief Constructor
     * @param maxSize Maximum number of entries to keep
     * @param persistence Persistence layer (DIP)
     */
    explicit History(size_t maxSize = 50, IPersistence *persistence = nullptr);

    /**
     * @brief Add a track to history (at the top)
     * If track already exists, move it to top
     * @param track Media file to add
     * @return true always (history always accepts)
     */
    bool addTrack(std::shared_ptr<MediaFile> track) override;

    /**
     * @brief Remove a track from history
     * @param index Index of track to remove
     * @return true if removed successfully
     */
    bool removeTrack(size_t index);

    /**
     * @brief Remove track by filepath
     * @param filepath Path of track to remove
     * @return true if removed successfully
     */
    bool removeTrackByPath(const std::string &filepath) override;

    void clear() override;

    /**
     * @brief Get recent tracks
     * @param count Number of recent tracks to get
     * @return Vector of recent tracks
     */
    std::vector<std::shared_ptr<MediaFile>> getRecent(size_t count) const;

    std::shared_ptr<MediaFile> getTrack(size_t index) const;
    const std::vector<std::shared_ptr<MediaFile>> &getAll() const
    {
        return history_;
    }

    // ITrackCollection implementation
    const std::vector<std::shared_ptr<MediaFile>> &getTracks() const override
    {
        return getAll();
    }
    size_t size() const override
    {
        return history_.size();
    }
    bool contains(const std::string &path) const override;

    bool isEmpty() const
    {
        return history_.empty();
    }
    void setMaxSize(size_t maxSize);
    size_t getMaxSize() const
    {
        return maxSize_;
    }

    /**
     * @brief Save history to disk
     * @return true if saved successfully
     */
    bool save();

    /**
     * @brief Load history from disk
     * @return true if loaded successfully
     */
    bool load();

  private:
    std::vector<std::shared_ptr<MediaFile>> history_;
    size_t maxSize_;
    IPersistence *persistence_;
    mutable std::mutex dataMutex_; ///< Thread-safety for history operations

    /**
     * @brief Find index of track by path
     * @param filepath Path to find
     * @return Index if found, -1 otherwise
     */
    int findTrackIndex(const std::string &filepath) const;

    /**
     * @brief Trim history to max size
     */
    void trimToMaxSize();
};

#endif // HISTORY_H
