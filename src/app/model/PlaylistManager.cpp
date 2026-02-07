#include "app/model/PlaylistManager.h"
#include "utils/Logger.h"
#include "service/TagLibMetadataReader.h"
#include <json.hpp>

PlaylistManager::PlaylistManager(IPersistence* persistence)
    : persistence_(persistence) {
    initializeNowPlayingPlaylist();
    initializeFavoritesPlaylist();
}

std::shared_ptr<Playlist> PlaylistManager::createPlaylist(const std::string& name) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    if (playlists_.find(name) != playlists_.end()) {
        Logger::warn("Playlist already exists: " + name);
        return nullptr;
    }
    
    auto playlist = std::make_shared<Playlist>(name);
    playlists_[name] = playlist;
    
    Logger::info("Created playlist: " + name);
    Subject::notify();
    return playlist;
}

bool PlaylistManager::deletePlaylist(const std::string& name) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    // Prevent deleting "Now Playing" or "Favorites"
    if (name == NOW_PLAYING_NAME || name == FAVORITES_PLAYLIST_NAME) {
        Logger::warn("Cannot delete system playlist: " + name);
        return false;
    }
    
    auto it = playlists_.find(name);
    if (it == playlists_.end()) {
        return false;
    }
    
    playlists_.erase(it);
    Logger::info("Deleted playlist: " + name);
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
    std::lock_guard<std::mutex> lock(dataMutex_);
    return saveAllInternal();
}

bool PlaylistManager::saveAllInternal() {
    if (!persistence_) {
        Logger::warn("No persistence layer configured for PlaylistManager");
        return false;
    }

    // We don't lock here to avoid deadlock if called from within a locked context,
    // assuming dataMutex_ is managed by caller if needed, or we lock carefully.
    // Ideally, saveAllInternal should be private and we just lock in saveAll.
    // But since this is called from loadAll (which locks), we need care.
    // ref: loadAll calls saveAllInternal? No, loadAll just loads.
    
    // Actually, let's grab the lock for the manager's map
    // Note: accessing individual playlists might require their locks too if they are being modified?
    // For now, simpler approach:
    
    nlohmann::json allPlaylistsJson = nlohmann::json::array();
    
    for (const auto& [name, playlist] : playlists_) {
        if (playlist) {
            nlohmann::json plJson;
            to_json(plJson, *playlist);
            allPlaylistsJson.push_back(plJson);
        }
    }
    
    if (persistence_->saveToFile("data/playlists.json", allPlaylistsJson.dump(4))) {
        Logger::info("Saved all playlists to data/playlists.json");
        return true;
    } else {
        Logger::error("Failed to save playlists to data/playlists.json");
        return false;
    }
}

bool PlaylistManager::loadAll() {
    if (!persistence_) return false;
    
    std::lock_guard<std::mutex> lock(dataMutex_);
    playlists_.clear();
    
    bool migrationNeeded = false;
    
    // 1. Try to load the new consolidated file
    if (persistence_->fileExists("data/playlists.json")) {
        std::string content;
        if (persistence_->loadFromFile("data/playlists.json", content)) {
            try {
                nlohmann::json j = nlohmann::json::parse(content);
                
                // Check if it's the new format (Array of objects) or old format (Array of strings)
                if (j.is_array()) {
                    if (j.empty() || j[0].is_string()) {
                        Logger::info("Detected legacy playlist index. Starting migration...");
                        migrationNeeded = true;
                    } else {
                        // New format: Array of Playlist objects
                         for (const auto& item : j) {
                            auto playlist = std::make_shared<Playlist>("");
                            item.get_to(*playlist); // from_json
                            if (!playlist->getName().empty()) {
                                playlists_[playlist->getName()] = playlist;
                            }
                        }
                        
                        // Ensure system playlists exist
                        initializeNowPlayingPlaylist();
                        initializeFavoritesPlaylist();
                        
                        Logger::info("Loaded " + std::to_string(playlists_.size()) + " playlists from single file.");
                        return true;
                    }
                }
            } catch (const std::exception& e) {
                 Logger::error("Failed to parse playlists.json: " + std::string(e.what()));
                 migrationNeeded = true; 
            }
        }
    } else {
         migrationNeeded = true;
    }

    if (migrationNeeded) {
        Logger::info("Checking for legacy playlist files to migrate...");
        
        std::vector<std::string> legacyNames;
        if (persistence_->fileExists("data/playlists.json")) {
             std::string content;
             if (persistence_->loadFromFile("data/playlists.json", content)) {
                 try {
                     nlohmann::json j = nlohmann::json::parse(content);
                     if (j.is_array() && !j.empty() && j[0].is_string()) {
                         legacyNames = j.get<std::vector<std::string>>();
                     }
                 } catch (...) {}
             }
        }
        
        // Also manually add system playlists to check
        legacyNames.push_back(NOW_PLAYING_NAME);
        legacyNames.push_back(FAVORITES_PLAYLIST_NAME);
        
        for (const auto& name : legacyNames) {
            std::string filename = "data/playlist_" + name + ".json";
            if (persistence_->fileExists(filename)) {
                std::string content;
                if (persistence_->loadFromFile(filename, content)) {
                     try {
                        nlohmann::json j = nlohmann::json::parse(content);
                        auto playlist = std::make_shared<Playlist>(name); 
                        
                        if (j.contains("tracks")) {
                             for (const auto& item : j["tracks"]) {
                                auto track = std::make_shared<MediaFile>(""); 
                                item.get_to(*track);
                                playlist->addTrack(track); 
                             }
                        }
                         playlists_[name] = playlist;
                         Logger::info("Migrated playlist: " + name);
                         
                         // Delete legacy file after successful migration
                         persistence_->deleteFile(filename);
                         Logger::info("Deleted legacy file: " + filename);
                     } catch (...) {}
                }
            }
        }

        initializeNowPlayingPlaylist();
        initializeFavoritesPlaylist();

        saveAllInternal();
        Logger::info("Migration complete. Saved to consolidated file.");
        return true;
    }
    
    return false;
}

bool PlaylistManager::renamePlaylist(const std::string& oldName, const std::string& newName) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    // Prevent renaming "Now Playing" or "Favorites"
    if (oldName == NOW_PLAYING_NAME || oldName == FAVORITES_PLAYLIST_NAME) {
        Logger::warn("Cannot rename system playlist: " + oldName);
        return false;
    }
    
    // Check if old name exists
    auto it = playlists_.find(oldName);
    if (it == playlists_.end()) {
        return false;
    }
    
    // Check if new name already exists
    if (playlists_.find(newName) != playlists_.end()) {
        Logger::warn("Playlist with new name already exists: " + newName);
        return false;
    }
    
    // Rename
    auto playlist = it->second;
    playlist->rename(newName);
    playlists_.erase(it);
    playlists_[newName] = playlist;
    
    Logger::info("Renamed playlist from '" + oldName + "' to '" + newName + "'");
    Subject::notify();
    return true;
}

bool PlaylistManager::exists(const std::string& name) const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    return playlists_.find(name) != playlists_.end();
}

void PlaylistManager::initializeNowPlayingPlaylist() {
    if (playlists_.find(NOW_PLAYING_NAME) == playlists_.end()) {
        auto nowPlaying = std::make_shared<Playlist>(NOW_PLAYING_NAME);
        playlists_[NOW_PLAYING_NAME] = nowPlaying;
        Logger::info("Initialized 'Now Playing' playlist");
    }
}

void PlaylistManager::initializeFavoritesPlaylist() {
    if (playlists_.find(FAVORITES_PLAYLIST_NAME) == playlists_.end()) {
        auto favorites = std::make_shared<Playlist>(FAVORITES_PLAYLIST_NAME);
        playlists_[FAVORITES_PLAYLIST_NAME] = favorites;
        Logger::info("Initialized 'Favorites' playlist");
    }
}
