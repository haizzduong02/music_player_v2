#include "../../../inc/app/model/Library.h"
#include "../../../inc/utils/Logger.h"
#include <algorithm>

Library::Library(IPersistence* persistence)
    : persistence_(persistence) {
}

bool Library::addMedia(std::shared_ptr<MediaFile> mediaFile) {
    if (!mediaFile) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    // Check for duplicates using path index
    const std::string& path = mediaFile->getPath();
    if (pathIndex_.find(path) != pathIndex_.end()) {
        Logger::getInstance().warn("File already in library: " + path);
        return false;
    }
    
    // Add to collection
    mediaFiles_.push_back(mediaFile);
    pathIndex_.insert(path);
    mediaFile->setInLibrary(true);
    
    Logger::getInstance().info("Added to library: " + path);
    Subject::notify();
    return true;
}

bool Library::removeMedia(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    auto it = std::find_if(mediaFiles_.begin(), mediaFiles_.end(),
        [&filepath](const std::shared_ptr<MediaFile>& file) {
            return file->getPath() == filepath;
        });
    
    if (it == mediaFiles_.end()) {
        return false;
    }
    
    (*it)->setInLibrary(false);
    mediaFiles_.erase(it);
    pathIndex_.erase(filepath);
    
    Logger::getInstance().info("Removed from library: " + filepath);
    Subject::notify();
    return true;
}

std::vector<std::shared_ptr<MediaFile>> Library::search(
    const std::string& query,
    const std::vector<std::string>& searchFields) const {
    
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    if (query.empty()) {
        return mediaFiles_;  // Return all if query is empty
    }
    
    std::vector<std::shared_ptr<MediaFile>> results;
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
    
    for (const auto& file : mediaFiles_) {
        const MediaMetadata& meta = file->getMetadata();
        
        for (const auto& field : searchFields) {
            std::string fieldValue;
            
            if (field == "title") fieldValue = meta.title;
            else if (field == "artist") fieldValue = meta.artist;
            else if (field == "album") fieldValue = meta.album;
            else if (field == "genre") fieldValue = meta.genre;
            else continue;
            
            // Case-insensitive search
            std::transform(fieldValue.begin(), fieldValue.end(), fieldValue.begin(), ::tolower);
            
            if (fieldValue.find(lowerQuery) != std::string::npos) {
                results.push_back(file);
                break;  // Don't add same file multiple times
            }
        }
    }
    
    return results;
}

std::shared_ptr<MediaFile> Library::getByPath(const std::string& filepath) const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    // Fast lookup using path index
    if (pathIndex_.find(filepath) == pathIndex_.end()) {
        return nullptr;
    }
    
    auto it = std::find_if(mediaFiles_.begin(), mediaFiles_.end(),
        [&filepath](const std::shared_ptr<MediaFile>& file) {
            return file->getPath() == filepath;
        });
    
    return (it != mediaFiles_.end()) ? *it : nullptr;
}

void Library::clear() {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    for (auto& file : mediaFiles_) {
        file->setInLibrary(false);
    }
    
    mediaFiles_.clear();
    pathIndex_.clear();
    
    Logger::getInstance().info("Library cleared");
    Subject::notify();
}

bool Library::save() {
    if (!persistence_) {
        Logger::getInstance().warn("No persistence layer configured for Library");
        return false;
    }
    
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    try {
        // TODO: Implement persistence save
        // persistence_->save("library", mediaFiles_);
        return true;
    } catch (const std::exception& e) {
        Logger::getInstance().error("Failed to save library: " + std::string(e.what()));
        return false;
    }
}

bool Library::load() {
    if (!persistence_) {
        Logger::getInstance().warn("No persistence layer configured for Library");
        return false;
    }
    
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    try {
        // TODO: Implement persistence load
        // mediaFiles_ = persistence_->load<MediaFile>("library");
        rebuildPathIndex();
        Subject::notify();
        return true;
    } catch (const std::exception& e) {
        Logger::getInstance().error("Failed to load library: " + std::string(e.what()));
        return false;
    }
}

void Library::rebuildPathIndex() {
    pathIndex_.clear();
    for (const auto& file : mediaFiles_) {
        pathIndex_.insert(file->getPath());
    }
}
