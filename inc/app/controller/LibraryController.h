#ifndef LIBRARY_CONTROLLER_H
#define LIBRARY_CONTROLLER_H

#include "interfaces/ITrackListController.h"
#include "app/model/Library.h"
#include "app/controller/PlaybackController.h"
#include "interfaces/IFileSystem.h"
#include "interfaces/IMetadataReader.h"

/**
 * @file LibraryController.h
 * @brief Controller for library operations
 * 
 * Handles business logic for library management.
 * Coordinates between Library model, FileSystem, and MetadataReader.
 * Follows Single Responsibility Principle - only manages library operations.
 */

/**
 * @brief Library controller class
 * 
 * Orchestrates library operations using injected dependencies (DIP).
 * Does not manage UI - that's the View's responsibility.
 */
class LibraryController : public ITrackListController {
public:
    /**
     * @brief Constructor with dependency injection
     * @param library Library model
     * @param fileSystem File system interface
     * @param metadataReader Metadata reader interface
     */
    LibraryController(
        Library* library,
        IFileSystem* fileSystem,
        IMetadataReader* metadataReader,
        PlaybackController* playbackController);
    
    /**
     * @brief Add media files from a directory
     * @param directoryPath Path to scan
     * @param recursive Scan recursively
     * @return Number of files added
     */
    int addMediaFilesFromDirectory(const std::string& directoryPath, bool recursive = true);
    
    /**
     * @brief Add a single media file
     * @param filepath Path to the media file
     * @return true if added successfully
     */
    bool addMediaFile(const std::string& filepath);
    
    /**
     * @brief Remove media from library
     * @param filepath Path of file to remove
     * @return true if removed successfully
     */
    bool removeMedia(const std::string& filepath);
    
    /**
     * @brief Search library
     * @param query Search query
     * @param searchFields Fields to search in
     * @return Vector of matching media files
     */
    std::vector<std::shared_ptr<MediaFile>> searchMedia(
        const std::string& query,
        const std::vector<std::string>& searchFields = {"title", "artist", "album"});
    
    /**
     * @brief Refresh library (reload metadata for all files)
     * @return Number of files refreshed
     */
    int refreshLibrary();
    
    /**
     * @brief Verify library files (check if files still exist)
     * Removes missing files from library
     * @return Number of files removed
     */
    int verifyLibrary();
    
    /**
     * @brief Get library statistics
     * @param totalFiles Output: total files
     * @param totalSize Output: total size in bytes
     * @param totalDuration Output: total duration in seconds
     */
    void getLibraryStats(size_t& totalFiles, size_t& totalSize, int& totalDuration);
    
    /**
     * @brief Update metadata for a file
     * @param filepath File to update
     * @param metadata New metadata
     * @return true if updated successfully
     */
    bool updateMetadata(const std::string& filepath, const MediaMetadata& metadata);

    // ITrackListController implementation
    void playTrack(const std::vector<std::shared_ptr<MediaFile>>& context, size_t index) override;
    void removeTracks(const std::set<std::string>& paths) override;
    void removeTrackByPath(const std::string& path) override;
    void clearAll() override;
    
private:
    Library* library_;
    IFileSystem* fileSystem_;
    IMetadataReader* metadataReader_;
    PlaybackController* playbackController_;
};

#endif // LIBRARY_CONTROLLER_H
