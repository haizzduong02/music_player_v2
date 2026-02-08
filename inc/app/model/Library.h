#ifndef LIBRARY_H
#define LIBRARY_H

#include "app/model/MediaFile.h"
#include "interfaces/IPersistence.h"
#include "interfaces/ITrackCollection.h"
#include "utils/Subject.h"
#include <json.hpp>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

/**
 * @file Library.h
 * @brief Library model for managing media collection
 *
 * Maintains the collection of media files and notifies observers
 * when the library changes.
 */

/**
 * @brief Library class
 *
 * Manages the collection of media files in the library.
 * Implements Observer pattern to notify views of changes.
 * Follows Single Responsibility Principle - only manages library data.
 */
class Library : public Subject, public ITrackCollection
{
  public:
    /**
     * @brief Constructor
     * @param persistence Persistence layer for saving/loading
     */
    explicit Library(IPersistence *persistence);

    /**
     * @brief Add a media file to the library
     * @param mediaFile Media file to add
     * @return true if added successfully
     */
    bool addMedia(std::shared_ptr<MediaFile> mediaFile);

    /**
     * @brief Add multiple media files to the library efficiently
     * @param mediaFiles Vector of media files to add
     * @return Number of files successfully added
     */
    int addMediaBatch(const std::vector<std::shared_ptr<MediaFile>> &mediaFiles);

    /**
     * @brief Remove a media file from the library
     * @param filepath Path of the file to remove
     * @return true if removed successfully
     */
    bool removeMedia(const std::string &filepath);

    /**
     * @brief Check if a file is in the library
     * @param filepath Path to check
     * @return true if file is in library
     */
    bool contains(const std::string &filepath) const override;

    /**
     * @brief Save library to disk
     * @return true if successful
     */
    bool save();

    /**
     * @brief Load library from disk
     * @return true if successful
     */
    bool load();

    /**
     * @brief Search library by various criteria
     * @param query Search query
     * @param searchFields Fields to search in (e.g., "title", "artist")
     * @return Vector of matching media files
     */
    std::vector<std::shared_ptr<MediaFile>>
    search(const std::string &query, const std::vector<std::string> &searchFields = {"title", "artist", "album"}) const;

    /**
     * @brief Get media file by path
     * @param filepath Path to the file
     * @return Shared pointer to media file, nullptr if not found
     */
    std::shared_ptr<MediaFile> getByPath(const std::string &filepath) const;

    size_t size() const override
    {
        return mediaFiles_.size();
    }
    const std::vector<std::shared_ptr<MediaFile>> &getAll() const
    {
        return mediaFiles_;
    }

    std::unordered_set<std::string> getPathIndex() const
    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        return pathIndex_;
    }

    void clear() override;

    // ITrackCollection implementation
    bool addTrack(std::shared_ptr<MediaFile> track) override
    {
        return addMedia(track);
    }
    bool removeTrackByPath(const std::string &path) override
    {
        return removeMedia(path);
    }
    const std::vector<std::shared_ptr<MediaFile>> &getTracks() const override
    {
        return getAll();
    }

  private:
    friend class LibraryTest;
    std::vector<std::shared_ptr<MediaFile>> mediaFiles_;
    std::unordered_set<std::string> pathIndex_; // For fast lookup
    IPersistence *persistence_;
    mutable std::mutex dataMutex_; ///< Thread-safety for library operations

    /**
     * @brief Update path index
     */
    void rebuildPathIndex();
};

#endif // LIBRARY_H
