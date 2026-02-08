#include "service/TagLibMetadataReader.h"
#include <filesystem>
#include <gtest/gtest.h>

// ASSUMPTION: Tests are run from project root, so "tests/assets" is valid
const std::string ASSET_PATH = "tests/assets/sample.mp3";

class MetadataTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Skip if asset doesn't exist
        if (!std::filesystem::exists(ASSET_PATH))
        {
            GTEST_SKIP() << "Skipping: Asset not found at " << ASSET_PATH;
        }
    }
};

TEST_F(MetadataTest, ReadsMp3Tags)
{
    TagLibMetadataReader reader;

    MediaMetadata meta = reader.readMetadata(ASSET_PATH);

    // Check known values from our ffmpeg generation command
    // -metadata title="Test Title" -metadata artist="Test Artist"
    EXPECT_EQ(meta.title, "Test Title");
    EXPECT_EQ(meta.artist, "Test Artist");

    // TagLib or our reader might default to empty strings if not found

    // Check durations/bitrate - harder to predict exactly with synthetic sine wave but should be non-zero
    EXPECT_GT(meta.duration, 0);
}

TEST_F(MetadataTest, SupportsEditingReturnsTrueForMp3)
{
    TagLibMetadataReader reader;
    EXPECT_TRUE(reader.supportsEditing(ASSET_PATH));
}
