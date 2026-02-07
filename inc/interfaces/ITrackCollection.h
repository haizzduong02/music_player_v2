#ifndef I_TRACK_COLLECTION_H
#define I_TRACK_COLLECTION_H

#include <memory>
#include <string>
#include <vector>

class MediaFile;

/**
 * @file ITrackCollection.h
 * @brief Interface for collections of MediaFile objects
 *
 * Provides a common contract for track collection operations.
 * Implements Interface Segregation Principle (ISP).
 * Used by Library, Playlist, and History models.
 */

/**
 * @brief Interface for track collections
 *
 * Abstract interface defining common operations for
 * managing collections of MediaFile objects.
 */
class ITrackCollection
{
  public:
    virtual ~ITrackCollection() = default;

    /**
     * @brief Add a track to the collection
     * @param track Media file to add
     * @return true if added successfully
     */
    virtual bool addTrack(std::shared_ptr<MediaFile> track) = 0;

    /**
     * @brief Remove a track by filepath
     * @param path Path of track to remove
     * @return true if removed successfully
     */
    virtual bool removeTrackByPath(const std::string &path) = 0;

    /**
     * @brief Get all tracks in the collection
     * @return Vector of all tracks
     */
    virtual const std::vector<std::shared_ptr<MediaFile>> &getTracks() const = 0;

    /**
     * @brief Get the number of tracks
     * @return Number of tracks in collection
     */
    virtual size_t size() const = 0;

    /**
     * @brief Clear all tracks from the collection
     */
    virtual void clear() = 0;

    /**
     * @brief Check if a track exists in the collection
     * @param path Track path to check
     * @return true if track exists
     */
    virtual bool contains(const std::string &path) const = 0;
};

#endif // I_TRACK_COLLECTION_H
