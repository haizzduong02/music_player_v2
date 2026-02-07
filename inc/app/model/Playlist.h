#ifndef PLAYLIST_H
#define PLAYLIST_H

#include "MediaFile.h"
#include "../../utils/Subject.h"
#include "../../interfaces/IPersistence.h"
#include <vector>
#include <memory>
#include <string>
#include <json.hpp>

/**
 * @file Playlist.h
 * @brief Playlist model
 * 
 * Represents a single playlist with ordered tracks.
 * Notifies observers when playlist changes.
 */

/**
 * @brief Playlist class
 * 
 * Manages an ordered list of media tracks.
 * Supports shuffle and loop modes.
 * Implements Observer pattern (extends Subject).
 */
/**
 * @brief Repeat mode for playback
 */
enum class RepeatMode {
    NONE,
    ALL,
    ONE
};

class Playlist : public Subject {
public:
    /**
     * @brief Constructor
     * @param name Playlist name
     * @param persistence Persistence layer (DIP)
     */
    explicit Playlist(const std::string& name, IPersistence* persistence = nullptr);
    
    /**
     * @brief Add a track to the playlist
     * @param track Media file to add
     * @return true if added successfully
     */
    bool addTrack(std::shared_ptr<MediaFile> track);
    
    /**
     * @brief Insert track at specific position
     * @param track Media file to add
     * @param position Position to insert at
     * @return true if inserted successfully
     */
    bool insertTrack(std::shared_ptr<MediaFile> track, size_t position);
    
    /**
     * @brief Remove a track from the playlist
     * @param index Index of track to remove
     * @return true if removed successfully
     */
    bool removeTrack(size_t index);
    
    /**
     * @brief Remove track by filepath
     * @param filepath Path of track to remove
     * @return true if removed successfully
     */
    bool removeTrackByPath(const std::string& filepath);
    
    /**
     * @brief Get a track by index
     * @param index Track index
     * @return Shared pointer to track, nullptr if invalid index
     */
    std::shared_ptr<MediaFile> getTrack(size_t index) const;
    
    /**
     * @brief Save playlist to disk
     * @return true if saved successfully
     */
    bool save();
    
    /**
     * @brief Load playlist from disk
     * @return true if loaded successfully
     */
    bool load();
    
    // JSON serialization
    friend void to_json(nlohmann::json& j, const Playlist& p);
    friend void from_json(const nlohmann::json& j, Playlist& p);

    /**
     * @brief Get tracks in the playlist
     * @return Vector of tracks
     */
    const std::vector<std::shared_ptr<MediaFile>>& getTracks() const {
        std::lock_guard<std::mutex> lock(dataMutex_);
        return tracks_;
    }
    
    const std::string& getName() const { return name_; }
    
    /**
     * @brief Rename the playlist
     * @param newName New name
     */
    void rename(const std::string& newName);
    
    size_t size() const { return tracks_.size(); }
    
    bool isEmpty() const { return tracks_.empty(); }
    
    /**
     * @brief Clear all tracks
     */
    void clear();
    
    /**
     * @brief Shuffle the playlist
     */
    void shuffle();
    
    void setRepeatMode(RepeatMode mode) { repeatMode_ = mode; }
    RepeatMode getRepeatMode() const { return repeatMode_; }
    
    // Legacy support (optional, or just remove)
    bool isLoopEnabled() const { return repeatMode_ != RepeatMode::NONE; }
    

    
    /**
     * @brief Check if track exists in playlist
     * @param filepath Track path to check
     * @return true if track exists
     */
    bool contains(const std::string& filepath) const;
    
private:
    std::string name_;
    std::vector<std::shared_ptr<MediaFile>> tracks_;
    RepeatMode repeatMode_;
    IPersistence* persistence_;
    mutable std::mutex dataMutex_;  ///< Thread-safety for playlist operations
};

#endif // PLAYLIST_H
