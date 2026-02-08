#ifndef MOCK_METADATA_READER_H
#define MOCK_METADATA_READER_H

#include "interfaces/IMetadataReader.h"
#include <gmock/gmock.h>

class MockMetadataReader : public IMetadataReader
{
  public:
    MOCK_METHOD(MediaMetadata, readMetadata, (const std::string &), (override));
    MOCK_METHOD(bool, writeMetadata, (const std::string &, const MediaMetadata &), (override));
    MOCK_METHOD((std::map<std::string, std::string>), extractTags,
                (const std::string &, const std::vector<std::string> &), (override));
    MOCK_METHOD(bool, supportsEditing, (const std::string &), (override));
};

#endif // MOCK_METADATA_READER_H
