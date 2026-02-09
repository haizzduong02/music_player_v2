#ifndef PLAYLIST_CONTROLLER_H
#define PLAYLIST_CONTROLLER_H

#include "app/model/Library.h"
#include "app/model/PlaylistManager.h"
#include "interfaces/IMetadataReader.h"
#include <string>
#include <vector>

/**
 * @file PlaylistController.h
 * @brief Controller for playlist operations
 *
 * Handles business logic for playlist management.
 * Coordinates between PlaylistManager and Library.
 */

/**
 * @brief Playlist controller class
 *
 * Orchestrates playlist operations using injected dependencies (DIP).
 */
class PlaylistController
{
  public:
    /**
     * @brief Constructor with dependency injection
     * @param playlistManager Playlist manager
     * @param library Library model (for adding tracks)
     * @param metadataReader Metadata reader for new tracks
     */
    PlaylistController(PlaylistManager *playlistManager, Library *library, IMetadataReader *metadataReader);
    virtual ~PlaylistController() = default;

    /**
     * @brief Create a new playlist
     * @param name Playlist name
     * @return true if created successfully
     */
    bool createPlaylist(const std::string &name);

    /**
     * @brief Delete a playlist
     * @param name Playlist name
     * @return true if deleted successfully
     */
    bool deletePlaylist(const std::string &name);

    /**
     * @brief Rename a playlist
     * @param oldName Current name
     * @param newName New name
     * @return true if renamed successfully
     */
    bool renamePlaylist(const std::string &oldName, const std::string &newName);

    /**
     * @brief Add track to playlist
     * @param playlistName Playlist to add to
     * @param filepath Track to add
     * @return true if added successfully
     */
    bool addToPlaylist(const std::string &playlistName, const std::string &filepath);

    /**
     * @brief Add track to playlist and library
     * If track not in library, adds it first
     * @param playlistName Playlist to add to
     * @param filepath Track to add
     * @return true if added successfully
     */
    virtual bool addToPlaylistAndLibrary(const std::string &playlistName, const std::string &filepath);

    /**
     * @brief Remove track from playlist
     * @param playlistName Playlist name
     * @param trackIndex Track index to remove
     * @return true if removed successfully
     */
    bool removeFromPlaylist(const std::string &playlistName, size_t trackIndex);

    /**
     * @brief Remove track from playlist by filepath
     * @param playlistName Playlist name
     * @param filepath Track path to remove
     * @return true if removed successfully
     */
    bool removeFromPlaylistByPath(const std::string &playlistName, const std::string &filepath);

    /**
     * @brief Remove track from ALL playlists by filepath
     * @param filepath Track path to remove
     * @return Number of playlists affected
     */
    int removeTrackFromAllPlaylists(const std::string &filepath);

    /**
     * @brief Get a playlist
     * @param name Playlist name
     * @return Pointer to playlist, nullptr if not found
     */
    std::shared_ptr<Playlist> getPlaylist(const std::string &name);

    /**
     * @brief Get all playlist names
     * @return Vector of playlist names
     */
    std::vector<std::string> getPlaylistNames();

    /**
     * @brief Get "Now Playing" playlist
     * @return Pointer to Now Playing playlist
     */
    std::shared_ptr<Playlist> getNowPlayingPlaylist();

    /**
     * @brief Shuffle a playlist
     * @param name Playlist name
     * @return true if shuffled successfully
     */
    bool shufflePlaylist(const std::string &name);

    /**
     * @brief Set loop mode for a playlist
     * @param name Playlist name
     * @param loop Loop enabled
     * @return true if set successfully
     */
    bool setPlaylistLoop(const std::string &name, bool loop);

    /**
     * @brief Get Library instance
     * @return Pointer to Library model
     */
    Library *getLibrary() const
    {
        return library_;
    }

  private:
    PlaylistManager *playlistManager_;
    Library *library_;
    IMetadataReader *metadataReader_;
};

#endif // PLAYLIST_CONTROLLER_H
