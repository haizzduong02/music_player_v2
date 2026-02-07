#include "app/controller/PlaylistController.h"
#include "app/model/MediaFileFactory.h"
#include "utils/Logger.h"

PlaylistController::PlaylistController(PlaylistManager* playlistManager, Library* library, IMetadataReader* metadataReader)
    : playlistManager_(playlistManager), library_(library), metadataReader_(metadataReader) {
}

bool PlaylistController::createPlaylist(const std::string& name) {
    auto playlist = playlistManager_->createPlaylist(name);
    if (playlist) {
        // playlistManager_->saveAll(); // Removed: Save only on exit
        return true;
    }
    return false;
}

bool PlaylistController::deletePlaylist(const std::string& name) {
    bool success = playlistManager_->deletePlaylist(name);
    if (success) {
        // playlistManager_->saveAll(); // Removed: Save only on exit
    }
    return success;
}

bool PlaylistController::renamePlaylist(const std::string& oldName, const std::string& newName) {
    bool success = playlistManager_->renamePlaylist(oldName, newName);
    if (success) {
        // playlistManager_->saveAll(); // Removed: Save only on exit
    }
    return success;
}

bool PlaylistController::addToPlaylist(const std::string& playlistName, const std::string& filepath) {
    // Get file from library
    auto file = library_->getByPath(filepath);
    if (!file) {
        Logger::warn("File not in library: " + filepath);
        return false;
    }
    
    // Get playlist
    auto playlist = playlistManager_->getPlaylist(playlistName);
    if (!playlist) {
        Logger::warn("Playlist not found: " + playlistName);
        return false;
    }
    
    bool success = playlist->addTrack(file);
    if (success) {
        // playlistManager_->saveAll(); // Removed: Save only on exit
    }
    return success;
}

bool PlaylistController::addToPlaylistAndLibrary(
    const std::string& playlistName,
    const std::string& filepath) {
    
    // Check if file is in library
    auto file = library_->getByPath(filepath);
    
    // If not, add to library first
    if (!file) {
        file = MediaFileFactory::createMediaFile(filepath, metadataReader_);
        if (!file || !library_->addMedia(file)) {
            Logger::error("Failed to add file to library: " + filepath);
            return false;
        }
    }
    
    // Now add to playlist
    return addToPlaylist(playlistName, filepath);
}

bool PlaylistController::removeFromPlaylist(const std::string& playlistName, size_t trackIndex) {
    auto playlist = playlistManager_->getPlaylist(playlistName);
    if (!playlist) {
        return false;
    }
    
    bool success = playlist->removeTrack(trackIndex);
    if (success) {
        // playlistManager_->saveAll(); // Removed: Save only on exit
    }
    return success;
}

bool PlaylistController::removeFromPlaylistByPath(
    const std::string& playlistName,
    const std::string& filepath) {
    
    auto playlist = playlistManager_->getPlaylist(playlistName);
    if (!playlist) {
        return false;
    }
    
    bool success = playlist->removeTrackByPath(filepath);
    if (success) {
        // playlistManager_->saveAll(); // Removed: Save only on exit
    }
    return success;
}

std::shared_ptr<Playlist> PlaylistController::getPlaylist(const std::string& name) {
    return playlistManager_->getPlaylist(name);
}

std::vector<std::string> PlaylistController::getPlaylistNames() {
    auto playlists = playlistManager_->getAllPlaylists();
    std::vector<std::string> names;
    names.reserve(playlists.size());
    
    for (const auto& playlist : playlists) {
        names.push_back(playlist->getName());
    }
    
    return names;
}

std::shared_ptr<Playlist> PlaylistController::getNowPlayingPlaylist() {
    return playlistManager_->getNowPlayingPlaylist();
}

bool PlaylistController::shufflePlaylist(const std::string& name) {
    auto playlist = playlistManager_->getPlaylist(name);
    if (!playlist) {
        return false;
    }
    
    playlist->shuffle();
    return true;
}

bool PlaylistController::setPlaylistLoop(const std::string& name, bool loop) {
    auto playlist = playlistManager_->getPlaylist(name);
    if (!playlist) {
        return false;
    }
    
    playlist->setRepeatMode(loop ? RepeatMode::ALL : RepeatMode::NONE);
    return true;
}
