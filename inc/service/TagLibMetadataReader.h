#ifndef TAGLIB_METADATA_READER_H
#define TAGLIB_METADATA_READER_H

#include "interfaces/IMetadataReader.h"
#include <map>
#include <string>

/**
 * @file TagLibMetadataReader.h
 * @brief Concrete implementation of IMetadataReader using TagLib
 *
 * Provides metadata reading/writing using TagLib library.
 */

/**
 * @brief TagLib metadata reader class
 *
 * Concrete implementation of IMetadataReader.
 * Uses TagLib for metadata operations.
 */
class TagLibMetadataReader : public IMetadataReader
{
  public:
    TagLibMetadataReader() = default;
    ~TagLibMetadataReader() override = default;

    // IMetadataReader implementation
    MediaMetadata readMetadata(const std::string &filepath) override;

    bool writeMetadata(const std::string &filepath, const MediaMetadata &metadata) override;

    std::map<std::string, std::string> extractTags(const std::string &filepath,
                                                   const std::vector<std::string> &tags) override;

    bool supportsEditing(const std::string &filepath) override;

  private:
    std::string getExtension(const std::string &filepath);
    bool isFormatSupported(const std::string &extension);
};

#endif // TAGLIB_METADATA_READER_H
