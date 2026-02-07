#ifndef PLAYLIST_MANAGER_H
#define PLAYLIST_MANAGER_H

#include "Playlist.h"
#include "../../utils/Subject.h"
#include "../../interfaces/IPersistence.h"
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

/**
 * @file PlaylistManager.h
 * @brief Manager for multiple playlists
 * 
 * Manages creation, deletion, and access to all playlists.
 * Always maintains a "Now Playing" default playlist.
 */

/**
 * @brief Playlist manager class
 * 
 * Manages all playlists in the application.
 * Ensures there's always a "Now Playing" playlist.
 * Notifies observers when playlists are added/removed.
 */
class PlaylistManager : public Subject {
public:
    /**
     * @brief Constructor
     * @param persistence Persistence layer (DIP)
     */
    explicit PlaylistManager(IPersistence* persistence);
    
    /**
     * @brief Create a new playlist
     * @param name Playlist name
     * @return Pointer to created playlist, nullptr if name exists
     */
    std::shared_ptr<Playlist> createPlaylist(const std::string& name);
    
    /**
     * @brief Delete a playlist
     * @param name Playlist name
     * @return true if deleted (cannot delete "Now Playing")
     */
    bool deletePlaylist(const std::string& name);
    
    /**
     * @brief Get a playlist by name
     * @param name Playlist name
     * @return Pointer to playlist, nullptr if not found
     */
    std::shared_ptr<Playlist> getPlaylist(const std::string& name) const;
    
    /**
     * @brief Get all playlists
     * @return Vector of all playlists
     */
    std::vector<std::shared_ptr<Playlist>> getAllPlaylists() const;
    
    /**
     * @brief Get the "Now Playing" playlist
     * @return Pointer to Now Playing playlist
     */
    std::shared_ptr<Playlist> getNowPlayingPlaylist() const;
    
    /**
     * @brief Get playlist names
     * @return Vector of all playlist names
     */
    std::vector<std::string> getPlaylistNames() const;
    
    /**
     * @brief Check if a playlist exists
     * @param name Playlist name
     * @return true if playlist exists
     */
    bool exists(const std::string& name) const;
    
    /**
     * @brief Get number of playlists
     * @return Playlist count
     */
    size_t count() const {
        return playlists_.size();
    }
    
    /**
     * @brief Save all playlists to disk
     * @return true if all saved successfully
     */
    bool saveAll();
    
    /**
     * @brief Load all playlists from disk
     * @return true if loaded successfully
     */
    bool loadAll();
    
    /**
     * @brief Rename a playlist
     * @param oldName Current name
     * @param newName New name
     * @return true if renamed successfully
     */
    bool renamePlaylist(const std::string& oldName, const std::string& newName);
    
public:
    static constexpr const char* FAVORITES_PLAYLIST_NAME = "Favorites";
    
private:
    std::unordered_map<std::string, std::shared_ptr<Playlist>> playlists_;
    IPersistence* persistence_;
    mutable std::mutex dataMutex_;  ///< Thread-safety for playlist manager operations
    
    static constexpr const char* NOW_PLAYING_NAME = "Now Playing";
    
    /**
     * @brief Initialize the default "Now Playing" playlist
     */
    void initializeNowPlayingPlaylist();
    void initializeFavoritesPlaylist();

    /**
     * @brief Internal helper to save all without locking (prevents deadlock)
     */
    bool saveAllInternal();
};

#endif // PLAYLIST_MANAGER_H
