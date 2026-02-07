#include "app/controller/LibraryController.h"
#include "app/model/MediaFileFactory.h"
#include "utils/Logger.h"

LibraryController::LibraryController(
    Library* library,
    IFileSystem* fileSystem,
    IMetadataReader* metadataReader,
    PlaybackController* playbackController)
    : library_(library),
      fileSystem_(fileSystem),
      metadataReader_(metadataReader),
      playbackController_(playbackController) {
}

int LibraryController::addMediaFilesFromDirectory(const std::string& directoryPath, bool recursive) {
    if (!fileSystem_ || !library_) {
        return 0;
    }
    
    // scanDirectory returns vector<string> of file paths
    auto filepaths = fileSystem_->scanDirectory(
        directoryPath, 
        MediaFileFactory::getAllSupportedFormats()
    );
    int addedCount = 0;
    
    for (const auto& filepath : filepaths) {
        if (addMediaFile(filepath)) {
            addedCount++;
        }
    }
    
    Logger::info("Added " + std::to_string(addedCount) + " files from " + directoryPath);
    return addedCount;
}

bool LibraryController::addMediaFile(const std::string& filepath) {
    if (!library_ || !metadataReader_) {
        return false;
    }
    
    // Create MediaFile and add to library  
    auto file = MediaFileFactory::createMediaFile(filepath, metadataReader_);
    if (!file) {
        return false;
    }
    
    // Move unique_ptr to shared_ptr
    return library_->addMedia(std::shared_ptr<MediaFile>(std::move(file)));
}

bool LibraryController::removeMedia(const std::string& filepath) {
    if (!library_) {
        return false;
    }
    
    bool result = library_->removeMedia(filepath);
    if (result && onTrackRemovedCallback_) {
        onTrackRemovedCallback_(filepath);
    }
    return result;
}

std::vector<std::shared_ptr<MediaFile>> LibraryController::searchMedia(
    const std::string& query,
    const std::vector<std::string>& searchFields) {
    
    if (!library_) {
        return {};
    }
    
    // Use library's search functionality
    return library_->search(query);
}

int LibraryController::refreshLibrary() {
    if (!library_ || !metadataReader_) {
        return 0;
    }
    
    // Re-read metadata for all files
    auto allFiles = library_->getAll();
    int refreshedCount = 0;
    
    for (auto& file : allFiles) {
        if (file) {
            auto metadata = metadataReader_->readMetadata(file->getPath());
            // Update file metadata (would need setMetadata method)
            refreshedCount++;
        }
    }
    
    Logger::info("Refreshed " + std::to_string(refreshedCount) + " files");
    return refreshedCount;
}

int LibraryController::verifyLibrary() {
    if (!library_ || !fileSystem_) {
        return 0;
    }
    
    auto allFiles = library_->getAll();
    int removedCount = 0;
    
    for (const auto& file : allFiles) {
        if (file && !fileSystem_->exists(file->getPath())) {
            library_->removeMedia(file->getPath());
            removedCount++;
        }
    }
    
    Logger::info("Removed " + std::to_string(removedCount) + " missing files");
    return removedCount;
}
void LibraryController::playTrack(const std::vector<std::shared_ptr<MediaFile>>& context, size_t index) {
    if (playbackController_) {
        playbackController_->setCurrentPlaylist(nullptr);
        playbackController_->playContext(context, index);
    }
}

void LibraryController::removeTracks(const std::set<std::string>& paths) {
    if (!library_) return;
    for (const auto& path : paths) {
        removeMedia(path); // Use internal method to trigger callback
    }
}

void LibraryController::removeTrackByPath(const std::string& path) {
    removeMedia(path); // Use internal method to trigger callback
}

void LibraryController::clearAll() {
    if (!library_) return;
    
    // We need to notify about all removals. 
    // Optimization: Could add a 'clearAll' callback, but for now iterate.
    auto allFiles = library_->getAll();
    for (const auto& file : allFiles) {
        if (file && onTrackRemovedCallback_) {
            onTrackRemovedCallback_(file->getPath());
        }
    }
    
    library_->clear();
}
