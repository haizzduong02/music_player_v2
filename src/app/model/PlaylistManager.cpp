#include "../../../inc/app/model/PlaylistManager.h"
#include "../../../inc/utils/Logger.h"
#include "../../../inc/service/TagLibMetadataReader.h"
#include <json.hpp>

PlaylistManager::PlaylistManager(IPersistence* persistence)
    : persistence_(persistence) {
    initializeNowPlayingPlaylist();
    initializeFavoritesPlaylist();
}

std::shared_ptr<Playlist> PlaylistManager::createPlaylist(const std::string& name) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    if (playlists_.find(name) != playlists_.end()) {
        Logger::getInstance().warn("Playlist already exists: " + name);
        return nullptr;
    }
    
    auto playlist = std::make_shared<Playlist>(name, persistence_);
    playlists_[name] = playlist;
    
    Logger::getInstance().info("Created playlist: " + name);
    Subject::notify();
    return playlist;
}

bool PlaylistManager::deletePlaylist(const std::string& name) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    // Prevent deleting "Now Playing" or "Favorites"
    if (name == NOW_PLAYING_NAME || name == FAVORITES_PLAYLIST_NAME) {
        Logger::getInstance().warn("Cannot delete system playlist: " + name);
        return false;
    }
    
    auto it = playlists_.find(name);
    if (it == playlists_.end()) {
        return false;
    }
    
    playlists_.erase(it);
    Logger::getInstance().info("Deleted playlist: " + name);
    Subject::notify();
    return true;
}

std::shared_ptr<Playlist> PlaylistManager::getPlaylist(const std::string& name) const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    auto it = playlists_.find(name);
    return (it != playlists_.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<Playlist>> PlaylistManager::getAllPlaylists() const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    std::vector<std::shared_ptr<Playlist>> result;
    result.reserve(playlists_.size());
    
    for (const auto& pair : playlists_) {
        result.push_back(pair.second);
    }
    
    return result;
}

std::shared_ptr<Playlist> PlaylistManager::getNowPlayingPlaylist() const {
    return getPlaylist(NOW_PLAYING_NAME);
}

bool PlaylistManager::saveAll() {
    if (!persistence_) {
        Logger::getInstance().warn("No persistence layer configured for PlaylistManager");
        return false;
    }
    
    std::lock_guard<std::mutex> lock(dataMutex_);
    return saveAllInternal();
}

bool PlaylistManager::saveAllInternal() {
    if (!persistence_) return false;
    
    bool allSuccess = true;
    std::vector<std::string> names;
    
    for (auto& pair : playlists_) {
        names.push_back(pair.first);
        if (!pair.second->save()) {
            allSuccess = false;
        }
    }
    
    // Save playlist index
    nlohmann::json j = names;
    if (!persistence_->saveToFile("playlists.json", j.dump(4))) {
        allSuccess = false;
    }
    
    return allSuccess;
}

bool PlaylistManager::loadAll() {
    if (!persistence_) {
        Logger::getInstance().warn("No persistence layer configured for PlaylistManager");
        return false;
    }
    
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    try {
        // Load playlist names
        if (persistence_->fileExists("playlists.json")) {
            std::string content;
            if (persistence_->loadFromFile("playlists.json", content)) {
                nlohmann::json j = nlohmann::json::parse(content);
                
                for (const auto& item : j) {
                    std::string name = item.get<std::string>();
                    if (playlists_.find(name) == playlists_.end()) {
                        auto playlist = std::make_shared<Playlist>(name, persistence_);
                        playlists_[name] = playlist;
                        playlist->load();
                    } else {
                         playlists_[name]->load();
                    }
                }
            }
        }
        
        // Also load "Now Playing" if it was saved
        if (playlists_.find(NOW_PLAYING_NAME) != playlists_.end()) {
             playlists_[NOW_PLAYING_NAME]->load();
        } else {
             initializeNowPlayingPlaylist();
             playlists_[NOW_PLAYING_NAME]->load(); 
        }

        // Ensure Favorites exists and load it
        if (playlists_.find(FAVORITES_PLAYLIST_NAME) == playlists_.end()) {
            // Create manually to update map
            auto playlist = std::make_shared<Playlist>(FAVORITES_PLAYLIST_NAME, persistence_);
            playlists_[FAVORITES_PLAYLIST_NAME] = playlist;
            Logger::getInstance().info("Created playlist: " + std::string(FAVORITES_PLAYLIST_NAME));
        }
        playlists_[FAVORITES_PLAYLIST_NAME]->load();

        // Fix missing metadata (self-healing)
        bool metadataUpdated = false;
        TagLibMetadataReader metadataReader;
        
        for (auto& [name, playlist] : playlists_) {
            if (!playlist) continue;
            
            auto tracks = playlist->getTracks();
            for (auto& track : tracks) {
                if (!track) continue;
                
                // If duration is 0, metadata is likely missing/uninitialized
                if (track->getMetadata().duration == 0) {
                    try {
                        // Attempt to read metadata from file
                        MediaMetadata meta = metadataReader.readMetadata(track->getPath());
                        
                        // If we got valid data, update the track
                        if (meta.duration > 0 || !meta.title.empty()) {
                            track->setMetadata(meta);
                            metadataUpdated = true;
                            Logger::getInstance().info("Restored metadata for: " + track->getPath());
                        }
                    } catch (const std::exception& e) {
                        Logger::getInstance().warn("Error restoring metadata: " + std::string(e.what()));
                    }
                }
            }
        }
        
        if (metadataUpdated) {
            saveAllInternal();
        }

        Subject::notify();
        return true;
    } catch (const std::exception& e) {
        Logger::getInstance().error("Failed to load playlists: " + std::string(e.what()));
        return false;
    }
}

bool PlaylistManager::renamePlaylist(const std::string& oldName, const std::string& newName) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    // Prevent renaming "Now Playing" or "Favorites"
    if (oldName == NOW_PLAYING_NAME || oldName == FAVORITES_PLAYLIST_NAME) {
        Logger::getInstance().warn("Cannot rename system playlist: " + oldName);
        return false;
    }
    
    // Check if old name exists
    auto it = playlists_.find(oldName);
    if (it == playlists_.end()) {
        return false;
    }
    
    // Check if new name already exists
    if (playlists_.find(newName) != playlists_.end()) {
        Logger::getInstance().warn("Playlist with new name already exists: " + newName);
        return false;
    }
    
    // Rename
    auto playlist = it->second;
    playlist->rename(newName);
    playlists_.erase(it);
    playlists_[newName] = playlist;
    
    Logger::getInstance().info("Renamed playlist from '" + oldName + "' to '" + newName + "'");
    Subject::notify();
    return true;
}

bool PlaylistManager::exists(const std::string& name) const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    return playlists_.find(name) != playlists_.end();
}

void PlaylistManager::initializeNowPlayingPlaylist() {
    if (playlists_.find(NOW_PLAYING_NAME) == playlists_.end()) {
        auto nowPlaying = std::make_shared<Playlist>(NOW_PLAYING_NAME, persistence_);
        playlists_[NOW_PLAYING_NAME] = nowPlaying;
        Logger::getInstance().info("Initialized 'Now Playing' playlist");
    }
}

void PlaylistManager::initializeFavoritesPlaylist() {
    if (playlists_.find(FAVORITES_PLAYLIST_NAME) == playlists_.end()) {
        auto favorites = std::make_shared<Playlist>(FAVORITES_PLAYLIST_NAME, persistence_);
        playlists_[FAVORITES_PLAYLIST_NAME] = favorites;
        Logger::getInstance().info("Initialized 'Favorites' playlist");
    }
}
