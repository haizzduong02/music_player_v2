#include "../../../inc/app/controller/PlaylistController.h"
#include "../../../inc/app/model/MediaFileFactory.h"
#include "../../../inc/utils/Logger.h"

PlaylistController::PlaylistController(PlaylistManager* playlistManager, Library* library, IMetadataReader* metadataReader)
    : playlistManager_(playlistManager), library_(library), metadataReader_(metadataReader) {
}

bool PlaylistController::createPlaylist(const std::string& name) {
    auto playlist = playlistManager_->createPlaylist(name);
    return playlist != nullptr;
}

bool PlaylistController::deletePlaylist(const std::string& name) {
    return playlistManager_->deletePlaylist(name);
}

bool PlaylistController::renamePlaylist(const std::string& oldName, const std::string& newName) {
    return playlistManager_->renamePlaylist(oldName, newName);
}

bool PlaylistController::addToPlaylist(const std::string& playlistName, const std::string& filepath) {
    // Get file from library
    auto file = library_->getByPath(filepath);
    if (!file) {
        Logger::getInstance().warn("File not in library: " + filepath);
        return false;
    }
    
    // Get playlist
    auto playlist = playlistManager_->getPlaylist(playlistName);
    if (!playlist) {
        Logger::getInstance().warn("Playlist not found: " + playlistName);
        return false;
    }
    
    return playlist->addTrack(file);
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
            Logger::getInstance().error("Failed to add file to library: " + filepath);
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
    
    return playlist->removeTrack(trackIndex);
}

bool PlaylistController::removeFromPlaylistByPath(
    const std::string& playlistName,
    const std::string& filepath) {
    
    auto playlist = playlistManager_->getPlaylist(playlistName);
    if (!playlist) {
        return false;
    }
    
    return playlist->removeTrackByPath(filepath);
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
    
    playlist->setLoop(loop);
    return true;
}
